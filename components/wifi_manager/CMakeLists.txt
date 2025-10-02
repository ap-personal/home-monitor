idf_component_register(
    SRCS "wifi_manager.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_event esp_http_client nvs_flash esp_netif freertos
)