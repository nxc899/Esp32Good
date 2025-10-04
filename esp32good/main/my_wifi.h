#ifndef _MY_WIFI_H
#define _MY_WIFI_H
#include "esp_err.h"
typedef enum
{
    STA_OFF=0,
    STA_START,
    WIFI_CONNECT,
    WIFI_DISCONNECT,
    WIFI_GOTIP,
    WIFI_MODE_ALL
}wifi_state;

typedef struct
{
    wifi_state state;
    void (*wifi_init)(void);
    esp_err_t(*wifi_connect)(void);
    
}wifi_run_data;

void my_wifi_start(void);

extern wifi_run_data wifirdata;
#endif