/* Boot_Key Example*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

static QueueHandle_t gpio_evt_queue = NULL;  //定义队列消息

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%"PRIu32"] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}

void app_main(void)
{
    //zero-initialize the config structure.
    gpio_config_t io_conf = {
        //disable interrupt
        .intr_type = GPIO_INTR_NEGEDGE,
        //set as output mode
        .mode = GPIO_MODE_INPUT,
        //bit mask of the pins that you want to set,e.g.GPIO0  GPIO_NUM宏即0值，0左移1，使能GPIO0
        .pin_bit_mask = 1ULL <<GPIO_NUM_0,
        //disable pull-down mode
        .pull_down_en = 0,
        //disable pull-up mode
        .pull_up_en = 1
        //configure GPIO with the given settings
    };

    gpio_config(&io_conf);

    //create a queue to handle gpio event from isr 创建队列消息
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    //install gpio isr service 打开中断服务
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin 创建中断服务函数
    gpio_isr_handler_add(GPIO_NUM_0, gpio_isr_handler, (void*) GPIO_NUM_0);

}
