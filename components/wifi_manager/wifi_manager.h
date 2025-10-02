#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>
#include "wifi_config.h"

/**
 * @file wifi_manager.h
 * @brief WiFi Connection Manager and IoT Data Transmission System
 * 
 * This module provides comprehensive WiFi connectivity management and HTTP-based
 * data transmission capabilities for ESP32-based IoT applications. It abstracts
 * the complexity of WiFi connection handling, network error recovery, and remote
 * data transmission into a simple, reliable API.
 * 
 * Key Features:
 * - Automatic WiFi connection with configurable retry logic
 * - Real-time connection status monitoring and signal strength reporting
 * - JSON-formatted HTTP POST transmission for IoT platforms
 * - Robust error handling with graceful degradation
 * - Non-blocking operation design for embedded applications
 * - Easy integration with sensor data collection systems
 * 
 * Configuration:
 * WiFi network credentials and HTTP server settings are configured in wifi_config.h
 * This separation allows easy customization without modifying core functionality.
 * 
 * Supported Use Cases:
 * - Sensor data transmission to cloud platforms (AWS IoT, Google Cloud, etc.)
 * - Local server communication for home automation systems
 * - Webhook integration with services like Zapier, IFTTT
 * - Custom REST API endpoints for data collection
 * 
 * Network Resilience:
 * The manager handles network interruptions gracefully, automatically attempting
 * reconnection without disrupting the main application flow. Failed transmissions
 * are reported but do not block system operation.
 * 
 * @author ESP32 Development Team
 * @version 1.0
 * @date 2025-10-01
 * @see wifi_config.h for network and server configuration
 */

/**
 * @brief WiFi connection status enumeration
 * 
 * Represents the current state of WiFi connectivity, allowing applications
 * to make informed decisions about network-dependent operations and provide
 * appropriate user feedback.
 * 
 * Status Transitions:
 * DISCONNECTED -> CONNECTING -> CONNECTED (successful connection)
 * DISCONNECTED -> CONNECTING -> ERROR (failed connection after retries)
 * CONNECTED -> DISCONNECTED (network loss or manual disconnect)
 */
typedef enum {
    WIFI_STATUS_DISCONNECTED = 0,    ///< Not connected to any network
    WIFI_STATUS_CONNECTING,          ///< Attempting to establish connection
    WIFI_STATUS_CONNECTED,           ///< Successfully connected and operational
    WIFI_STATUS_ERROR                ///< Connection failed after all retry attempts
} wifi_status_t;

/**
 * @brief Sensor data structure for network transmission
 * 
 * Standardized data structure for transmitting environmental sensor readings
 * over HTTP. Includes all necessary fields for data identification, validation,
 * and temporal correlation in IoT applications.
 * 
 * Field Descriptions:
 * - temperature: Current temperature reading in degrees Celsius
 * - humidity: Current relative humidity percentage (0-100%)
 * - timestamp: Unix timestamp for data correlation and ordering
 * - device_id: Unique identifier for this ESP32 device (max 31 chars + null)
 * 
 * JSON Representation:
 * When transmitted, this structure is serialized to JSON format:
 * {
 *   "device_id": "ESP32_SENSOR_01",
 *   "timestamp": 1696118400,
 *   "temperature": 23.5,
 *   "humidity": 65.2,
 *   "rssi": -45
 * }
 */
typedef struct {
    float temperature;      ///< Temperature in Celsius (-40 to +125°C range for DHT11)
    float humidity;         ///< Relative humidity percentage (0-100%)
    uint32_t timestamp;     ///< Unix timestamp when data was collected
    char device_id[32];     ///< Unique device identifier string (null-terminated)
} sensor_data_t;

