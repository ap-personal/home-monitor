/**
 * @file wifi_manager.c
 * @brief WiFi Manager Implementation for ESP32 IoT Data Transmission
 * 
 * This module provides complete WiFi connectivity and HTTP data transmission
 * functionality for the ESP32 Environmental Monitor. It handles WiFi connection
 * management, network status monitoring, and secure data transmission to remote
 * servers for IoT applications.
 * 
 * Key Features:
 * - Automatic WiFi connection with retry mechanism
 * - Real-time connection status monitoring
 * - Signal strength (RSSI) measurement and reporting
 * - JSON data formatting for sensor readings
 * - HTTP POST transmission with proper headers
 * - Comprehensive error handling and recovery
 * - Event-driven architecture for responsive operation
 * 
 * Architecture Overview:
 * The WiFi manager uses ESP-IDF's event-driven WiFi framework combined with
 * FreeRTOS event groups for reliable asynchronous operation. This design
 * ensures the system remains responsive during network operations and provides
 * robust error handling for real-world deployment scenarios.
 * 
 * Connection Management:
 * - Automatic retry on connection failure (configurable attempts)
 * - Graceful handling of network disconnections
 * - Signal strength monitoring for link quality assessment
 * - NVS flash storage for network credentials persistence
 * 
 * Data Transmission Protocol:
 * - HTTP POST requests with JSON payload format
 * - Standard Content-Type and User-Agent headers
 * - Configurable server endpoint and timeout values
 * - Structured error reporting for transmission failures
 * 
 * Security Considerations:
 * - WPA2-PSK authentication for network security
 * - HTTPS support available (configure in wifi_config.h)
 * - Input validation for all data formatting operations
 * - Safe buffer handling to prevent overflows
 * 
 * @author ESP32 WiFi Team
 * @version 2.0
 * @date 2025-10-01
 */

#include "wifi_manager.h"       // WiFi manager API definitions
#include "esp_wifi.h"           // ESP32 WiFi driver functions
#include "esp_event.h"          // ESP-IDF event system
#include "esp_log.h"            // ESP-IDF logging system
#include "esp_system.h"         // ESP32 system functions
#include "esp_http_client.h"    // HTTP client for data transmission
#include "nvs_flash.h"          // Non-volatile storage for WiFi credentials
#include "freertos/FreeRTOS.h"  // FreeRTOS kernel functions
#include "freertos/task.h"      // FreeRTOS task management
#include "freertos/event_groups.h" // FreeRTOS event group synchronization
#include <string.h>             // Standard string functions
#include <stdio.h>              // Standard I/O for formatting
#include <time.h>               // Time functions for timestamps

/**
 * @brief Logging tag for WiFi manager messages
 * 
 * Used by ESP-IDF logging system to identify and filter messages from
 * the WiFi manager component during development and debugging.
 */
static const char *TAG = "WIFI_MANAGER";

// === Event Group Bit Definitions ===
// FreeRTOS event group bits used for WiFi state synchronization

/**
 * @brief Event bit indicating successful WiFi connection
 * Set when ESP32 successfully connects to WiFi and receives IP address
 */
#define WIFI_CONNECTED_BIT    BIT0

/**
 * @brief Event bit indicating WiFi connection failure
 * Set when connection attempts exceed retry limit or authentication fails
 */
#define WIFI_FAIL_BIT         BIT1

// === Static State Variables ===
// Internal state management for WiFi operations

/**
 * @brief FreeRTOS event group handle for WiFi synchronization
 * 
 * Coordinates between WiFi event handlers and application threads.
 * Allows blocking operations to wait for specific connection states.
 */
static EventGroupHandle_t wifi_event_group;

/**
 * @brief Current WiFi connection status
 * 
 * Tracks the real-time connection state for application use.
 * Updated by event handlers and read by status query functions.
 */
static wifi_status_t current_status = WIFI_STATUS_DISCONNECTED;

/**
 * @brief Current WiFi signal strength in dBm
 * 
 * Stores the received signal strength indicator (RSSI) value.
 * Updated when connection is established and periodically thereafter.
 * Range: -100 dBm (very poor) to -30 dBm (excellent)
 */
static int8_t current_rssi = 0;

/**
 * @brief Current connection retry attempt counter
 * 
 * Tracks the number of connection attempts made during current session.
 * Reset to zero on successful connection, incremented on each failure.
 * Used to implement connection retry limit logic.
 */
