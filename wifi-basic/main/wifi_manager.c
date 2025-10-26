#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#define MAX_CONNECT_RETRY 10    //最大重连次数
static int sta_connect_cnt = 0; //重连次数

#define TAG "wifi_manager"

static p_wifi_state_cb wifi_callback = NULL; 

static bool is_sta_connected = false; //当前sta连接状态

/*事件函数定义*/
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if(event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();   //Connect WiFi station to the AP
                break;
            case WIFI_EVENT_STA_DISCONNECTED: //包括SSID密码输错等
                if(is_sta_connected)
                {
                    is_sta_connected = false;
                    if(wifi_callback){
                        wifi_callback(WIFI_STATE_DISCONNECTED);
                    }
                }
                if(sta_connect_cnt < MAX_CONNECT_RETRY)
                {
                    esp_wifi_connect();
                    sta_connect_cnt++;
                }
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "connected to ap");
                break;
            default:
                break;
        }
    }
    else if(event_base == IP_EVENT)
    {
        if(event_id == IP_EVENT_STA_GOT_IP)
        {
            ESP_LOGI(TAG,"Get ip addr");
            is_sta_connected = true;
            if(wifi_callback)
            {
                wifi_callback(WIFI_STATE_CONNECTED);
            }
        }
    }
}

void wifi_manager_init(p_wifi_state_cb handler)
{
    ESP_ERROR_CHECK(esp_netif_init());

    //创建事件循环
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    //目前我们的事件就两类，wifi事件和ip事件
    //注册两类的事件
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));
    #if 0
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };
    #endif

    wifi_callback = handler;

    //配置WIFI工作模式 STA/AP/STA and AP
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    //ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

}

void wifi_manager_connect(const char *ssid, const char *password)
{
    wifi_config_t wifi_config = {
        .sta = {
            //.ssid = EXAMPLE_ESP_WIFI_SSID,
            //.password = EXAMPLE_ESP_WIFI_PASS,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,  //加密方式改为WPA2_PSK
            //.sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            //.sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };
    //使用格式化字符串函数 snprintf
    snprintf((char *)wifi_config.sta.ssid, 32, "%s", ssid);
    snprintf((char *)wifi_config.sta.password, 64, "%s", password);

    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    if(mode != WIFI_MODE_STA)
    {
        esp_wifi_stop();
        esp_wifi_set_mode(WIFI_MODE_STA);
    }
     sta_connect_cnt = 0; //重连次数清0
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    esp_wifi_start();
}