/**
 * @brief Initialize WiFi communication subsystem
 * 
 * Performs complete initialization of the WiFi stack including NVS storage setup,
 * TCP/IP networking, event handling, and WiFi driver configuration. This function
 * must be called before any other WiFi operations.
 * 
 * Initialization Process:
 * 1. Initialize NVS (Non-Volatile Storage) for WiFi configuration persistence
 * 2. Set up TCP/IP network interface and event loop
 * 3. Configure WiFi driver with optimized settings
 * 4. Register event handlers for connection state management
 * 5. Configure station mode with provided credentials
 * 
 * Configuration Source:
 * WiFi credentials (SSID, password) and security settings are loaded from
 * wifi_config.h. Modify that file to update network settings.
 * 
 * Error Conditions:
 * - NVS initialization failure (corrupted flash, insufficient space)
 * - Network interface setup failure (driver issues, memory constraints)
 * - Event handler registration failure (system resource exhaustion)
 * 
 * @return ESP_OK if initialization completed successfully
 * @return ESP_FAIL if any initialization step failed
 * 
 * @pre WiFi hardware must be functional and properly powered
 * @post WiFi system ready for connection attempts
 * @note This function only initializes - call wifi_manager_connect() to connect
 * @warning Must be called exactly once before other WiFi operations
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief Establish connection to configured WiFi network
 * 
 * Attempts to connect to the WiFi network specified in wifi_config.h using
 * the configured credentials and security settings. Implements automatic retry
 * logic with exponential backoff to handle temporary network issues.
 * 
 * Connection Process:
 * 1. Start WiFi station mode with configured parameters
 * 2. Initiate connection attempt to target network
 * 3. Wait for connection establishment or timeout
 * 4. Retry on failure up to configured maximum attempts
 * 5. Update internal connection status based on result
 * 
 * Retry Strategy:
 * - Maximum retry attempts: Defined by WIFI_RETRY_COUNT in wifi_config.h
 * - Retry delay: Progressive delay between attempts for network stability
 * - Automatic status updates: Internal state machine tracks progress
 * 
 * Blocking Behavior:
 * This function blocks until either successful connection or final failure.
 * Use wifi_manager_get_status() for non-blocking status monitoring.
 * 
 * @return ESP_OK if connection established successfully
 * @return ESP_FAIL if all connection attempts failed
 * 
 * @pre wifi_manager_init() must have completed successfully
 * @post WiFi either connected or in error state, status updated accordingly
 * @note Connection status can be monitored with wifi_manager_get_status()
 * @warning Blocks execution until connection result determined
 */
esp_err_t wifi_manager_connect(void);

/**
 * @brief Disconnect from current WiFi network
 * 
 * Gracefully terminates the current WiFi connection and updates internal state.
 * This function is useful for power management, network switching, or clean
 * shutdown procedures.
 * 
 * Disconnection Process:
 * 1. Send disconnection request to WiFi driver
 * 2. Wait for disconnection confirmation
 * 3. Update internal status to DISCONNECTED
 * 4. Reset signal strength and connection metrics
 * 
 * @return ESP_OK if disconnection completed successfully
 * @return ESP_FAIL if disconnection request failed
 * 
 * @pre Must be called only when WiFi is connected or connecting
 * @post WiFi status set to DISCONNECTED, ready for reconnection
 * @note Safe to call multiple times (idempotent operation)
 * @see wifi_manager_connect() to reconnect after disconnection
 */
esp_err_t wifi_manager_disconnect(void);

/**
 * @brief Get current WiFi connection status
 * 
 * Returns the current state of WiFi connectivity without blocking or triggering
 * any network operations. Useful for application logic that needs to adapt
 * behavior based on network availability.
 * 
 * Status Values:
 * - WIFI_STATUS_DISCONNECTED: No network connection active
 * - WIFI_STATUS_CONNECTING: Connection attempt in progress
 * - WIFI_STATUS_CONNECTED: Successfully connected and operational
 * - WIFI_STATUS_ERROR: Connection failed after all retry attempts
 * 
 * @return Current WiFi connection status
 * 
 * @note This is a non-blocking status query
 * @note Status is updated automatically by background event handlers
 * @see wifi_status_t for detailed status descriptions
 */
wifi_status_t wifi_manager_get_status(void);

/**
 * @brief Get current WiFi signal strength (RSSI)
 * 
 * Returns the Received Signal Strength Indicator (RSSI) for the current WiFi
 * connection. RSSI provides information about connection quality and can be
 * used for network diagnostics or user feedback.
 * 
 * RSSI Interpretation:
 * - -30 to -50 dBm: Excellent signal strength
 * - -50 to -60 dBm: Good signal strength
 * - -60 to -70 dBm: Fair signal strength
 * - -70 to -80 dBm: Weak signal strength
 * - Below -80 dBm: Very weak, may cause connection issues
 * 
 * @return Signal strength in dBm (negative values, closer to 0 = stronger)
 * @return 0 if not connected or signal strength unavailable
 * 
 * @pre WiFi must be connected for meaningful RSSI values
 * @note RSSI values are updated automatically during connection
 * @note Returned value is signed (negative dBm values)
 */
int8_t wifi_manager_get_rssi(void);