static int retry_count = 0;

/**
 * @brief WiFi Event Handler for Connection State Management
 * 
 * This function handles all WiFi-related events from the ESP-IDF event system.
 * It manages the complete connection lifecycle from initial startup through
 * disconnection, including retry logic and status updates.
 * 
 * Event Processing:
 * - WIFI_EVENT_STA_START: Triggers initial connection attempt
 * - WIFI_EVENT_STA_DISCONNECTED: Handles disconnection with retry logic
 * - IP_EVENT_STA_GOT_IP: Confirms successful connection and IP assignment
 * 
 * State Transitions:
 * DISCONNECTED → CONNECTING → CONNECTED (success path)
 * DISCONNECTED → CONNECTING → ERROR (failure after max retries)
 * CONNECTED → CONNECTING (on unexpected disconnection)
 * 
 * Retry Mechanism:
 * Automatically retries connection up to WIFI_RETRY_COUNT times before
 * marking the connection as failed. Provides exponential backoff behavior
 * through the underlying WiFi driver.
 * 
 * Signal Strength Monitoring:
 * On successful connection, queries the access point information to obtain
 * current RSSI value for signal quality assessment.
 * 
 * @param arg User data pointer (unused in this implementation)
 * @param event_base Event base identifier (WIFI_EVENT or IP_EVENT)
 * @param event_id Specific event type within the base
 * @param event_data Event-specific data payload
 * 
 * @note This function runs in ESP-IDF event task context
 * @note All logging and state updates must be thread-safe
 * @see esp_event_handler_register() for handler registration details
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data) 
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
        // WiFi station has started - initiate connection attempt
        esp_wifi_connect();
        current_status = WIFI_STATUS_CONNECTING;
        ESP_LOGI(TAG, "WiFi station started, initiating connection to '%s'...", WIFI_SSID);
        
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        // WiFi disconnected - implement retry logic with backoff
        if (retry_count < WIFI_RETRY_COUNT) {
            esp_wifi_connect();
            retry_count++;
            current_status = WIFI_STATUS_CONNECTING;
            ESP_LOGI(TAG, "WiFi disconnected, retry attempt %d/%d", retry_count, WIFI_RETRY_COUNT);
        } 
        else 
        {
            // Max retries exceeded - mark as failed and signal waiting tasks
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
            current_status = WIFI_STATUS_ERROR;
            current_rssi = 0;  // Clear signal strength on failure
            ESP_LOGE(TAG, "WiFi connection failed after %d attempts - check credentials and signal", WIFI_RETRY_COUNT);
        }
        
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        // Successfully obtained IP address - connection complete
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "✓ WiFi connected successfully! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        
        // Reset retry counter for future connection attempts
        retry_count = 0;
        current_status = WIFI_STATUS_CONNECTED;
        
        // Signal waiting tasks that connection is ready
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        
        // Query and store current signal strength for monitoring
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) 
        {
            current_rssi = ap_info.rssi;
            ESP_LOGI(TAG, "✓ Signal strength: %d dBm (%s)", 
                     current_rssi, 
                     (current_rssi > -50) ? "Excellent" :
                     (current_rssi > -60) ? "Good" :
                     (current_rssi > -70) ? "Fair" : "Poor");
        } 
        else 
        {
            ESP_LOGW(TAG, "Unable to query signal strength information");
            current_rssi = 0;
        }
    }
}

/**
 * @brief HTTP Client Event Handler for Response Processing
 * 
 * Handles HTTP client events during data transmission operations.
 * Provides detailed logging and error reporting for network communication
 * debugging and monitoring purposes.
 * 
 * Event Types Handled:
 * - HTTP_EVENT_ERROR: Network or protocol errors during transmission
 * - HTTP_EVENT_ON_CONNECTED: Successful connection to HTTP server
 * - HTTP_EVENT_ON_DATA: Server response data reception
 * - HTTP_EVENT_DISCONNECTED: Clean disconnection from server
 * 
 * Logging Strategy:
 * Uses different log levels based on event importance:
 * - ERROR level for transmission failures
 * - DEBUG level for normal protocol events
 * - INFO level for important status updates
 * 
 * Response Data Handling:
 * Server responses are logged for debugging purposes but not processed
 * by the application. This can be extended to handle server confirmations
 * or error responses as needed.
 * 
 * @param evt HTTP client event structure containing event details
 * @return ESP_OK to continue processing, ESP_FAIL to abort
 * 
 * @note This function runs in HTTP client task context
 * @note Response data logging is limited to avoid excessive output
 * @see esp_http_client_init() for client configuration details
 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt) 
{
    switch(evt->event_id) 
    {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP transmission error occurred");
            break;
            
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP connection established to server");
            break;
            
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP server response: %.*s", evt->data_len, (char*)evt->data);
            break;
            
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP connection closed cleanly");
            break;
            
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP header received: %s: %s", evt->header_key, evt->header_value);
            break;
            
        default:
            ESP_LOGD(TAG, "HTTP event: %d", evt->event_id);
            break;
    }
    return ESP_OK;
}

/**
 * @brief Initialize WiFi Manager and Network Subsystem
 * 
 * Performs complete initialization of the WiFi management system including
 * NVS storage, TCP/IP stack, WiFi driver, and event handling infrastructure.
 * This function must be called before any other WiFi operations.
 * 
 * Initialization Sequence:
 * 1. Initialize NVS flash storage for WiFi credentials persistence
 * 2. Create FreeRTOS event group for connection synchronization
 * 3. Initialize TCP/IP network stack and create default station interface
 * 4. Initialize WiFi driver with default configuration
 * 5. Register event handlers for WiFi and IP events
 * 6. Configure WiFi station mode with network credentials
 * 
 * NVS Storage:
 * Non-volatile storage is required by the WiFi driver to store calibration
 * data and connection parameters. If NVS initialization fails due to
 * version mismatch or insufficient space, the entire flash is erased
 * and reinitialized.
 * 
 * Event Handling:
 * Registers handlers for WIFI_EVENT and IP_EVENT to manage connection
 * state transitions and provide real-time status updates to the application.
 * 
 * Network Configuration:
 * Configures the ESP32 as a WiFi station (client) using credentials
 * defined in wifi_config.h. WPA2-PSK authentication is required for
 * security compliance.
 * 
 * @return ESP_OK on successful initialization
 * @return ESP_FAIL if critical components fail to initialize
 * 
 * @note This function must be called from application task context
 * @note WiFi credentials must be configured in wifi_config.h before compilation
 * @note Function will abort on critical errors using ESP_ERROR_CHECK
 * 
 * @see wifi_manager_connect() to establish network connection after initialization
 */
