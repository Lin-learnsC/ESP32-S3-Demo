idf_component_register(SRCS "ap_wifi.c" "main.c" "wifi_manager.c" "ws_server.c"
                    PRIV_REQUIRES esp_wifi nvs_flash
                    INCLUDE_DIRS "."
		    REQUIRES esp_wifi esp_http_server lwip json spiffs esp_driver_gpio
)
spiffs_create_partition_image(html ../html FLASH_IN_PROJECT)