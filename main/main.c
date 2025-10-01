#include <stdio.h>
#include "esp_log.h"
#include "system_manager.h"

static const char *TAG = "MAIN";

/**
 * @brief Main application entry point
 * 
 * Minimal main function that only handles system-level initialization
 * and startup. All application logic is delegated to the system manager.
 */
void app_main(void)
{
    ESP_LOGI(TAG, "   ESP32 Sensor Monitoring System");
    ESP_LOGI(TAG, "   ST7789 Display + DHT11 Sensor");
    
    // Initialize all system components
    esp_err_t ret = system_init();
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "System initialization failed");
        return;
    }
    
    // Start system operation
    ret = system_start();
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "System startup failed");
        return;
    }
    
    ESP_LOGI(TAG, "System running successfully");
}