esp_err_t wifi_manager_init(void) 
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "   WiFi Manager Initialization");
    ESP_LOGI(TAG, "   Target Network: %s", WIFI_SSID);
    ESP_LOGI(TAG, "   Security: WPA2-PSK");
    ESP_LOGI(TAG, "========================================");
    
    // === NVS FLASH INITIALIZATION ===
    // Initialize non-volatile storage required for WiFi operation
    ESP_LOGI(TAG, "Initializing NVS flash storage...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_LOGW(TAG, "NVS flash needs to be erased and reinitialized");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "✓ NVS flash storage initialized successfully");
    
    // === EVENT GROUP CREATION ===
    // Create FreeRTOS event group for WiFi state synchronization
    ESP_LOGI(TAG, "Creating WiFi event synchronization group...");
    wifi_event_group = xEventGroupCreate();
    if (wifi_event_group == NULL) 
    {
        ESP_LOGE(TAG, "CRITICAL: Failed to create WiFi event group - insufficient memory");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "✓ WiFi event group created successfully");
    
    // === TCP/IP STACK INITIALIZATION ===
    // Initialize network interface and event loop infrastructure
    ESP_LOGI(TAG, "Initializing TCP/IP network stack...");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    ESP_LOGI(TAG, "✓ TCP/IP stack and network interfaces initialized");
    
    // === WIFI DRIVER INITIALIZATION ===
    // Initialize WiFi driver with default ESP-IDF configuration
    ESP_LOGI(TAG, "Initializing WiFi driver...");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_LOGI(TAG, "✓ WiFi driver initialized with default configuration");
    
    // === EVENT HANDLER REGISTRATION ===
    // Register callbacks for WiFi and IP events
    ESP_LOGI(TAG, "Registering WiFi event handlers...");
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    ESP_LOGI(TAG, "✓ Event handlers registered for WiFi and IP events");
    
    // === WIFI STATION CONFIGURATION ===
    // Configure ESP32 as WiFi station with network credentials
    ESP_LOGI(TAG, "Configuring WiFi station parameters...");
    wifi_config_t wifi_config = 
    {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,  // Require WPA2 security minimum
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_LOGI(TAG, "✓ WiFi station configured for network '%s'", WIFI_SSID);
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "✓ WiFi Manager Initialization Complete");
    ESP_LOGI(TAG, "✓ Ready for connection establishment");
    ESP_LOGI(TAG, "========================================");
    return ESP_OK;
}

