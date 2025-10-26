#ifndef _WIFI_MANAGER_H_
#define _WIFI_MANAGER_H_

typedef enum
{
    WIFI_STATE_CONNECTED,
    WIFI_STATE_DISCONNECTED
} WIFI_STATE;

typedef void (*p_wifi_state_cb)(WIFI_STATE);   //定义回调函数p_wifi_state_cb，用来通知主任务 wifi的连接状态

extern void wifi_manager_init(p_wifi_state_cb handler);

extern void wifi_manager_connect(const char *ssid, const char *password);

#endif
