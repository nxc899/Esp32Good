#include "my_wifi.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"

#define SSID "HUAWEI-1CADZ0"
#define PWD "18550584690"

static const char* WIFI_TAG = "wfii_log";

wifi_run_data wifirdata = {
    .state = STA_OFF, 
    .wifi_init = my_wifi_start,
    .wifi_connect = esp_wifi_connect};

static void wifi_event_handle(void* event_handler_arg,esp_event_base_t event_base,int32_t event_id,void* event_data)
{
    if(event_base == WIFI_EVENT)
    {
        switch(event_id)
        {
            case WIFI_EVENT_STA_START:
                wifirdata.state = STA_START;
                esp_wifi_connect(); 
            break;
            case WIFI_EVENT_STA_CONNECTED:
                wifirdata.state = WIFI_CONNECT;
                ESP_LOGI(WIFI_TAG,"WIFI CONNECTED");
            break;
            case WIFI_EVENT_STA_DISCONNECTED:
                wifirdata.state = WIFI_DISCONNECT;
                ESP_LOGI(WIFI_TAG,"WIFI DISCONNECTED");
            break;
            default:break;
        }
    }
    else if(event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI(WIFI_TAG,"ESP GOT IP!");
    }
}

void my_wifi_start(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_init_cfg);

    esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,wifi_event_handle,NULL);
    esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP,wifi_event_handle,NULL);

    wifi_config_t wificfg = {
        .sta = {
            .ssid = SSID,
            .password = PWD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA,&wificfg));
    ESP_ERROR_CHECK(esp_wifi_start());
}


