/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "wifi_manager.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define TAG "main"

#define DEFAULT_WIFI_SSID "testwifi"
#define DEFAULT_WIFI_PASSWORD "926629abcd"

void wifi_state_handler(WIFI_STATE state)
{
    if(state == WIFI_STATE_CONNECTED){
        ESP_LOGI(TAG, "wifi connect success");
    }
    if(state == WIFI_STATE_DISCONNECTED){
        ESP_LOGI(TAG, "wifi disconnect");
    }

}

void app_main(void)
{
    nvs_flash_init();
    wifi_manager_init(wifi_state_handler);
    wifi_manager_connect(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);
    while(1)
    {
        vTaskDelay(20);
    }

}
