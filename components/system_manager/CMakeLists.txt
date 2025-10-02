idf_component_register(
    SRCS "system_manager.c"
    INCLUDE_DIRS "."
    REQUIRES st7789 dht11 wifi_manager freertos
)