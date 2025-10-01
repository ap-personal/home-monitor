#include "system_manager.h"
#include "st7789.h"
#include "dht11.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "SYSTEM_MANAGER";
static TaskHandle_t sensor_task_handle = NULL;

/**
 * @brief Display startup screen
 * 
 * Shows system information and initialization status on the display.
 * Provides visual feedback during system startup.
 */
static void display_startup_screen(void) {
    st7789_clear_screen(ST7789_BLACK);
    st7789_draw_large_string(20, 60, "SYSTEM", ST7789_CYAN, ST7789_BLACK);
    st7789_draw_large_string(20, 100, "READY", ST7789_GREEN, ST7789_BLACK);
    vTaskDelay(pdMS_TO_TICKS(2000));
}

/**
 * @brief Sensor monitoring task
 * 
 * Continuously reads all sensors and updates the display.
 * Runs independently without blocking other system operations.
 * Includes proper error handling and recovery mechanisms.
 * 
 * @param pvParameters Task parameters (unused)
 */
static void sensor_monitoring_task(void *pvParameters) {
    char temp_str[16], humidity_str[16];
    uint32_t cycle_count = 0;
    
    ESP_LOGI(TAG, "Sensor monitoring task started");
    
    // Give system time to stabilize after initialization
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    while (1) {
        // Initialize strings with safe defaults
        strcpy(temp_str, "--.-C");
        strcpy(humidity_str, "--%");
        
        // Read DHT11 every cycle (every 3 seconds)
        ESP_LOGD(TAG, "Reading DHT11 sensors...");
        dht11_get_temperature_string(temp_str, sizeof(temp_str));
        dht11_get_humidity_string(humidity_str, sizeof(humidity_str));
        
        ESP_LOGD(TAG, "Updating display...");
        
        // Update display with error protection
        st7789_clear_screen(ST7789_BLACK);
        
        // Temperature - centered on screen
        st7789_draw_large_string(10, 80, "TEMP:", ST7789_WHITE, ST7789_BLACK);
        st7789_draw_large_string(120, 80, temp_str, ST7789_RED, ST7789_BLACK);
        
        // Humidity - centered on screen
        st7789_draw_large_string(10, 140, "HUMD:", ST7789_WHITE, ST7789_BLACK);
        st7789_draw_large_string(120, 140, humidity_str, ST7789_BLUE, ST7789_BLACK);
        
        ESP_LOGI(TAG, "Sensors - Temp: %s, Humidity: %s", 
                 temp_str, humidity_str);
        
        cycle_count++;
        
        // Update interval - DHT11 needs at least 2 seconds between reads
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/**
 * @brief Initialize all system components
 * 
 * Sequentially initializes all hardware components with proper error handling.
 * Initialization order is important for some components.
 * 
 * @return ESP_OK on successful initialization, ESP_FAIL if any component fails
 */
esp_err_t system_init(void) {
    ESP_LOGI(TAG, "Starting system initialization...");
    
    // Initialize display first (for status feedback)
    ESP_LOGI(TAG, "Initializing display...");
    esp_err_t ret = st7789_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Display initialization failed: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Display initialized successfully");
    
    // Initialize DHT11 sensor
    ESP_LOGI(TAG, "Initializing DHT11 sensor...");
    ret = dht11_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "DHT11 initialization failed: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "DHT11 sensor initialized successfully");
    
    ESP_LOGI(TAG, "System initialization completed successfully");
    return ESP_OK;
}

/**
 * @brief Start system operation
 * 
 * Creates and starts all necessary tasks for normal system operation.
 * Should be called after successful system_init().
 * 
 * @return ESP_OK on successful startup, ESP_FAIL on error
 */
esp_err_t system_start(void) {
    ESP_LOGI(TAG, "Starting system operation...");
    
    // Show startup screen
    display_startup_screen();
    
    // Create sensor monitoring task
    BaseType_t task_created = xTaskCreate(
        sensor_monitoring_task,
        "sensor_monitor",
        8192,  // Increased stack size for sensor operations
        NULL,  // Parameters
        3,     // Lower priority to prevent conflicts
        &sensor_task_handle
    );
    
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sensor monitoring task");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "System started successfully");
    return ESP_OK;
}

/**
 * @brief Stop all system operations
 * 
 * Gracefully stops all running tasks and cleans up resources.
 * 
 * @return ESP_OK on successful shutdown
 */
esp_err_t system_stop(void) {
    ESP_LOGI(TAG, "Stopping system operation...");
    
    // Stop sensor monitoring task
    if (sensor_task_handle != NULL) {
        vTaskDelete(sensor_task_handle);
        sensor_task_handle = NULL;
        ESP_LOGI(TAG, "Sensor monitoring task stopped");
    }
    
    // Clear display
    st7789_clear_screen(ST7789_BLACK);
    st7789_draw_large_string(20, 100, "STOPPED", ST7789_RED, ST7789_BLACK);
    
    ESP_LOGI(TAG, "System stopped successfully");
    return ESP_OK;
}