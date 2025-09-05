#include "aliot_dm.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "app_global.h"
#include "aliot_mqtt.h"
#include "bsp_i2c.h"

static const char* DMTAG = "aliyun_dm";

static int iot_id = 0;
aliyun_dm_info_t* aliyun_dm_info_create(mqtt_msg_type_t msgtype)
{
    char id[5] = {0};
    aliyun_dm_info_t* dm = (aliyun_dm_info_t*)malloc(sizeof(aliyun_dm_info_t));
    if(dm)
    {
        sprintf(id,"%d",iot_id);
        memset(dm,0,sizeof(aliyun_dm_info_t));
        dm->obj = cJSON_CreateObject();
        cJSON_AddStringToObject(dm->obj,"id",id);
        cJSON_AddStringToObject(dm->obj,"version","1.0");
        switch(msgtype)
        {
            case MQTT_PROPERTY_REPORT_HUMITEMP:
            case MQTT_PROPERTY_REPORT_LIGHTSWITCH:
                cJSON_AddObjectToObject(dm->obj,"params");
                cJSON_AddStringToObject(dm->obj,"method","thing.event.property.post");
                break;
            case MQTT_PROPERTY_REPLY_LIGHTSWITCH:
                cJSON_AddStringToObject(dm->obj,"code","200");
                cJSON_AddStringToObject(dm->obj,"message","success");
                cJSON_AddObjectToObject(dm->obj,"data");
                break;
            
            default:break;
        }
        return dm;
    }
    return NULL;
}

void aliyun_dm_info_mondify(aliyun_dm_info_t *pdm, const char *name, void* value, int value_type)
{
    if (pdm)
    {
        cJSON* params = cJSON_GetObjectItem(pdm->obj, "params");
        if (params)
        {
            switch (value_type)
            {
                case cJSON_Number:
                    cJSON_AddNumberToObject(params, name, *(float*)value);
                    break;
                case cJSON_String:
                    cJSON_AddStringToObject(params, name, (char*)value);
                    break;
                case cJSON_True:
                    cJSON_AddNumberToObject(params, name, *(int*)value);
                    break;
                default:
                    // 不支持的类型
                    break;
            }
        }
    }
}

void aliyun_dm_info_print(aliyun_dm_info_t *pdm)
{
    if(pdm)
    {
        if(pdm->outbuf)
        {
            cJSON_free(pdm->outbuf);
            pdm->outbuf = NULL;
        }
        pdm->outbuf = cJSON_PrintUnformatted(pdm->obj);
    }
}

void aliyun_dm_info_delet(aliyun_dm_info_t *pdm)
{
    if(pdm)
    {
        if(pdm->outbuf)
        {
            cJSON_free(pdm->outbuf);
            pdm->outbuf = NULL;
        }
        if(pdm->obj)
        {
            cJSON_Delete(pdm->obj);
            pdm->obj = NULL;
        }
        free(pdm);
    }
}

int mqtt_send_message(mqtt_msg_type_t msgtype)
{
    aliyun_dm_info_t* dm = aliyun_dm_info_create(msgtype);
    if(dm==NULL)
    {
        ESP_LOGI(DMTAG,"create dm failed");
        vTaskDelay(pdMS_TO_TICKS(500));
        return 0;
    }
    switch(msgtype)
    {
        case MQTT_PROPERTY_REPORT_HUMITEMP:
            aliyun_dm_info_mondify(dm,"Humidity",&humidity,cJSON_Number);
            aliyun_dm_info_mondify(dm,"temperature",&temp,cJSON_Number);
            aliyun_dm_info_mondify(dm,"Atmosphere",&bmp280.pressure,cJSON_Number);
            // char* str = cJSON_Print(root);
            // ESP_LOGI(DMTAG,"the humitemp json is:%s",str);
            // cJSON_free(str);
            // cJSON_Delete(root);
            break;
        case MQTT_PROPERTY_REPORT_LIGHTSWITCH:
            aliyun_dm_info_mondify(dm,"LightSwitch",&app_light_state,cJSON_True);
            break;
        case MQTT_PROPERTY_REPLY_LIGHTSWITCH:
            break;
        
        default:break; 
    }
    
    aliyun_dm_info_print(dm);
    if(dm->outbuf == NULL)
    {
        ESP_LOGI(DMTAG,"dm outbuf is NULL!");
        aliyun_dm_info_delet(dm);
        return 0;
    }

    switch(msgtype)
    {
        case MQTT_PROPERTY_REPORT_HUMITEMP:
            esp_mqtt_client_publish(esp_mqtt_client,MQTT_POST_PERPERTY_TOPIC,dm->outbuf,strlen(dm->outbuf),0,0);
            break;
        case MQTT_PROPERTY_REPORT_LIGHTSWITCH:
            esp_mqtt_client_publish(esp_mqtt_client,MQTT_POST_PERPERTY_TOPIC,dm->outbuf,strlen(dm->outbuf),2,0);
            break;
        case MQTT_PROPERTY_REPLY_LIGHTSWITCH:
            esp_mqtt_client_publish(esp_mqtt_client,MQTT_PROPERTY_SET_REPLY_TOPIC,dm->outbuf,strlen(dm->outbuf),1,0);
            break;
        default:break;
    }
    

    ESP_LOGI(DMTAG,"post message is %s\nhas been posted!",dm->outbuf);

    ESP_LOGI(DMTAG,"delete dm");
    aliyun_dm_info_delet(dm);
    return 1;
}

