#ifdef __cplusplus
extern "C" {
#endif

#ifndef DHT11_H
#define DHT11_H

/*============================================================================*/
/* INCLUDE FILES
 - system and project includes
 - needed interfaces from external units
 - internal and external interfaces from this unit
 */

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>
#include "pinout.h"

/*============================================================================*/
/* EXPORTED CONSTANTS */
/**
 * @file dht11.h
 * @brief DHT11 Temperature and Humidity Sensor Driver for ESP32
 * 
 * Single-wire digital sensor driver with precise timing and error handling.
 * Provides temperature and humidity readings with built-in checksum validation.
 * 
 * GPIO pin assignments are defined in pinout.h for centralized management.
 */

// DHT11 timing constants (in microseconds) - Aggressive tolerances for difficult sensors
#define DHT11_START_LOW_TIME     18000  // 18ms low signal to start
#define DHT11_START_HIGH_TIME    40     // 20-40µs high signal (maximum spec)
#define DHT11_RESPONSE_TIMEOUT   500    // Max time to wait for response (5x increased)
#define DHT11_BIT_TIMEOUT        500    // Max time to wait for bit (5x increased)
#define DHT11_BIT_THRESHOLD      50     // Threshold to distinguish 0/1 bits
#define DHT11_STABILIZATION_MS   200    // Stabilization delay before communication

// DHT11 retry configuration
#define DHT11_MAX_RETRIES        3      // Maximum read attempts
#define DHT11_RETRY_DELAY_MS     500    // Delay between retry attempts (increased)

// DHT11 measurement limits
#define DHT11_TEMP_MIN          0       // Minimum temperature (°C)
#define DHT11_TEMP_MAX          50      // Maximum temperature (°C)  
#define DHT11_HUMIDITY_MIN      20      // Minimum humidity (%)
#define DHT11_HUMIDITY_MAX      95      // Maximum humidity (%)

/*============================================================================*/
/* EXPORTED TYPEDEFINITIONS */
/**
 * @brief DHT11 sensor data structure
 */
typedef struct {
    float temperature;    // Temperature in Celsius
    float humidity;       // Relative humidity in percentage
    bool valid;           // True if data is valid (checksum passed)
} dht11_data_t;

/*============================================================================*/
/* EXPORTED FUNCTIONS */
/**
 * @brief Initialize DHT11 temperature and humidity sensor
 * 
 * Configures GPIO pin for single-wire communication.
 * Sets up pin as open-drain with pull-up resistor.
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t dht11_init(void);
/*----------------------------------------------------------------------------*/
/**
 * @brief Read temperature and humidity from DHT11 sensor
 * 
 * Performs complete communication sequence with DHT11:
 * 1. Sends start signal
 * 2. Waits for sensor response
 * 3. Reads 40 bits of data (humidity + temperature + checksum)
 * 4. Validates checksum
 * 
 * @param data Pointer to structure to store sensor readings
 * @return ESP_OK on successful read, ESP_FAIL on communication error
 */
esp_err_t dht11_read(dht11_data_t* data);
/*----------------------------------------------------------------------------*/
/**
 * @brief Get temperature as formatted string
 * 
 * Formats temperature reading for display purposes.
 * Includes error handling for invalid readings.
 * 
 * @param buffer Output buffer for formatted temperature string
 * @param buffer_size Size of output buffer (minimum 8 characters)
 * @return ESP_OK on success, ESP_INVALID_ARG on invalid parameters
 */
esp_err_t dht11_get_temperature_string(char* buffer, size_t buffer_size);
/*----------------------------------------------------------------------------*/
/**
 * @brief Get humidity as formatted string
 * 
 * Formats humidity reading for display purposes.
 * Includes error handling for invalid readings.
 * 
 * @param buffer Output buffer for formatted humidity string
 * @param buffer_size Size of output buffer (minimum 6 characters)
 * @return ESP_OK on success, ESP_INVALID_ARG on invalid parameters
 */
esp_err_t dht11_get_humidity_string(char* buffer, size_t buffer_size);

#endif // DHT11_H
/*============================================================================*/

#ifdef __cplusplus
}
#endif

/** @} */