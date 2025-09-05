#include <stdio.h>
#include "app_wifi.h"
#include "aliot_mqtt.h"
#include "freertos/FreeRTOS.h"
#include "aliot_dm.h"
#include "app_global.h"
#include "esp_log.h"
#include "aliot_dm.h"
#include "mqtt_client.h"
#include "bsp_i2c.h"
#include "w25q64_driver.h"

static const char* MAINTAG = "app_main";

static TaskHandle_t mqtt_post_task_handler = NULL;
static TaskHandle_t read_i2c_device_data_handler = NULL;
static TaskHandle_t w25q64_task_handler = NULL;

static void mqtt_post_task(void* arg);
static void read_i2c_device_data(void* arg);
static void w25q64_task(void* arg);

void app_main(void)
{
    mqtt_event_group = xEventGroupCreate();
    ESP_LOGI(MAINTAG,"create event group succeed!");
    app_wifistart();
    xEventGroupWaitBits(mqtt_event_group,WIFI_GOT_IP_BIT,pdTRUE,pdFALSE,portMAX_DELAY);
    ESP_LOGI(MAINTAG,"got ip succeed,event group bit = %d",WIFI_GOT_IP_BIT);
    aliot_mqtt_start();
    xEventGroupWaitBits(mqtt_event_group,MQTT_SUBSCRIBED_BIT,pdTRUE,pdFALSE,portMAX_DELAY);
    ESP_LOGI(MAINTAG,"got ip succeed,event group bit = %d",MQTT_SUBSCRIBED_BIT);


    xTaskCreate(mqtt_post_task,"mqtt_post_task",4096,NULL,5,&mqtt_post_task_handler);
    xTaskCreate(read_i2c_device_data,"read_i2c_device_data",4096,NULL,5,&read_i2c_device_data_handler);
    xTaskCreate(w25q64_task,"w25q64_task",4096,NULL,5,&w25q64_task_handler);
}

static void mqtt_post_task(void* arg)
{
    while(1)
    {
        EventBits_t event_bits = xEventGroupWaitBits(mqtt_event_group,LIGHT_SWITCH_BIT|MQTT_PROPPERTY_SET_BIT|ATH20_GET_HUMITEMP_BIT,pdTRUE,pdFALSE,pdMS_TO_TICKS(500));
        if(event_bits == LIGHT_SWITCH_BIT)
        {
            mqtt_send_message(MQTT_PROPERTY_REPORT_LIGHTSWITCH);  
        }
        else if(event_bits == MQTT_PROPPERTY_SET_BIT)
        {
            mqtt_send_message(MQTT_PROPERTY_REPLY_LIGHTSWITCH);           
        }
        else if(event_bits == ATH20_GET_HUMITEMP_BIT)
        {
            mqtt_send_message(MQTT_PROPERTY_REPORT_HUMITEMP);
        }
        vTaskDelay(pdMS_TO_TICKS(500));

    }
}

static void read_i2c_device_data(void* arg)
{
    bsp_i2c_init();
    ath20_init();
    bmp280_init();
    while(1)
    {
        i2c_read_ath20_data(&humidity,&temp);
        ESP_LOGI(MAINTAG,"humidity is %f,temperate is %f",humidity,temp);
        i2c_read_bmp280_data(&bmp280);
        ESP_LOGI(MAINTAG,"bmp280 temp is %f,humi is %f",bmp280.temperature,bmp280.pressure);
        xEventGroupSetBits(mqtt_event_group,ATH20_GET_HUMITEMP_BIT);

        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}

static void w25q64_task(void* arg)
{
    uint8_t recvbuf[64];
    w25q64_read_data_t read_data = {0};

    w25q64_config_t cfg = {
        .host = SPI2_HOST,
        .mosi = GPIO_NUM_10,
        .miso = GPIO_NUM_12,
        .clk = GPIO_NUM_11,
        .cs = GPIO_NUM_13,
        .mode = 0,
        .clk_src = SPI_CLK_SRC_DEFAULT,
        .spi_freq = 1000000
    };
    w25q64_init(&cfg);

    w25q64_read_unique_id(&read_data);
    for(int i =0;i<read_data.datalen;i++){
        ESP_LOGI(MAINTAG,"w25q64 unique id is:%0x",read_data.buf[i]);
    }
    
    w25q64_read_device_id(&read_data);
    for(int i =0;i<read_data.datalen;i++){
        ESP_LOGI(MAINTAG,"w25q64 device id is:%0x",read_data.buf[i]);
    }

    memset(recvbuf, 0, sizeof(recvbuf));

    w25q64_page_write(0x0000,0,0,test_buf,sizeof(test_buf));
    w25q64_page_read(0x0000,0,0,recvbuf, sizeof(recvbuf));
    ESP_LOGI(MAINTAG,"w25q64 read buf:%.*s",strlen((char *)recvbuf),(char*)recvbuf);
    w25q64_erase_sector(0x0000,0,0);
    w25q64_page_read(0x0000,0,0,recvbuf, sizeof(recvbuf));
    ESP_LOGI(MAINTAG,"w25q64 read buf:%.*s",strlen((char *)recvbuf),(char*)recvbuf);
    vTaskDelete(NULL);
}