/**
 * @brief Send sensor data to remote server via HTTP POST
 * 
 * Transmits environmental sensor data to the configured HTTP endpoint using
 * JSON formatting. Implements comprehensive error handling and connection
 * verification to ensure reliable data delivery.
 * 
 * Transmission Process:
 * 1. Verify WiFi connectivity before attempting transmission
 * 2. Format sensor data into JSON payload with metadata
 * 3. Configure HTTP client with appropriate headers and timeout
 * 4. Execute HTTP POST request to configured server endpoint
 * 5. Validate server response and handle success/error conditions
 * 6. Clean up HTTP client resources
 * 
 * JSON Payload Format:
 * {
 *   "device_id": "ESP32_SENSOR_01",
 *   "timestamp": 1696118400,
 *   "temperature": 23.5,
 *   "humidity": 65.2,
 *   "rssi": -45
 * }
 * 
 * Error Handling:
 * - Pre-transmission WiFi connectivity verification
 * - HTTP timeout protection (configurable in wifi_config.h)
 * - Server response code validation (2xx = success)
 * - Automatic resource cleanup on success or failure
 * 
 * @param data Pointer to sensor data structure containing readings to transmit
 * 
 * @return ESP_OK if data transmitted successfully and server confirmed receipt
 * @return ESP_FAIL if transmission failed (network, server, or timeout issues)
 * @return ESP_ERR_INVALID_ARG if data pointer is NULL
 * 
 * @pre WiFi must be connected (check with wifi_manager_is_ready())
 * @pre data pointer must be valid and contain meaningful sensor readings
 * @post HTTP client resources automatically cleaned up regardless of result
 * @note This function blocks until transmission completes or times out
 * @warning Large data or slow networks may cause extended blocking
 */
esp_err_t wifi_manager_send_data(const sensor_data_t* data);

/**
 * @brief Check if WiFi is ready for data transmission
 * 
 * Convenience function that verifies WiFi connectivity status and readiness
 * for network operations. Useful for application logic that needs to determine
 * whether network-dependent features should be enabled.
 * 
 * Readiness Criteria:
 * - WiFi status must be WIFI_STATUS_CONNECTED
 * - Network interface must be operational
 * - IP address must be assigned and valid
 * 
 * @return true if WiFi is connected and ready for data transmission
 * @return false if WiFi is not connected, connecting, or in error state
 * 
 * @note This is equivalent to (wifi_manager_get_status() == WIFI_STATUS_CONNECTED)
 * @note Non-blocking status check - safe to call frequently
 * @see wifi_manager_get_status() for detailed status information
 */
bool wifi_manager_is_ready(void);

/**
 * @brief Format sensor data into JSON string for HTTP transmission
 * 
 * Converts sensor data structure into a properly formatted JSON string suitable
 * for HTTP POST transmission. Includes data validation, proper JSON escaping,
 * and metadata enrichment (RSSI, device identification).
 * 
 * JSON Structure Generated:
 * - device_id: Unique device identifier from sensor data
 * - timestamp: Unix timestamp for temporal correlation
 * - temperature: Temperature reading in Celsius (2 decimal places)
 * - humidity: Humidity percentage (2 decimal places)
 * - rssi: Current WiFi signal strength for network quality assessment
 * 
 * Buffer Requirements:
 * Minimum buffer size should be 200 bytes to accommodate full JSON structure
 * with typical sensor data values and metadata.
 * 
 * Error Conditions:
 * - NULL data pointer or buffer pointer
 * - Insufficient buffer size for complete JSON string
 * - Data formatting errors (extremely rare)
 * 
 * @param data Pointer to sensor data structure to be formatted
 * @param json_buffer Output buffer for generated JSON string
 * @param buffer_size Size of output buffer in bytes
 * 
 * @return ESP_OK if JSON formatting completed successfully
 * @return ESP_ERR_INVALID_ARG if any pointer is NULL or buffer too small
 * @return ESP_ERR_INVALID_SIZE if formatted JSON exceeds buffer capacity
 * 
 * @pre data must point to valid sensor_data_t structure
 * @pre json_buffer must point to writable memory of buffer_size bytes
 * @pre buffer_size must be at least 200 bytes for typical data
 * @post json_buffer contains null-terminated JSON string on success
 * @note Generated JSON is always null-terminated
 * @warning Buffer overflow protection - verify buffer_size is adequate
 */
esp_err_t wifi_manager_format_json(const sensor_data_t* data, char* json_buffer, size_t buffer_size);

#endif // WIFI_MANAGER_H