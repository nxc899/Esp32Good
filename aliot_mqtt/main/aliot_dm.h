#ifndef __ALIOT_DM_H
#define __ALIOT_DM_H
#include "cJSON.h"
typedef struct {
    cJSON *obj;//操作cjson对象
    char *outbuf;
} aliyun_dm_info_t;

typedef enum 
{
    MQTT_PROPERTY_REPORT_LIGHTSWITCH = 0,
    MQTT_PROPERTY_REPLY_LIGHTSWITCH,
    MQTT_PROPERTY_REPORT_HUMITEMP,
}mqtt_msg_type_t;

aliyun_dm_info_t* aliyun_dm_info_create(mqtt_msg_type_t msgtype);
void aliyun_dm_info_mondify(aliyun_dm_info_t *pdm,const char *name,void* value ,int value_type);
void aliyun_dm_info_print(aliyun_dm_info_t *pdm);
void aliyun_dm_info_delet(aliyun_dm_info_t *pdm);

int mqtt_send_message(mqtt_msg_type_t msgtype);


#endif