/**
 * @brief Establish WiFi Connection to Configured Network
 * 
 * Initiates WiFi connection to the network specified during initialization.
 * This function starts the WiFi driver and waits for connection completion
 * or failure with a blocking operation using FreeRTOS event groups.
 * 
 * Connection Process:
 * 1. Reset retry counter for fresh connection attempt
 * 2. Start WiFi driver (triggers WIFI_EVENT_STA_START)
 * 3. Wait for either WIFI_CONNECTED_BIT or WIFI_FAIL_BIT
 * 4. Return success/failure based on final connection state
 * 
 * Blocking Behavior:
 * This function blocks until connection succeeds or all retry attempts
 * are exhausted. Maximum wait time depends on WIFI_RETRY_COUNT and
 * network response times (typically 30-60 seconds total).
 * 
 * Event Synchronization:
 * Uses FreeRTOS event groups to coordinate between this function and
 * the WiFi event handler. This ensures thread-safe operation and
 * reliable state management.
 * 
 * Retry Logic:
 * Connection retries are handled automatically by the event handler.
 * This function only initiates the process and waits for the final result.
 * 
 * @return ESP_OK on successful connection and IP address assignment
 * @return ESP_FAIL if connection fails after all retry attempts
 * 
 * @note This function blocks until connection completes or fails
 * @note Call wifi_manager_init() before calling this function
 * @note Network credentials must be valid and network must be accessible
 * 
 * @see wifi_manager_get_status() to check connection state
 * @see wifi_manager_get_rssi() to check signal strength after connection
 */
esp_err_t wifi_manager_connect(void) 
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "   WiFi Connection Establishment");
    ESP_LOGI(TAG, "   Target Network: %s", WIFI_SSID);
    ESP_LOGI(TAG, "   Max Retry Attempts: %d", WIFI_RETRY_COUNT);
    ESP_LOGI(TAG, "========================================");
    
    // Reset connection state for fresh attempt
    retry_count = 0;
    current_status = WIFI_STATUS_CONNECTING;
    
    // Clear any previous event bits to ensure clean state
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    
    // Start WiFi driver - this triggers the connection process
    ESP_LOGI(TAG, "Starting WiFi driver and connection sequence...");
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Wait for connection result with indefinite timeout
    // The event handler will set one of these bits based on connection outcome
    ESP_LOGI(TAG, "Waiting for connection result...");
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                          WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                          pdFALSE,    // Don't clear bits on exit
                                          pdFALSE,    // Wait for ANY bit (OR operation)
                                          portMAX_DELAY); // Wait indefinitely
    
    // Analyze connection result and provide detailed feedback
    if (bits & WIFI_CONNECTED_BIT) 
    {
        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, "✓ WiFi Connection Successful!");
        ESP_LOGI(TAG, "✓ Network: %s", WIFI_SSID);
        ESP_LOGI(TAG, "✓ Signal Strength: %d dBm", current_rssi);
        ESP_LOGI(TAG, "✓ Ready for data transmission");
        ESP_LOGI(TAG, "========================================");
        return ESP_OK;
    } 
    else if (bits & WIFI_FAIL_BIT) 
    {
        ESP_LOGE(TAG, "========================================");
        ESP_LOGE(TAG, "✗ WiFi Connection Failed");
        ESP_LOGE(TAG, "✗ Exhausted all %d retry attempts", WIFI_RETRY_COUNT);
        ESP_LOGE(TAG, "✗ Check network credentials and signal strength");
        ESP_LOGE(TAG, "========================================");
        return ESP_FAIL;
    }
    
    // This should not happen, but handle unexpected state
    ESP_LOGE(TAG, "Unexpected WiFi connection state - no event bits set");
    return ESP_FAIL;
}

