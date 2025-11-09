#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include <string.h>
#include "lwip/ip4_addr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"  //信号

#include "esp_netif.h"
#include "esp_mac.h"
#include "lwip/ip4_addr.h"
#define TAG     "wifi_manager"

#define MAX_CONNECT_RETRY   10    //最大重连次数
static int sta_connect_cnt = 0; //重连次数



static bool is_sta_connected = false; //当前sta连接状态

static SemaphoreHandle_t scan_sem = NULL;

static esp_netif_t *esp_netif_sta = NULL;
static esp_netif_t *esp_netif_ap = NULL;

//AP模式下的SSID名称
static const char *ap_ssid_name = "ESP32-AP";

//AP模式下的密码
static const char *ap_password = "12345678";

//function declartion begin
static p_wifi_state_cb wifi_callback = NULL; 

void wifi_manager_init(p_wifi_state_cb handler);

esp_err_t wifi_manager_connect(const char *ssid, const char *password);

esp_err_t wifi_manager_ap(void);

esp_err_t wifi_manager_scan(p_wifi_scan_cb f);
//function declartion end

/*事件响应回调函数*/
/* @param arg   用户传递的参数
 * @param event_base    事件类别
 * @param event_id      事件ID
 * @param event_data    事件携带的数据
 * @return 无
*/
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if(event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:      //WIFI以STA模式启动后触发此事件
        {
            wifi_mode_t mode;
            esp_wifi_get_mode(&mode);
            if(mode == WIFI_MODE_STA)
                esp_wifi_connect();         //启动WIFI连接
            break;
        }
        case WIFI_EVENT_STA_CONNECTED:  //WIFI连上路由器后，触发此事件
            ESP_LOGI(TAG, "Connected to AP");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:   //WIFI从路由器断开连接后触发此事件
            if(is_sta_connected)
            {
                if(wifi_callback){
                    wifi_callback(WIFI_STATE_DISCONNECTED);
                }
                is_sta_connected = false;
            }
            if(sta_connect_cnt < MAX_CONNECT_RETRY)
            {
            wifi_mode_t mode;
            esp_wifi_get_mode(&mode);
            if(mode == WIFI_MODE_STA)
                    esp_wifi_connect();             //继续重连
                sta_connect_cnt++;
            }
            ESP_LOGI(TAG,"connect to the AP fail,retry now");
            break;
        case WIFI_EVENT_AP_STACONNECTED:
        {
            //有设备连接了热点，把它的MAC打印出来
            wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
            ESP_LOGI(TAG, "Station "MACSTR" joined, AID=%d",
                    MAC2STR(event->mac), event->aid);
            break;
        }
        case WIFI_EVENT_AP_STADISCONNECTED:
        {
            //有设备断开了热点
            wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
            ESP_LOGI(TAG, "Station "MACSTR" left, AID=%d",
                    MAC2STR(event->mac), event->aid);
            break;
        }
        default:
            break;
        }
    }
    else if(event_base == IP_EVENT)      //IP相关事件
    {
        if(event_id == IP_EVENT_STA_GOT_IP)
        {
            ESP_LOGI(TAG,"Get ip addr");          //只有获取到路由器分配的IP，才认为是连上了路由
            sta_connect_cnt = 0;   //注：成功连接后，重连次数重置 25.11.9
            is_sta_connected = true;
            if(wifi_callback)
            {
                wifi_callback(WIFI_STATE_CONNECTED);
            }
        }
    }
}

/** 初始化wifi，默认进入STA模式
 * @param 无
 * @return 无 
*/
void wifi_manager_init(p_wifi_state_cb handler)
{
    ESP_ERROR_CHECK(esp_netif_init());  //用于初始化tcpip协议栈

    //创建事件循环
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_sta = esp_netif_create_default_wifi_sta();    //使用默认配置创建STA对象
    esp_netif_ap = esp_netif_create_default_wifi_ap();      //使用默认配置创建AP对象
    //初始化WIFI
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
														
    wifi_callback = handler;
    scan_sem = xSemaphoreCreateBinary();  //step 1-创建二进制信号量
    //配置WIFI工作模式 STA/AP/STA and AP
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    //ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    
    ESP_LOGI(TAG, "wifi_init finished.");
}

