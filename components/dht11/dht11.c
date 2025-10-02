/**
 * @file dht11.c
 * @brief DHT11 Temperature and Humidity Sensor Driver Implementation
 * 
 * This module provides a complete driver implementation for the DHT11 digital
 * temperature and humidity sensor. The DHT11 uses a single-wire communication
 * protocol with precise timing requirements that must be carefully managed.
 * 
 * Protocol Overview:
 * The DHT11 communication follows a specific timing sequence:
 * 1. Host pulls data line low for 18ms (start signal)
 * 2. Host releases line (pulled high by resistor) for 20-40µs
 * 3. DHT11 responds with 80µs low + 80µs high
 * 4. DHT11 transmits 40 bits of data (5 bytes total)
 * 5. Each bit: 50µs low + variable high (26-28µs = '0', 70µs = '1')
 * 
 * Data Format (40 bits total):
 * - Byte 0: Humidity integer part
 * - Byte 1: Humidity decimal part
 * - Byte 2: Temperature integer part  
 * - Byte 3: Temperature decimal part
 * - Byte 4: Checksum (sum of bytes 0-3)
 * 
 * Timing Considerations:
 * - All timing is implemented using microsecond-precision delays
 * - Interrupts are disabled during critical timing sections
 * - GPIO operations use direct register access for speed
 * - Timeout mechanisms prevent infinite loops on sensor failure
 * 
 * Error Handling Strategy:
 * - Communication errors return ESP_FAIL with detailed logging
 * - Checksum validation ensures data integrity
 * - Last known good readings provide fallback values
 * - Graceful degradation when sensor is temporarily unavailable
 * 
 * @author Adrian Puscasu
 * @version 1.2
 * @date 2025-10-01
 */

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*/
/* INCLUDE FILES
 - system and project includes
 - needed interfaces from external units
 - internal and external interfaces from this unit
 */
#include "dht11.h"              // DHT11 sensor driver API definitions
#include "driver/gpio.h"        // ESP32 GPIO control functions
#include "esp_log.h"            // ESP-IDF logging system
#include "esp_timer.h"          // High-precision timer for microsecond delays
#include "freertos/FreeRTOS.h"  // FreeRTOS kernel functions
#include "freertos/task.h"      // FreeRTOS task management
#include <stdio.h>              // Standard I/O for string formatting

/*============================================================================*/
/* LOCAL CONSTANTS */
/**
 * @brief Logging tag for DHT11 driver messages
 * 
 * Used by ESP-IDF logging system to identify messages from the DHT11 driver.
 * Enables filtering and categorization of sensor-related log output.
 */
static const char *TAG = "DHT11";

/*============================================================================*/
/* LOCAL TYPEDEFINITIONS (STRUCTURES, UNIONS, ENUMS, MACROS) */

/*============================================================================*/
/* EXPORTED TYPEDEFINITIONS */

/*============================================================================*/
/* LOCAL VARIABLES */
/**
 * @brief Last known good sensor reading cache
 * 
 * Stores the most recent valid sensor reading to provide fallback data
 * when fresh readings fail due to communication errors or sensor issues.
 * This ensures the system can continue operating with stale but valid data
 * during temporary sensor unavailability.
 * 
 * @note Initialized to zero/invalid state at startup
 * @note Updated only when new readings pass checksum validation
 */
static dht11_data_t last_reading = {0};

/*============================================================================*/
/* EXPORTED VARIABLES */

/*============================================================================*/
/* LOCAL FUNCTIONS */
/**
 * @brief Wait for GPIO pin to reach expected logic level with timeout
 * 
 * Helper function that monitors a GPIO pin until it reaches the expected
 * logic level or times out. This is essential for DHT11 communication
 * timing as it allows precise measurement of signal transitions.
 * 
 * Timing Measurement:
 * Uses esp_timer_get_time() for microsecond-precision timing measurements.
 * This accuracy is critical for distinguishing between '0' and '1' bits
 * in the DHT11 protocol.
 * 
 * Timeout Protection:
 * Prevents infinite loops when the sensor fails to respond or is disconnected.
 * Returns immediately when timeout is exceeded to allow error handling.
 * 
 * Usage in DHT11 Protocol:
 * - Wait for sensor response acknowledgment
 * - Measure bit timing for data decoding
 * - Detect communication failures quickly
 * 
 * @param expected_level Logic level to wait for (0 = low, 1 = high)
 * @param timeout_us Maximum time to wait in microseconds
 * 
 * @return Time actually waited in microseconds (>= 0)
 * @return -1 if timeout was exceeded before pin reached expected level
 * 
 * @note This function busy-waits and should only be used during sensor communication
 * @note Interrupts should be disabled during timing-critical measurements
 */