/**
 * @brief Disconnect from Current WiFi Network
 * 
 * Gracefully disconnects from the currently connected WiFi network and
 * updates internal state accordingly. This function provides a clean
 * way to terminate network connectivity when needed.
 * 
 * Disconnection Process:
 * 1. Issue disconnect command to WiFi driver
 * 2. Update internal connection status immediately
 * 3. Clear signal strength indicator
 * 4. Log disconnection status for debugging
 * 
 * State Management:
 * Upon successful disconnection, internal state is updated to
 * WIFI_STATUS_DISCONNECTED and RSSI is cleared to indicate
 * no active connection.
 * 
 * Use Cases:
 * - Power management (disconnect to save power)
 * - Network switching (disconnect before connecting to different network)
 * - Error recovery (disconnect and reconnect to resolve issues)
 * - Application shutdown (clean termination of network services)
 * 
 * @return ESP_OK on successful disconnection command
 * @return ESP_FAIL if disconnection command fails
 * 
 * @note This function only initiates disconnection - actual disconnection
 *       is asynchronous and will trigger WIFI_EVENT_STA_DISCONNECTED
 * @note Internal state is updated immediately regardless of actual disconnection
 * @note Can be called even when not connected (will return success)
 * 
 * @see wifi_manager_connect() to re-establish connection after disconnection
 * @see wifi_manager_get_status() to monitor disconnection completion
 */
esp_err_t wifi_manager_disconnect(void) 
{
    ESP_LOGI(TAG, "Initiating WiFi disconnection...");
    
    // Issue disconnect command to WiFi driver
    esp_err_t ret = esp_wifi_disconnect();
    
    if (ret == ESP_OK) 
    {
        // Update internal state immediately for consistent API behavior
        current_status = WIFI_STATUS_DISCONNECTED;
        current_rssi = 0;  // Clear signal strength
        ESP_LOGI(TAG, "✓ WiFi disconnection command issued successfully");
        ESP_LOGI(TAG, "✓ Network services are now offline");
    } 
    else 
    {
        ESP_LOGW(TAG, "✗ WiFi disconnection command failed: %s", esp_err_to_name(ret));
        ESP_LOGW(TAG, "This may indicate WiFi driver is not running or already disconnected");
    }
    
    return ret;
}

/**
 * @brief Get Current WiFi Connection Status
 * 
 * Returns the current WiFi connection state as tracked by the WiFi manager.
 * This function provides real-time status information for application
 * decision making and user interface updates.
 * 
 * Status Values:
 * - WIFI_STATUS_DISCONNECTED: No active connection
 * - WIFI_STATUS_CONNECTING: Connection attempt in progress
 * - WIFI_STATUS_CONNECTED: Successfully connected with IP address
 * - WIFI_STATUS_ERROR: Connection failed after all retries
 * 
 * Thread Safety:
 * This function is thread-safe and can be called from any task context.
 * The returned value reflects the most recent status update from the
 * WiFi event handler.
 * 
 * @return wifi_status_t Current connection status
 * 
 * @note Status is updated asynchronously by WiFi event handlers
 * @note Use wifi_manager_is_ready() for simple connected/not-connected check
 * @see wifi_status_t for complete list of possible status values
 */
wifi_status_t wifi_manager_get_status(void) 
{
    return current_status;
}

/**
 * @brief Get Current WiFi Signal Strength (RSSI)
 * 
 * Returns the received signal strength indicator (RSSI) value in dBm
 * for the currently connected WiFi network. This value indicates the
 * quality of the wireless connection.
 * 
 * RSSI Interpretation:
 * - -30 to -50 dBm: Excellent signal strength
 * - -50 to -60 dBm: Good signal strength
 * - -60 to -70 dBm: Fair signal strength
 * - -70 to -80 dBm: Poor signal strength
 * - Below -80 dBm: Very poor signal, connection may be unstable
 * 
 * Value Validity:
 * RSSI value is only valid when WiFi status is WIFI_STATUS_CONNECTED.
 * When disconnected or during connection attempts, this function
 * returns 0 to indicate no valid signal measurement.
 * 
 * Update Frequency:
 * RSSI is updated when connection is first established and can be
 * refreshed periodically by the application if needed.
 * 
 * @return int8_t Signal strength in dBm (negative values)
 * @return 0 if not connected or signal strength unavailable
 * 
 * @note RSSI values are always negative (closer to 0 = stronger signal)
 * @note Value is only meaningful when connection status is CONNECTED
 * @see wifi_manager_get_status() to verify connection before using RSSI
 */
