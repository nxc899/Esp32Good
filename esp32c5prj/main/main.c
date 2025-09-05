#include <stdio.h>
#include "my_wifi.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char* TAG = "main_fun";

void app_main(void)
{
    wifi_init();

    while(1)
    {
        ESP_LOGI(TAG,"main loop");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
