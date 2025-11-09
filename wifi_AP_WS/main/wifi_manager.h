#ifndef _WIFI_MANAGER_H_
#define _WIFI_MANAGER_H_

#include "esp_err.h"
#include "esp_wifi.h"

typedef enum
{
    WIFI_STATE_CONNECTED,
    WIFI_STATE_DISCONNECTED,
} WIFI_STATE;

//定义回调函数p_wifi_scan_cb, ap_records列表为函数参数
typedef void (*p_wifi_scan_cb)(int num, wifi_ap_record_t *ap_records); 

//定义回调函数p_wifi_state_cb, WIFI_STATE为函数参数, 用来通知主任务 wifi的连接状态
typedef void (*p_wifi_state_cb)(WIFI_STATE state);   

/** 初始化wifi，默认进入STA模式
 * @param f wifi状态变化回调函数
 * @return 无 
*/
extern void wifi_manager_init(p_wifi_state_cb handler);

/** 进入ap+sta模式
 * @param 无
 * @return 成功/失败
*/
extern esp_err_t wifi_manager_ap(void);

/** 启动扫描
 * @param 无
 * @return 成功/失败
*/
extern esp_err_t wifi_manager_scan(p_wifi_scan_cb f);

/** 连接wifi
 * @param ssid
 * @param password
 * @return 成功/失败
*/
extern esp_err_t wifi_manager_connect(const char *ssid, const char *password);

#endif
