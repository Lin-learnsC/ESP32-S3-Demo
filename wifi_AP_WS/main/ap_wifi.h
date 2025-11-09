#ifndef _AP_WIFI_H_
#define _AP_WIFI_H_
#include "wifi_manager.h"
#include <stdbool.h>

/** wifi功能和ap配网功能初始化
 * @param f wifi连接状态回调函数
 * @return 无 
*/
extern void ap_wifi_init(p_wifi_state_cb f);

/** 连接某个热点
 * @param ssid
 * @param password
 * @return 无 
*/
extern void ap_wifi_set(const char* ssid,const char* password);

/** 启动配网模式
 * @param enable 暂无用，强制true
 * @return 无 
*/
extern void ap_wifi_apcfg(bool enable);

#endif