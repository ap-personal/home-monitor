#include "dht11.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

static const char *TAG = "DHT11";
static dht11_data_t last_reading = {0};

/**
 * @brief Initialize DHT11 temperature and humidity sensor
 * 
 * Configures GPIO pin for single-wire communication with DHT11.
 * Sets up open-drain output with pull-up for reliable communication.
 * 
 * @return ESP_OK on successful initialization, ESP_FAIL on error
 */
esp_err_t dht11_init(void) {
    ESP_LOGI(TAG, "Initializing DHT11 temperature/humidity sensor...");
    
    // Configure data pin as open-drain output with pull-up
    gpio_config_t config = {
        .pin_bit_mask = (1ULL << DHT11_DATA_PIN),
        .mode = GPIO_MODE_OUTPUT_OD,  // Open drain for single-wire protocol
        .pull_up_en = GPIO_PULLUP_ENABLE,  // Internal pull-up resistor
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    esp_err_t ret = gpio_config(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure DHT11 data pin %d", DHT11_DATA_PIN);
        return ret;
    }
    
    // Set pin high initially (idle state)
    gpio_set_level(DHT11_DATA_PIN, 1);
    
    ESP_LOGI(TAG, "DHT11 initialized on GPIO%d", DHT11_DATA_PIN);
    
    return ESP_OK;
}

/**
 * @brief Wait for pin state change with timeout
 * 
 * Helper function to wait for GPIO pin to reach expected state.
 * Used during DHT11 communication protocol.
 * 
 * @param expected_level Expected pin level (0 or 1)
 * @param timeout_us Timeout in microseconds
 * @return Time waited in microseconds, or -1 on timeout
 */
static int32_t wait_for_pin_state(int expected_level, uint32_t timeout_us) {
    int64_t start_time = esp_timer_get_time();
    
    while (gpio_get_level(DHT11_DATA_PIN) != expected_level) {
        int64_t elapsed = esp_timer_get_time() - start_time;
        if (elapsed > timeout_us) {
            return -1;  // Timeout
        }
    }
    
    return (int32_t)(esp_timer_get_time() - start_time);
}

/**
 * @brief Read single bit from DHT11
 * 
 * Reads one bit of data during DHT11 communication.
 * Timing analysis determines if bit is 0 or 1.
 * 
 * @return Bit value (0 or 1), or -1 on error
 */
static int read_bit(void) {
    // Wait for low-to-high transition (start of bit)
    if (wait_for_pin_state(0, DHT11_BIT_TIMEOUT) < 0) {
        return -1;
    }
    if (wait_for_pin_state(1, DHT11_BIT_TIMEOUT) < 0) {
        return -1;
    }
    
    // Measure high pulse duration
    int32_t high_time = wait_for_pin_state(0, DHT11_BIT_TIMEOUT);
    if (high_time < 0) {
        return -1;
    }
    
    // Determine bit value based on pulse duration
    // 0 bit: ~26-28µs high, 1 bit: ~70µs high
    return (high_time > DHT11_BIT_THRESHOLD) ? 1 : 0;
}

/**
 * @brief Read complete 40-bit data from DHT11
 * 
 * Implements the DHT11 communication protocol:
 * 1. Send start signal (18ms low + 20µs high)
 * 2. Wait for DHT11 response (80µs low + 80µs high)
 * 3. Read 40 bits of data
 * 4. Validate checksum
 * 
 * @param data Pointer to structure to store sensor readings
 * @return ESP_OK on successful read, ESP_FAIL on communication/checksum error
 */
esp_err_t dht11_read(dht11_data_t* data) {
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Initialize data structure
    data->temperature = 0.0;
    data->humidity = 0.0;
    data->valid = false;
    
    // Disable interrupts for precise timing
    portDISABLE_INTERRUPTS();
    
    // Send start signal: 18ms low
    gpio_set_level(DHT11_DATA_PIN, 0);
    esp_rom_delay_us(DHT11_START_LOW_TIME);
    
    // Pull high for 20-40µs
    gpio_set_level(DHT11_DATA_PIN, 1);
    esp_rom_delay_us(DHT11_START_HIGH_TIME);
    
    // Switch to input mode to read response
    gpio_set_direction(DHT11_DATA_PIN, GPIO_MODE_INPUT);
    
    // Wait for DHT11 response: 80µs low + 80µs high
    if (wait_for_pin_state(0, DHT11_RESPONSE_TIMEOUT) < 0) {
        goto error_exit;
    }
    
    if (wait_for_pin_state(1, DHT11_RESPONSE_TIMEOUT) < 0) {
        goto error_exit;
    }
    
    // Read 40 bits of data
    uint8_t raw_data[5] = {0};  // 5 bytes: humidity_int, humidity_dec, temp_int, temp_dec, checksum
    
    for (int i = 0; i < 5; i++) {
        for (int j = 7; j >= 0; j--) {
            int bit = read_bit();
            if (bit < 0) {
                goto error_exit;
            }
            raw_data[i] |= (bit << j);
        }
    }
    
    // Re-enable interrupts
    portENABLE_INTERRUPTS();
    
    // Reset pin to output mode
    gpio_set_direction(DHT11_DATA_PIN, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(DHT11_DATA_PIN, 1);
    
    // Validate checksum
    uint8_t checksum = raw_data[0] + raw_data[1] + raw_data[2] + raw_data[3];
    if (checksum != raw_data[4]) {
        return ESP_FAIL;
    }
    
    // Extract humidity and temperature
    data->humidity = (float)raw_data[0] + (float)raw_data[1] / 10.0;
    data->temperature = (float)raw_data[2] + (float)raw_data[3] / 10.0;
    data->valid = true;
    
    // Store as last known good reading
    last_reading = *data;
    
    return ESP_OK;
    
error_exit:
    portENABLE_INTERRUPTS();
    gpio_set_direction(DHT11_DATA_PIN, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(DHT11_DATA_PIN, 1);
    return ESP_FAIL;
}

/**
 * @brief Get temperature reading as formatted string
 * 
 * Attempts fresh reading, falls back to last known good value on error.
 * Formats temperature with appropriate precision for display.
 * 
 * @param buffer Output buffer for formatted temperature string
 * @param buffer_size Size of output buffer (minimum 8 characters)
 * @return ESP_OK on success, ESP_INVALID_ARG on invalid parameters
 */
esp_err_t dht11_get_temperature_string(char* buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size < 8) {
        return ESP_ERR_INVALID_ARG;
    }
    
    dht11_data_t data;
    esp_err_t ret = dht11_read(&data);
    
    if (ret == ESP_OK && data.valid) {
        snprintf(buffer, buffer_size, "%.1fC", data.temperature);
    } else if (last_reading.valid) {
        // Use last known good reading
        snprintf(buffer, buffer_size, "%.1fC", last_reading.temperature);
    } else {
        snprintf(buffer, buffer_size, "--.-C");
    }
    
    return ESP_OK;
}

/**
 * @brief Get humidity reading as formatted string
 * 
 * Attempts fresh reading, falls back to last known good value on error.
 * Formats humidity with appropriate precision for display.
 * 
 * @param buffer Output buffer for formatted humidity string
 * @param buffer_size Size of output buffer (minimum 6 characters)
 * @return ESP_OK on success, ESP_INVALID_ARG on invalid parameters
 */
esp_err_t dht11_get_humidity_string(char* buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size < 6) {
        return ESP_ERR_INVALID_ARG;
    }
    
    dht11_data_t data;
    esp_err_t ret = dht11_read(&data);
    
    if (ret == ESP_OK && data.valid) {
        snprintf(buffer, buffer_size, "%.0f%%", data.humidity);
    } else if (last_reading.valid) {
        // Use last known good reading
        snprintf(buffer, buffer_size, "%.0f%%", last_reading.humidity);
    } else {
        snprintf(buffer, buffer_size, "--%%");
    }
    
    return ESP_OK;
}