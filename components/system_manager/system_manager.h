#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include "esp_err.h"

/**
 * @file system_manager.h
 * @brief System Manager for ESP32 Sensor Monitoring System
 * 
 * Coordinates initialization and operation of all system components:
 * - ST7789 Display
 * - DHT11 Temperature/Humidity Sensor
 * - HC-SR04 Ultrasonic Distance Sensor
 */

/**
 * @brief Initialize all system components
 * 
 * Performs sequential initialization of all hardware components
 * in the correct order with proper error handling.
 * 
 * @return ESP_OK on successful initialization, ESP_FAIL if any component fails
 */
esp_err_t system_init(void);

/**
 * @brief Start system operation
 * 
 * Creates and starts all necessary tasks for system operation.
 * Call this after successful system_init().
 * 
 * @return ESP_OK on successful startup, ESP_FAIL on error
 */
esp_err_t system_start(void);

/**
 * @brief Stop all system operations
 * 
 * Gracefully stops all running tasks and cleans up resources.
 * Useful for system shutdown or restart scenarios.
 * 
 * @return ESP_OK on successful shutdown
 */
esp_err_t system_stop(void);

#endif // SYSTEM_MANAGER_H