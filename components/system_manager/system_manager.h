#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "dht11.h"

/**
 * @file system_manager.h
 * @brief Dual-Core System Manager for ESP32 Sensor Monitoring System
 * 
 * Coordinates initialization and operation of all system components using
 * dual-core architecture for optimal performance:
 * - Core 0: DHT11 sensor reading task (dedicated timing-critical operations)
 * - Core 1: WiFi transmission task (independent periodic data sending)
 * - ST7789 Display updates (main thread)
 * 
 * Thread-safe data sharing ensures reliable communication between cores.
 */

/**
 * @brief Shared sensor data structure for inter-task communication
 * 
 * This structure is used to safely share sensor readings between the
 * sensor task (Core 0) and WiFi task (Core 1) using FreeRTOS synchronization.
 */
typedef struct {
    dht11_data_t data;          ///< DHT11 sensor reading data
    uint32_t timestamp;         ///< When the reading was taken (cycles)
    bool has_new_data;          ///< Flag indicating fresh data available
    SemaphoreHandle_t mutex;    ///< Mutex for thread-safe access
} shared_sensor_data_t;

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
 * @brief Start dual-core system operation
 * 
 * Creates and starts separate tasks for sensor reading and WiFi transmission
 * on different CPU cores for optimal performance and independence.
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