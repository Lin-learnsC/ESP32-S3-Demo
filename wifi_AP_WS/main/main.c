/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "ap_wifi.h"

#define TAG     "main"

static QueueHandle_t gpio_evt_queue = NULL;  //定义队列消息

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);  //发送队列
}

static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {   //接收队列
            vTaskDelay(pdMS_TO_TICKS(20)); //消抖延时
            if (gpio_get_level(io_num) == 0){
                ESP_LOGI(TAG, "Button Pressed on GPIO %d", io_num);
                //ap_wifi_apcfg(true);   //按键按下后，再执行后续逻辑
            }
            //printf("GPIO[%"PRIu32"] intr, val: %d\n", io_num, gpio_get_level(io_num));
            //ap_wifi_apcfg(true);
        }
    }
}

void wifi_state_handle(WIFI_STATE state)
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
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        //bit mask of the pins that you want to set,e.g.GPIO0  GPIO_NUM宏即0值，0左移1，使能GPIO0
        .pin_bit_mask = 1ULL <<GPIO_NUM_0,
        .pull_down_en = 0,
        .pull_up_en = 1
    };
    //create a queue to handle gpio event from isr 创建队列消息
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 1, NULL);
    //install gpio isr service 打开中断服务
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin 创建中断服务函数
    gpio_isr_handler_add(GPIO_NUM_0, gpio_isr_handler, (void*) GPIO_NUM_0);

	ESP_ERROR_CHECK(nvs_flash_init());
    //wifi_manager_init(wifi_state_handler);
    //wifi_manager_connect(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);

    ap_wifi_init(wifi_state_handle);
    ap_wifi_apcfg(true);
    // while(1)
    // {
    //     vTaskDelay(20);
    // }

}