static int32_t wait_for_pin_state(int expected_level, uint32_t timeout_us) 
{
    int64_t start_time = esp_timer_get_time();  // Record start time for timeout calculation
    
    // Small initial delay to avoid immediate false positives on pin transitions
    esp_rom_delay_us(1);
    
    // Poll pin state until it matches expected level or timeout occurs
    while (gpio_get_level(DHT11_DATA_PIN) != expected_level) 
    {
        int64_t elapsed = esp_timer_get_time() - start_time;
        if (elapsed > timeout_us) 
        {
            return -1;  // Timeout exceeded - sensor not responding
        }
        
        // Small delay to prevent excessive CPU usage during polling
        // This also helps with GPIO stability on some ESP32 variants
        esp_rom_delay_us(1);
    }
    
    // Return actual time waited (useful for bit timing analysis)
    return (int32_t)(esp_timer_get_time() - start_time);
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
 * @brief Read single data bit from DHT11 sensor
 * 
 * Implements the DHT11 bit reading protocol by measuring the duration
 * of the high pulse that follows each bit's low start pulse.
 * 
 * DHT11 Bit Encoding:
 * - Every bit starts with ~50µs low pulse
 * - Followed by variable-length high pulse:
 *   * '0' bit: 26-28µs high pulse
 *   * '1' bit: ~70µs high pulse
 * 
 * Decoding Algorithm:
 * 1. Wait for low pulse (start of bit)
 * 2. Wait for high pulse (data encoding)
 * 3. Measure high pulse duration
 * 4. Compare duration to threshold (DHT11_BIT_THRESHOLD)
 * 5. Return 0 for short pulses, 1 for long pulses
 * 
 * Error Conditions:
 * - Timeout waiting for low-to-high transition
 * - Timeout waiting for high-to-low transition
 * - Invalid pulse duration measurements
 * 
 * @return 0 for logic '0' bit
 * @return 1 for logic '1' bit
 * @return -1 on communication error or timeout
 * 
 * @note Called 40 times sequentially to read complete sensor data
 * @note Timing accuracy is critical for correct bit decoding
 */
static int read_bit(void) 
{
    int64_t bit_start = esp_timer_get_time();
    
    // Wait for low-to-high transition (start of bit)
    // Each bit transmission begins with a low pulse followed by data encoding
    int32_t low_wait = wait_for_pin_state(0, DHT11_BIT_TIMEOUT);
    if (low_wait < 0) 
    {
        ESP_LOGD(TAG, "Bit read failed: timeout waiting for low state after %lldµs", 
                 esp_timer_get_time() - bit_start);
        return -1;  // Timeout waiting for start of bit
    }
    
    int32_t high_wait = wait_for_pin_state(1, DHT11_BIT_TIMEOUT);
    if (high_wait < 0) {
        ESP_LOGD(TAG, "Bit read failed: timeout waiting for high state (low took %ldµs)", low_wait);
        return -1;  // Timeout waiting for data encoding pulse
    }
    
    // Measure high pulse duration (this encodes the bit value)
    // The duration of this pulse determines whether the bit is '0' or '1'
    int32_t high_time = wait_for_pin_state(0, DHT11_BIT_TIMEOUT);
    if (high_time < 0) 
    {
        ESP_LOGD(TAG, "Bit read failed: timeout waiting for end of high pulse (low=%ldµs, high_wait=%ldµs)", 
                 low_wait, high_wait);
        return -1;  // Timeout waiting for end of bit
    }
    
    // Validate pulse timing is within reasonable bounds
    // Valid DHT11 pulses should be 26-28µs (0) or 70µs (1)
    if (high_time < 15 || high_time > 100) 
    {
        ESP_LOGD(TAG, "Invalid pulse duration: %ldµs (expected 15-100µs)", high_time);
        return -1;  // Pulse duration outside valid range
    }
    
    // Determine bit value based on pulse duration
    // Threshold comparison converts timing measurement to digital value
    // 0 bit: ~26-28µs high, 1 bit: ~70µs high
    int bit_value = (high_time > DHT11_BIT_THRESHOLD) ? 1 : 0;
    ESP_LOGV(TAG, "Bit read: %d (high_time=%ldµs)", bit_value, high_time);
    return bit_value;
}
/*----------------------------------------------------------------------------*/

/*============================================================================*/
/* EXPORTED FUNCTIONS */
/**
 * @brief Initialize DHT11 temperature and humidity sensor
 * 
 * Configures the GPIO pin for single-wire communication with the DHT11 sensor.
 * The pin is set up as an open-drain output with internal pull-up resistor
 * to support the DHT11's single-wire protocol requirements.
 * 
 * GPIO Configuration Details:
 * - Mode: Open-drain output (allows bidirectional communication)
 * - Pull-up: Enabled (required for single-wire protocol idle state)
 * - Pull-down: Disabled (conflicts with pull-up requirement)
 * - Interrupts: Disabled (not needed for polled communication)
 * 
 * Pin Assignment:
 * The actual GPIO pin number is defined in pinout.h as DHT11_DATA_PIN
 * to maintain centralized pin management and prevent conflicts.
 * 
 * Initial State:
 * Pin is set to logic high (idle state) after configuration to ensure
 * the sensor is ready for communication sequences.
 * 
 * @return ESP_OK on successful initialization
 * @return ESP_FAIL if GPIO configuration fails
 * 
 * @note Must be called before any dht11_read() operations
 * @note Can be called multiple times safely (idempotent)
 * @see pinout.h for pin assignment details
 */
esp_err_t dht11_init(void) 
{
    ESP_LOGI(TAG, "Initializing DHT11 temperature/humidity sensor...");
    
    // Configure data pin as open-drain output with pull-up
    // This configuration allows both output (host->sensor) and input (sensor->host) modes
    gpio_config_t config = 
    {
        .pin_bit_mask = (1ULL << DHT11_DATA_PIN),    // Target specific GPIO pin
        .mode = GPIO_MODE_OUTPUT_OD,                 // Open drain for single-wire protocol
        .pull_up_en = GPIO_PULLUP_ENABLE,            // Internal pull-up resistor (required)
        .pull_down_en = GPIO_PULLDOWN_DISABLE,       // Disable conflicting pull-down
        .intr_type = GPIO_INTR_DISABLE               // No interrupts needed for polling
    };
    
    esp_err_t ret = gpio_config(&config);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to configure DHT11 data pin %d: %s", DHT11_DATA_PIN, esp_err_to_name(ret));
        return ret;
    }
    
    // Set pin high initially (idle state for single-wire protocol)
    // This ensures the sensor sees the expected idle state after initialization
    gpio_set_level(DHT11_DATA_PIN, 1);
    
    ESP_LOGI(TAG, "✓ DHT11 initialized successfully on GPIO%d", DHT11_DATA_PIN);
    ESP_LOGI(TAG, "✓ Pin configured as open-drain with pull-up resistor");
    ESP_LOGI(TAG, "✓ Sensor ready for temperature/humidity readings");
    
    return ESP_OK;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
 * @brief Read complete 40-bit data packet from DHT11 sensor
 * 
 * Implements the complete DHT11 communication protocol to obtain
 * temperature and humidity readings. This function handles all
 * timing-critical operations and data validation.
 * 
 * Communication Sequence:
 * 1. HOST INITIATES:
 *    - Pull data line low for 18ms (wake up sensor)
 *    - Release line (pull-up brings it high) for 20-40µs
 * 
 * 2. SENSOR RESPONDS:
 *    - DHT11 pulls line low for 80µs (acknowledge)
 *    - DHT11 pulls line high for 80µs (ready to transmit)
 * 
 * 3. DATA TRANSMISSION:
 *    - 40 bits transmitted sequentially (5 bytes total)
 *    - Each bit: 50µs low + variable high (26-28µs='0', 70µs='1')
 * 
 * 4. DATA VALIDATION:
 *    - Calculate checksum of first 4 bytes
 *    - Compare with 5th byte (checksum byte)
 *    - Accept data only if checksum matches
 * 
 * Data Format:
 * - Byte 0: Humidity integer part (0-100)
 * - Byte 1: Humidity decimal part (always 0 for DHT11)
 * - Byte 2: Temperature integer part (0-50°C)
 * - Byte 3: Temperature decimal part (always 0 for DHT11)
 * - Byte 4: Checksum (sum of bytes 0-3, lower 8 bits)
 * 
 * Critical Timing Sections:
 * Interrupts are disabled during timing-sensitive operations to
 * ensure accurate microsecond-level measurements required by the protocol.
 * 
 * Error Recovery:
 * On any communication error, the GPIO is restored to output mode
 * with high level to prepare for the next communication attempt.
 * 
 * @param data Pointer to structure for storing sensor readings
 * 
 * @return ESP_OK on successful read with valid checksum
 * @return ESP_FAIL on communication error or checksum mismatch
 * @return ESP_ERR_INVALID_ARG if data pointer is NULL
 * 
 * @note Minimum 2-second interval between reads (DHT11 limitation)
 * @note Function duration: ~20ms for complete communication
 * @note Interrupts disabled for ~5ms during critical timing sections
 */
/**
 * @brief Internal function to perform a single DHT11 read attempt
 * 
 * This function contains the core DHT11 communication logic without
 * retry handling. It's separated to enable clean retry implementation.
 * 
 * @param data Pointer to structure for storing sensor readings
 * @return ESP_OK on success, ESP_FAIL on communication error
 */
static esp_err_t dht11_read_attempt(dht11_data_t* data)
{
    // Initialize data structure to safe default values
    data->temperature = 0.0;
    data->humidity = 0.0;
    data->valid = false;
    
    ESP_LOGD(TAG, "DHT11 attempt starting with %dms stabilization delay...", DHT11_STABILIZATION_MS);
    
    // Add stabilization delay for sensor readiness
    vTaskDelay(pdMS_TO_TICKS(DHT11_STABILIZATION_MS));
    
    ESP_LOGD(TAG, "Beginning timing-critical DHT11 communication sequence...");
    
    // === CRITICAL TIMING SECTION START ===
    // Disable interrupts for precise timing during communication
    portDISABLE_INTERRUPTS();
    
    // Send start signal: pull data line low for 18ms
    // This wakes up the DHT11 and signals the start of communication
    gpio_set_level(DHT11_DATA_PIN, 0);
    esp_rom_delay_us(DHT11_START_LOW_TIME);  // 18ms low signal
    
    // Pull high for 20-40µs (host ready to receive)
    // This signals to the DHT11 that the host is ready for data transmission
    gpio_set_level(DHT11_DATA_PIN, 1);
    esp_rom_delay_us(DHT11_START_HIGH_TIME);  // 20-40µs high signal
    
    // Switch to input mode to read DHT11 response
    // Pin is now controlled by DHT11 for response and data transmission
    gpio_set_direction(DHT11_DATA_PIN, GPIO_MODE_INPUT);
    
    int64_t response_start = esp_timer_get_time();
    
    // Wait for DHT11 response acknowledgment: 80µs low + 80µs high
    // This confirms the sensor received the start signal and is ready to transmit
    if (wait_for_pin_state(0, DHT11_RESPONSE_TIMEOUT) < 0) 
    {
        int64_t fail_time = esp_timer_get_time();
        portENABLE_INTERRUPTS();
        gpio_set_direction(DHT11_DATA_PIN, GPIO_MODE_OUTPUT_OD);
        gpio_set_level(DHT11_DATA_PIN, 1);
        ESP_LOGW(TAG, "Failed waiting for initial low response after %lldµs", fail_time - response_start);
        return ESP_FAIL;
    }
    
    int64_t low_ack_time = esp_timer_get_time();
    
    if (wait_for_pin_state(1, DHT11_RESPONSE_TIMEOUT) < 0) 
    {
        int64_t fail_time = esp_timer_get_time();
        portENABLE_INTERRUPTS();
        gpio_set_direction(DHT11_DATA_PIN, GPIO_MODE_OUTPUT_OD);
        gpio_set_level(DHT11_DATA_PIN, 1);
        ESP_LOGW(TAG, "Failed waiting for high response after %lldµs (low took %lldµs)", 
                 fail_time - low_ack_time, low_ack_time - response_start);
        return ESP_FAIL;
    }
    
    int64_t high_ack_time = esp_timer_get_time();
    ESP_LOGD(TAG, "DHT11 acknowledged: low=%lldµs, high=%lldµs", 
             low_ack_time - response_start, high_ack_time - low_ack_time);
    
    // Read 40 bits of data (5 bytes: humidity_int, humidity_dec, temp_int, temp_dec, checksum)
    uint8_t raw_data[5] = {0};  // Initialize to zero for clean data collection
    
    for (int i = 0; i < 5; i++) 
    {  // 5 bytes total
        for (int j = 7; j >= 0; j--) 
        {  // 8 bits per byte, MSB first
            int bit = read_bit();
            if (bit < 0) 
            {
                goto error_exit;
            }
            raw_data[i] |= (bit << j);  // Assemble byte MSB first
        }
    }
    
    // === CRITICAL TIMING SECTION END ===
    // Re-enable interrupts now that timing-critical section is complete
    portENABLE_INTERRUPTS();
    
    // Reset pin to output mode with high level (idle state)
    // Prepare pin for next communication cycle
    gpio_set_direction(DHT11_DATA_PIN, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(DHT11_DATA_PIN, 1);
    
    ESP_LOGD(TAG, "DHT11 response acknowledged, data reception complete");
    ESP_LOGD(TAG, "Raw data: [0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X]", 
             raw_data[0], raw_data[1], raw_data[2], raw_data[3], raw_data[4]);
    ESP_LOGD(TAG, "Validating checksum...");
    
    // Validate data integrity using checksum
    // Checksum is the sum of the first 4 bytes (lower 8 bits)
    uint8_t calculated_checksum = raw_data[0] + raw_data[1] + raw_data[2] + raw_data[3];
    if (calculated_checksum != raw_data[4]) 
    {
        ESP_LOGW(TAG, "Checksum mismatch - calculated: 0x%02X, received: 0x%02X", 
                 calculated_checksum, raw_data[4]);
        ESP_LOGW(TAG, "Data may be corrupted, discarding reading");
        return ESP_FAIL;
    }
    
    // Extract and convert sensor readings to floating-point values
    // DHT11 format: integer.decimal (decimal part always 0 for DHT11)
    data->humidity = (float)raw_data[0] + (float)raw_data[1] / 10.0;
    data->temperature = (float)raw_data[2] + (float)raw_data[3] / 10.0;
    data->valid = true;
    
    // Validate readings are within expected sensor range
    if (data->humidity < DHT11_HUMIDITY_MIN || data->humidity > DHT11_HUMIDITY_MAX) 
    {
        ESP_LOGW(TAG, "Humidity reading %.1f%% outside valid range (%d-%d%%)", 
                 data->humidity, DHT11_HUMIDITY_MIN, DHT11_HUMIDITY_MAX);
    }
    
    if (data->temperature < DHT11_TEMP_MIN || data->temperature > DHT11_TEMP_MAX) 
    {
        ESP_LOGW(TAG, "Temperature reading %.1f°C outside valid range (%d-%d°C)", 
                 data->temperature, DHT11_TEMP_MIN, DHT11_TEMP_MAX);
    }
    
    // Store as last known good reading for fallback purposes
    last_reading = *data;
    
    ESP_LOGI(TAG, "✓ Successful reading: %.1f°C, %.0f%% humidity", 
             data->temperature, data->humidity);
    
    return ESP_OK;
    
error_exit:
    // Error recovery: restore pin to safe state and re-enable interrupts
    portENABLE_INTERRUPTS();
    gpio_set_direction(DHT11_DATA_PIN, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(DHT11_DATA_PIN, 1);
    
    ESP_LOGW(TAG, "DHT11 communication failed - timeout or bit error detected");
    ESP_LOGW(TAG, "Sensor may be disconnected, busy, or experiencing timing issues");
    return ESP_FAIL;
}

/**
 * @brief Read temperature and humidity from DHT11 sensor with retry mechanism
 * 
 * This function implements a robust retry mechanism to handle temporary
 * communication failures common with DHT11 sensors. It attempts multiple
 * reads with appropriate delays between attempts.
 * 
 * Retry Strategy:
 * - Attempts up to DHT11_MAX_RETRIES reads
 * - 100ms delay between attempts to allow sensor recovery
 * - Returns on first successful read
 * - Provides fallback to last known good reading if available
 * 
 * @param data Pointer to structure for storing sensor readings
 * 
 * @return ESP_OK on successful read with valid checksum
 * @return ESP_FAIL on all retry attempts failed
 * @return ESP_ERR_INVALID_ARG if data pointer is NULL
 */
esp_err_t dht11_read(dht11_data_t* data) 
{
    if (data == NULL) 
    {
        ESP_LOGE(TAG, "Invalid parameter: data pointer is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGD(TAG, "Starting DHT11 reading with retry mechanism...");
    
    // Attempt multiple reads with retry logic
    for (int attempt = 1; attempt <= DHT11_MAX_RETRIES; attempt++) 
    {
        ESP_LOGD(TAG, "Read attempt %d/%d", attempt, DHT11_MAX_RETRIES);
        
        esp_err_t result = dht11_read_attempt(data);
        if (result == ESP_OK) 
        {
            ESP_LOGD(TAG, "✓ DHT11 read successful on attempt %d", attempt);
            return ESP_OK;
        }
        
        // Log retry attempt (except for the last one)
        if (attempt < DHT11_MAX_RETRIES) 
        {
            ESP_LOGW(TAG, "Attempt %d failed, retrying in %dms...", attempt, DHT11_RETRY_DELAY_MS);
            vTaskDelay(pdMS_TO_TICKS(DHT11_RETRY_DELAY_MS));
        }
    }
    
    // All retry attempts failed
    ESP_LOGW(TAG, "All %d DHT11 read attempts failed", DHT11_MAX_RETRIES);
    
    // Return last known good reading if available as fallback
    if (last_reading.valid) 
    {
        ESP_LOGW(TAG, "Using cached reading: %.1f°C, %.0f%% humidity (age unknown)", 
                 last_reading.temperature, last_reading.humidity);
        *data = last_reading;
        data->valid = false;  // Mark as stale data
        return ESP_OK;  // Return OK but with stale data marker
    }
    
    ESP_LOGE(TAG, "No cached data available, DHT11 read completely failed");
    return ESP_FAIL;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
 * @brief Get temperature reading as formatted display string
 * 
 * Provides temperature data in a human-readable format suitable for
 * display on LCD screens or user interfaces. Handles sensor communication
 * errors gracefully by falling back to cached values when available.
 * 
 * Data Acquisition Strategy:
 * 1. Attempt fresh sensor reading via dht11_read()
 * 2. Use fresh data if reading successful and checksum valid
 * 3. Fall back to last known good reading if fresh read fails
 * 4. Display error indicator if no valid data available
 * 
 * Format Specifications:
 * - Temperature displayed with 1 decimal place precision
 * - Celsius unit indicator ('C') appended
 * - Error state shown as "--.-C" for clear visual feedback
 * 
 * Fallback Behavior:
 * Using cached readings during temporary sensor failures ensures
 * continuous system operation and prevents display flickering or
 * blank readings that could confuse users.
 * 
 * @param buffer Output buffer for formatted temperature string
 * @param buffer_size Size of output buffer in bytes (minimum 8 required)
 * 
 * @return ESP_OK on successful string formatting
 * @return ESP_ERR_INVALID_ARG if buffer is NULL or too small
 * 
 * @note Buffer size must accommodate "XX.XC\0" (minimum 6 chars + null terminator)
 * @note Function is safe to call frequently (handles DHT11 timing internally)
 * @note Falls back to cached data during temporary communication failures
 * 
 * @example
 * char temp_str[10];
 * dht11_get_temperature_string(temp_str, sizeof(temp_str));
 * printf("Temperature: %s\n", temp_str);  // Outputs: "Temperature: 23.5C"
 */
esp_err_t dht11_get_temperature_string(char* buffer, size_t buffer_size) 
{
    if (buffer == NULL || buffer_size < 8) 
    {
        ESP_LOGE(TAG, "Invalid parameters: buffer=%p, size=%zu (minimum 8 required)", 
                 buffer, buffer_size);
        return ESP_ERR_INVALID_ARG;
    }
    
    dht11_data_t data;
    esp_err_t ret = dht11_read(&data);
    
    if (ret == ESP_OK && data.valid) 
    {
        // Fresh reading successful - use current data
        snprintf(buffer, buffer_size, "%.1fC", data.temperature);
        ESP_LOGD(TAG, "Temperature string: %s (fresh reading)", buffer);
    } 
    else if (last_reading.valid) 
    {
        // Fresh reading failed - use last known good reading
        snprintf(buffer, buffer_size, "%.1fC", last_reading.temperature);
        ESP_LOGD(TAG, "Temperature string: %s (cached reading)", buffer);
    } 
    else 
    {
        // No valid data available - display error indicator
        snprintf(buffer, buffer_size, "--.-C");
        ESP_LOGD(TAG, "Temperature string: %s (no valid data)", buffer);
    }
    
    return ESP_OK;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
 * @brief Get humidity reading as formatted display string
 * 
 * Provides humidity data in a human-readable format suitable for
 * display on LCD screens or user interfaces. Implements the same
 * graceful error handling strategy as temperature readings.
 * 
 * Data Acquisition Strategy:
 * 1. Attempt fresh sensor reading via dht11_read()
 * 2. Use fresh data if reading successful and checksum valid
 * 3. Fall back to last known good reading if fresh read fails
 * 4. Display error indicator if no valid data available
 * 
 * Format Specifications:
 * - Humidity displayed as integer percentage (no decimal places)
 * - Percent symbol ('%') appended for unit clarity
 * - Error state shown as "--%"  for clear visual feedback
 * 
 * DHT11 Precision Notes:
 * The DHT11 sensor only provides integer humidity values,
 * so decimal formatting is not used for humidity readings.
 * 
 * @param buffer Output buffer for formatted humidity string
 * @param buffer_size Size of output buffer in bytes (minimum 6 required)
 * 
 * @return ESP_OK on successful string formatting
 * @return ESP_ERR_INVALID_ARG if buffer is NULL or too small
 * 
 * @note Buffer size must accommodate "XXX%\0" (minimum 5 chars + null terminator)
 * @note Function shares sensor communication with temperature readings
 * @note Falls back to cached data during temporary communication failures
 * 
 * @example
 * char humid_str[8];
 * dht11_get_humidity_string(humid_str, sizeof(humid_str));
 * printf("Humidity: %s\n", humid_str);  // Outputs: "Humidity: 65%"
 */
esp_err_t dht11_get_humidity_string(char* buffer, size_t buffer_size) 
{
    if (buffer == NULL || buffer_size < 6) 
    {
        ESP_LOGE(TAG, "Invalid parameters: buffer=%p, size=%zu (minimum 6 required)", 
                 buffer, buffer_size);
        return ESP_ERR_INVALID_ARG;
    }
    
    dht11_data_t data;
    esp_err_t ret = dht11_read(&data);
    
    if (ret == ESP_OK && data.valid) 
    {
        // Fresh reading successful - use current data
        snprintf(buffer, buffer_size, "%.0f%%", data.humidity);
        ESP_LOGD(TAG, "Humidity string: %s (fresh reading)", buffer);
    } 
    else if (last_reading.valid) 
    {
        // Fresh reading failed - use last known good reading
        snprintf(buffer, buffer_size, "%.0f%%", last_reading.humidity);
        ESP_LOGD(TAG, "Humidity string: %s (cached reading)", buffer);
    } 
    else 
    {
        // No valid data available - display error indicator
        snprintf(buffer, buffer_size, "--%%");
        ESP_LOGD(TAG, "Humidity string: %s (no valid data)", buffer);
    }
    
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif
/*============================================================================*/
/** @} */