/** 进入ap+sta模式
 * @param 无
 * @return 成功/失败
*/
esp_err_t wifi_manager_ap(void)
{
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    if(mode == WIFI_MODE_APSTA)   //需要使用AP+STA模式，才可以执行扫描同时保持客户端连接
    {
        return ESP_OK;
    }
    esp_wifi_disconnect();  //先断开
    esp_wifi_stop();        //停止工作
    esp_wifi_set_mode(WIFI_MODE_APSTA);
    wifi_config_t wifi_config = {
        .ap = {
            .channel = 5,               //wifi的通信信道
            .max_connection = 2,        //最大连接数
            .authmode = WIFI_AUTH_WPA2_PSK, // 加密方式为WPA2_PSK
        },
    };
    snprintf((char *)wifi_config.ap.ssid, 32, "%s", ap_ssid_name);
    wifi_config.ap.ssid_len = strlen(ap_ssid_name);
    snprintf((char *)wifi_config.ap.password, 64, "%s", ap_password);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    //AP模式下 设置ip地址、网关、子网掩码
    esp_netif_ip_info_t ipInfo;
    IP4_ADDR(&ipInfo.ip, 192, 168, 100, 1);
    IP4_ADDR(&ipInfo.gw, 192, 168, 100, 1);
    IP4_ADDR(&ipInfo.netmask, 255, 255, 255, 0);

    esp_netif_dhcps_stop(esp_netif_ap);   //关闭DHCP，即自动分配IP地址
    esp_netif_set_ip_info(esp_netif_ap, &ipInfo);
    esp_netif_dhcps_start(esp_netif_ap);  //开启DHCP

    esp_wifi_start();
    return ESP_OK;
}
/** 扫描任务
 * @param 无
 * @return 成功/失败
*/
static void scan_task(void *param)
{
    p_wifi_scan_cb callback = (p_wifi_scan_cb)param;  //强转
    uint16_t ap_count = 0;
    uint16_t ap_num = 20;
    wifi_ap_record_t *ap_list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) *ap_num); //分配堆空间，大小即ap_num的大小
    esp_wifi_scan_start(NULL,true); //true则阻塞在这里

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_list));
    ESP_LOGI(TAG, "Total ap count:%d,actual ap count:%d", ap_count, ap_num);

    if(callback)      //执行回调函数通知我们扫描结果
        callback(ap_num, ap_list);
    free(ap_list);
    xSemaphoreGive(scan_sem);  //step 2-give 释放掉信号量
    vTaskDelete(NULL);  //删除自身
} 

/** 启动扫描
 * @param 无
 * @return 成功/失败
*/
esp_err_t wifi_manager_scan(p_wifi_scan_cb f)
{
    if(!scan_sem)
    {
        scan_sem = xSemaphoreCreateBinary();
        xSemaphoreGive(scan_sem);
    }
    //先看一下是否有扫描任务在执行，防止重复扫描
    if(pdTRUE == xSemaphoreTake(scan_sem, 0))  //step 3-take
    {
        //清除上一次保存的ap list
        esp_wifi_clear_ap_list();  
        //启动一个扫描任务--分配到核1
        //return xTaskCreatePinnedToCore(scan_task,"scan",8192,f,3,NULL,1);
        if(pdTRUE == xTaskCreatePinnedToCore(scan_task,"scan",8192,f,3,NULL,1))
            return ESP_OK;
    }
    ESP_LOGE(TAG,"scan error!");
    return ESP_FAIL;  
}

/** 连接wifi
 * @param ssid
 * @param password
 * @return 成功/失败
*/
esp_err_t wifi_manager_connect(const char *ssid,const char *password)
{
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,  //加密方式为WPA2_PSK
        },
    };
    //使用格式化字符串函数 snprintf
    snprintf((char *)wifi_config.sta.ssid, 32, "%s", ssid);
    snprintf((char *)wifi_config.sta.password, 64, "%s", password);
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    if(mode != WIFI_MODE_STA)
    {
        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        esp_wifi_start();
    }
    else
    {
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        esp_wifi_connect();
    }
    return ESP_OK;
}