int8_t wifi_manager_get_rssi(void) 
{
    return current_rssi;
}

/**
 * @brief Check if WiFi is Ready for Data Transmission
 * 
 * Convenience function that returns true only when WiFi is fully connected
 * and ready for network operations such as HTTP requests or data transmission.
 * 
 * Ready Criteria:
 * WiFi is considered ready when:
 * - Connection status is WIFI_STATUS_CONNECTED
 * - IP address has been assigned
 * - Network stack is operational
 * 
 * Usage Pattern:
 * This function is typically used before attempting network operations
 * to ensure they will not fail due to connectivity issues.
 * 
 * @return true if WiFi is connected and ready for data transmission
 * @return false if WiFi is disconnected, connecting, or in error state
 * 
 * @note This is equivalent to checking (wifi_manager_get_status() == WIFI_STATUS_CONNECTED)
 * @note Use this function before calling wifi_manager_send_data()
 * @see wifi_manager_get_status() for detailed connection state information
 */
bool wifi_manager_is_ready(void) 
{
    return (current_status == WIFI_STATUS_CONNECTED);
}

/**
 * @brief Format Sensor Data as JSON for HTTP Transmission
 * 
 * Converts sensor reading data into a properly formatted JSON string suitable
 * for HTTP POST transmission to IoT servers. The JSON format follows standard
 * IoT conventions with device identification, timestamp, and sensor readings.
 * 
 * JSON Structure:
 * {
 *   "device_id": "ESP32_SENSOR_001",
 *   "timestamp": 1672531200,
 *   "temperature": 23.50,
 *   "humidity": 65.00,
 *   "rssi": -45
 * }
 * 
 * Field Descriptions:
 * - device_id: Unique identifier for this ESP32 device
 * - timestamp: Unix timestamp of the sensor reading
 * - temperature: Temperature in degrees Celsius (2 decimal places)
 * - humidity: Relative humidity percentage (2 decimal places)
 * - rssi: WiFi signal strength in dBm (current connection quality)
 * 
 * Buffer Safety:
 * The function performs bounds checking to prevent buffer overflows.
 * Minimum buffer size of 200 bytes is recommended to accommodate
 * the complete JSON structure with safety margin.
 * 
 * Error Handling:
 * - Validates all input parameters before processing
 * - Checks buffer size sufficiency before formatting
 * - Verifies JSON string length fits within buffer limits
 * - Returns specific error codes for different failure modes
 * 
 * @param data Pointer to sensor data structure to format
 * @param json_buffer Output buffer for formatted JSON string
 * @param buffer_size Size of output buffer in bytes (minimum 200)
 * 
 * @return ESP_OK on successful JSON formatting
 * @return ESP_ERR_INVALID_ARG if data pointer is NULL or buffer too small
 * @return ESP_ERR_INVALID_SIZE if formatted JSON exceeds buffer size
 * 
 * @note Formatted JSON string is null-terminated
 * @note Buffer size should be at least 200 bytes for safety
 * @note RSSI value is automatically included from current WiFi connection
 * 
 * @see sensor_data_t for input data structure definition
 * @see wifi_manager_send_data() for transmission of formatted data
 */
