#include "app_wifi.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "app_global.h"

#define WIFI_SSID "502"
#define WIFI_PASSWORD "17742026460"

#define WIFI_RECON_INTERVAL 200
#define WIFI_RECON_TIME 5

const char* TAG = "app_wifi";



void esp_wifi_event_handle(void* event_handler_arg, esp_event_base_t event_base,int32_t event_id,void* event_data)
{
    static uint8_t a = 0,i = 0;
    if(WIFI_EVENT == event_base)
    {
        switch(event_id)
        {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                ESP_LOGI(TAG,"STA START,CONNECTING WIFI!");
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG,"WIFI CONNECTED!");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG,"WIFI DISCONNECTED!");
                while(a!=5&&i!=1)
                {
                    esp_wifi_connect();
                    vTaskDelay(pdMS_TO_TICKS(WIFI_RECON_INTERVAL));
                    a++;
                }
                if(a==5)
                {
                    ESP_LOGI(TAG,"RECONNECT COUNT OUT!");
                }
                break;
            default:break;

        }
    }
    else if(event_id == IP_EVENT_STA_GOT_IP)
    {
        i=1;
        a=0;
        ESP_LOGI(TAG,"ESP GOT IP!");
        xEventGroupSetBits(mqtt_event_group,WIFI_GOT_IP_BIT);
    }
}


void app_wifistart(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));

    esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,esp_wifi_event_handle,NULL);
    esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP,esp_wifi_event_handle,NULL);

    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        }
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA,&wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());


}







