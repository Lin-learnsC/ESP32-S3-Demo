idf_component_register(SRCS "wifi_manager.c" "wifi_basic_main.c"
                       PRIV_REQUIRES esp_wifi nvs_flash
                       INCLUDE_DIRS ".")