esp_err_t wifi_manager_format_json(const sensor_data_t* data, char* json_buffer, size_t buffer_size) 
{
    // Validate input parameters to prevent errors and security issues
    if (data == NULL) 
    {
        ESP_LOGE(TAG, "Invalid parameter: sensor data pointer is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (json_buffer == NULL) 
    {
        ESP_LOGE(TAG, "Invalid parameter: JSON buffer pointer is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (buffer_size < 200) 
    {
        ESP_LOGE(TAG, "Invalid parameter: buffer size %zu too small (minimum 200 bytes)", buffer_size);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Format sensor data as JSON with proper structure and precision
    int len = snprintf(json_buffer, buffer_size,
        "{"
        "\"device_id\":\"%s\","
        "\"timestamp\":%lu,"
        "\"temperature\":%.2f,"
        "\"humidity\":%.2f,"
        "\"rssi\":%d"
        "}",
        data->device_id,                    // Unique device identifier
        (unsigned long)data->timestamp,     // Unix timestamp for data correlation
        data->temperature,                  // Temperature in Celsius (2 decimal precision)
        data->humidity,                     // Humidity percentage (2 decimal precision)
        current_rssi                        // Current WiFi signal strength
    );
    
    // Verify JSON formatting was successful and fits in buffer
    if (len < 0) 
    {
        ESP_LOGE(TAG, "JSON formatting failed - snprintf error");
        return ESP_ERR_INVALID_SIZE;
    }
    
    if (len >= buffer_size) 
    {
        ESP_LOGE(TAG, "JSON string too long (%d bytes) for buffer (%zu bytes)", len, buffer_size);
        return ESP_ERR_INVALID_SIZE;
    }
    
    ESP_LOGD(TAG, "JSON formatted successfully: %d bytes", len);
    ESP_LOGD(TAG, "JSON content: %s", json_buffer);
    
    return ESP_OK;
}

/**
 * @brief Transmit Sensor Data to Remote Server via HTTP POST
 * 
 * Performs complete HTTP POST transmission of sensor data to a configured
 * remote server. This function handles JSON formatting, HTTP client setup,
 * request execution, and response processing with comprehensive error handling.
 * 
 * Transmission Process:
 * 1. Verify WiFi connectivity is available
 * 2. Format sensor data as JSON payload
 * 3. Initialize and configure HTTP client
 * 4. Set appropriate HTTP headers for JSON transmission
 * 5. Execute POST request with sensor data
 * 6. Process server response and status codes
 * 7. Clean up HTTP client resources
 * 
 * HTTP Configuration:
 * - Method: POST for data submission
 * - Content-Type: application/json for proper server parsing
 * - User-Agent: ESP32-SensorMonitor/1.0 for server identification
 * - Timeout: Configurable via HTTP_TIMEOUT_MS
 * - URL: Configurable via HTTP_SERVER_URL in wifi_config.h
 * 
 * Error Handling:
 * The function handles multiple error conditions:
 * - WiFi connectivity issues (returns ESP_FAIL)
 * - Invalid input parameters (returns ESP_ERR_INVALID_ARG)
 * - JSON formatting failures (returns format error code)
 * - HTTP client initialization failures (returns ESP_FAIL)
 * - Network transmission errors (returns ESP_FAIL)
 * - Server error responses (returns ESP_FAIL)
 * 
 * Success Criteria:
 * Transmission is considered successful when:
 * - HTTP request completes without network errors
 * - Server responds with status code 200-299 (success range)
 * - No timeout or connection issues occur
 * 
 * Server Response Handling:
 * - Status codes 200-299: Success (data accepted)
 * - Status codes 400-499: Client error (bad request, auth failure)
 * - Status codes 500-599: Server error (temporary server issues)
 * - Network errors: Connectivity or timeout issues
 * 
 * @param data Pointer to sensor data structure to transmit
 * 
 * @return ESP_OK on successful transmission and server acceptance
 * @return ESP_FAIL if WiFi not connected or transmission fails
 * @return ESP_ERR_INVALID_ARG if data pointer is NULL
 * @return Format error codes if JSON formatting fails
 * 
 * @note WiFi must be connected before calling this function
 * @note Function blocks until transmission completes or times out
 * @note Server URL and timeout configured in wifi_config.h
 * @note HTTP client resources are automatically cleaned up
 * 
 * @see wifi_manager_is_ready() to check WiFi connectivity first
 * @see wifi_manager_format_json() for JSON formatting details
 * @see wifi_config.h for server configuration parameters
 */
esp_err_t wifi_manager_send_data(const sensor_data_t* data) 
{
    // === CONNECTIVITY VERIFICATION ===
    // Ensure WiFi is connected before attempting transmission
    if (!wifi_manager_is_ready()) 
    {
        ESP_LOGW(TAG, "Cannot send data - WiFi not connected");
        ESP_LOGW(TAG, "Current status: %d (CONNECTED=%d)", current_status, WIFI_STATUS_CONNECTED);
        return ESP_FAIL;
    }
    
    // === INPUT VALIDATION ===
    // Verify sensor data pointer is valid
    if (data == NULL) 
    {
        ESP_LOGE(TAG, "Cannot send data - sensor data pointer is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "   HTTP Data Transmission");
    ESP_LOGI(TAG, "   Server: %s", HTTP_SERVER_URL);
    ESP_LOGI(TAG, "   Device: %s", data->device_id);
    ESP_LOGI(TAG, "========================================");
    
    // === JSON PAYLOAD FORMATTING ===
    // Convert sensor data to JSON format for HTTP transmission
    char json_buffer[HTTP_BUFFER_SIZE];
    esp_err_t ret = wifi_manager_format_json(data, json_buffer, sizeof(json_buffer));
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to format sensor data as JSON: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "JSON payload prepared: %d bytes", strlen(json_buffer));
    ESP_LOGD(TAG, "Payload content: %s", json_buffer);
    
    // === HTTP CLIENT INITIALIZATION ===
    // Configure HTTP client for JSON POST transmission
    ESP_LOGI(TAG, "Initializing HTTP client...");
    esp_http_client_config_t config = 
    {
        .url = HTTP_SERVER_URL,              // Target server endpoint
        .event_handler = http_event_handler, // Response processing callback
        .timeout_ms = HTTP_TIMEOUT_MS,       // Network timeout configuration
        .method = HTTP_METHOD_POST,          // POST method for data submission
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) 
    {
        ESP_LOGE(TAG, "Failed to initialize HTTP client - insufficient memory or invalid config");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "✓ HTTP client initialized successfully");
    
    // === HTTP HEADERS CONFIGURATION ===
    // Set appropriate headers for JSON data transmission
    ESP_LOGI(TAG, "Configuring HTTP headers...");
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "User-Agent", "ESP32-SensorMonitor/1.0");
    esp_http_client_set_header(client, "Accept", "application/json");
    ESP_LOGI(TAG, "✓ HTTP headers configured for JSON transmission");
    
    // === PAYLOAD ATTACHMENT ===
    // Attach JSON sensor data as POST request body
    esp_http_client_set_post_field(client, json_buffer, strlen(json_buffer));
    ESP_LOGI(TAG, "✓ JSON payload attached to POST request");
    
    // === HTTP REQUEST EXECUTION ===
    // Perform the actual HTTP transmission
    ESP_LOGI(TAG, "Executing HTTP POST request...");
    ret = esp_http_client_perform(client);
    
    if (ret == ESP_OK) 
    {
        // === RESPONSE PROCESSING ===
        // Analyze server response for transmission success
        int status_code = esp_http_client_get_status_code(client);
        int content_length = esp_http_client_get_content_length(client);
        
        ESP_LOGI(TAG, "HTTP transmission completed");
        ESP_LOGI(TAG, "Response status: %d", status_code);
        ESP_LOGI(TAG, "Response length: %d bytes", content_length);
        
        if (status_code >= 200 && status_code < 300) 
        {
            // Success response range (2xx status codes)
            ESP_LOGI(TAG, "========================================");
            ESP_LOGI(TAG, "✓ Data Transmission Successful!");
            ESP_LOGI(TAG, "✓ Server accepted sensor data");
            ESP_LOGI(TAG, "✓ HTTP Status: %d", status_code);
            ESP_LOGI(TAG, "========================================");
            ret = ESP_OK;
        } 
        else 
        {
            // Server error or client error response
            ESP_LOGW(TAG, "========================================");
            ESP_LOGW(TAG, "✗ Server Error Response");
            ESP_LOGW(TAG, "✗ HTTP Status: %d", status_code);
            if (status_code >= 400 && status_code < 500) 
            {
                ESP_LOGW(TAG, "✗ Client Error: Check request format and server configuration");
            } 
            else if (status_code >= 500) 
            {
                ESP_LOGW(TAG, "✗ Server Error: Remote server experiencing issues");
            }
            ESP_LOGW(TAG, "========================================");
            ret = ESP_FAIL;
        }
    } 
    else 
    {
        // === NETWORK ERROR HANDLING ===
        // Handle network-level transmission failures
        ESP_LOGE(TAG, "========================================");
        ESP_LOGE(TAG, "✗ HTTP Transmission Failed");
        ESP_LOGE(TAG, "✗ Network Error: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "✗ Check network connectivity and server availability");
        ESP_LOGE(TAG, "========================================");
    }
    
    // === RESOURCE CLEANUP ===
    // Always clean up HTTP client resources
    esp_http_client_cleanup(client);
    ESP_LOGD(TAG, "HTTP client resources cleaned up");
    
    return ret;
}