#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_event.h"

#define WIFI_SSID "502"
#define WIFI_PWD  "17742026460"

static const char* TAG = "MY_WIFI";

void esp_wifi_handle(void* event_handler_arg,esp_event_base_t event_base,int32_t event_id,void* event_data)
{
    if(event_base == WIFI_EVENT)
    {
        switch(event_id)
        {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                ESP_LOGI(TAG,"sta start,connecting……");
            break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG,"sta connected!");
            break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG,"sta disconnected!");
            break;
        }
    }
    else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI(TAG,"sta got ip!");
    }
}

void wifi_init(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t wifiinitcfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&wifiinitcfg));

    esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,esp_wifi_handle,NULL);
    esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP,esp_wifi_handle,NULL);

    wifi_config_t wificfg = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PWD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        }
    };
    ESP_ERRORCHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERRORCHECK(esp_wifi_set_config(WIFI_IF_STA,&wificfg));
    ESP_ERRORCHECK(esp_wifi_start());
}
