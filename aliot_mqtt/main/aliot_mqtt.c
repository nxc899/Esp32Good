#include "aliot_mqtt.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "mbedtls/md5.h"
#include "mbedtls/md.h"
#include "esp_wifi.h"
#include "app_global.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"




#define ALIOT_DEVICE_NAME "myesp32"
#define ALIOT_PRODUCT_KEY "k1wfjO3sesH"
#define ALIOT_PORDUCT_SECTET "98f0b6f76bdfa5ce88bb2171b5ac5890"

#define ALIOT_MQTT_CLIENTID "myesp32s3|securemode=2,signmethod=hmacmd5,timestamp=1753457613302|"
#define ALIOT_MQTT_USERNAME "myesp32&k1wfjO3sesH"
#define ALIOT_MQTT_PASSWORD "D41BA1E236258C6DD69BDE84233137B4"

#define TOPIC "/ext/ntp/k1wfjO3sesH/myesp32/request"

#define ALIOT_SEVER_URL "mqtts://iot-06z00b9mpvf2mn1.mqtt.iothub.aliyuncs.com"

const char* MQTTTAG = "aliot_mqtt";

extern const char* certificate_rootca;

esp_mqtt_client_handle_t esp_mqtt_client = NULL;



void mqtt_callback(void* event_handler_arg,esp_event_base_t event_base,int32_t event_id,void* event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    //int msg_id = 0;
    printf("eventid = %ld\r\n",event_id);
    switch(event_id)
    {
        case MQTT_EVENT_CONNECTED:
            xEventGroupSetBits(mqtt_event_group,MQTT_SUBSCRIBED_BIT);
            ESP_LOGI(MQTTTAG,"connect to server success");
            esp_mqtt_client_subscribe(event->client,TOPIC,1);//发送连接报文
            esp_mqtt_client_subscribe(event->client,MQTT_REPLY_TOPIC,1);//上报属性回复消息主题
            esp_mqtt_client_subscribe(event->client,MQTT_PROPERTY_SET_TOPIC,1);//设置属性消息主题
            break;
        case MQTT_EVENT_SUBSCRIBED:
  
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(MQTTTAG,"mqtt data arrive,topic is %.*s",event->topic_len,event->topic);
            ESP_LOGI(MQTTTAG,"mqtt data arrive,topic is %.*s",event->data_len,event->data);
            if(strstr(event->data,"params"))//服务器属性设置
            {
                cJSON *root = cJSON_Parse(event->data);
                cJSON *param = cJSON_GetObjectItem(root,"params");
                if(param)
                {
                    cJSON *param_obj = cJSON_GetObjectItem(param,"LightSwitch");
                    if(param_obj)
                    {
                        int bool_ls = cJSON_GetNumberValue(param_obj);
                        ESP_LOGI(MQTTTAG,"Property params:%d",bool_ls);
                        app_light_state = bool_ls;//开灯/关灯操作
                        xEventGroupSetBits(mqtt_event_group,LIGHT_SWITCH_BIT);
                    }
                    cJSON_free(param_obj);
                }
                cJSON_free(root);
                cJSON_free(param);
                //发送响应报文
                xEventGroupSetBits(mqtt_event_group,MQTT_PROPPERTY_SET_BIT);
                }
            break;
        default:break;
    }
}

void aliot_mqtt_start()
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.port = 1883,
        .broker.address.uri = ALIOT_SEVER_URL,
        .credentials.client_id =ALIOT_MQTT_CLIENTID,
        .credentials.username = ALIOT_MQTT_USERNAME,
        .credentials.authentication.password = ALIOT_MQTT_PASSWORD,
        .broker.verification.certificate = certificate_rootca,
    };

    esp_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(esp_mqtt_client,MQTT_EVENT_ANY,mqtt_callback,NULL);
    esp_mqtt_client_start(esp_mqtt_client);
}


const char* certificate_rootca="-----BEGIN CERTIFICATE-----\n"
"MIIDdTCCAl2gAwIBAgILBAAAAAABFUtaw5QwDQYJKoZIhvcNAQEFBQAwVzELMAkG"
"A1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsTB1Jv"
"b3QgQ0ExGzAZBgNVBAMTEkdsb2JhbFNpZ24gUm9vdCBDQTAeFw05ODA5MDExMjAw"
"MDBaFw0yODAxMjgxMjAwMDBaMFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9i"
"YWxTaWduIG52LXNhMRAwDgYDVQQLEwdSb290IENBMRswGQYDVQQDExJHbG9iYWxT"
"aWduIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDaDuaZ"
"jc6j40+Kfvvxi4Mla+pIH/EqsLmVEQS98GPR4mdmzxzdzxtIK+6NiY6arymAZavp"
"xy0Sy6scTHAHoT0KMM0VjU/43dSMUBUc71DuxC73/OlS8pF94G3VNTCOXkNz8kHp"
"1Wrjsok6Vjk4bwY8iGlbKk3Fp1S4bInMm/k8yuX9ifUSPJJ4ltbcdG6TRGHRjcdG"
"snUOhugZitVtbNV4FpWi6cgKOOvyJBNPc1STE4U6G7weNLWLBYy5d4ux2x8gkasJ"
"U26Qzns3dLlwR5EiUWMWea6xrkEmCMgZK9FGqkjWZCrXgzT/LCrBbBlDSgeF59N8"
"9iFo7+ryUp9/k5DPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8E"
"BTADAQH/MB0GA1UdDgQWBBRge2YaRQ2XyolQL30EzTSo//z9SzANBgkqhkiG9w0B"
"AQUFAAOCAQEA1nPnfE920I2/7LqivjTFKDK1fPxsnCwrvQmeU79rXqoRSLblCKOz"
"yj1hTdNGCbM+w6DjY1Ub8rrvrTnhQ7k4o+YviiY776BQVvnGCv04zcQLcFGUl5gE"
"38NflNUVyRRBnMRddWQVDf9VMOyGj/8N7yy5Y0b2qvzfvGn9LhJIZJrglfCm7ymP"
"AbEVtQwdpf5pLGkkeB6zpxxxYu7KyJesF12KwvhHhm4qxFYxldBniYUr+WymXUad"
"DKqC5JlR3XC321Y9YeRq4VzW9v493kHMB65jUr9TU/Qr6cf9tveCX4XSQRjbgbME"
"HMUfpIBvFSDJ3gyICh3WZlXi/EjJKSZp4A==\n"
"-----END CERTIFICATE-----";

