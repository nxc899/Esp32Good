#ifndef __APP_GLOBAL_H
#define __APP_GLOBAL_H
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "mqtt_client.h"

#define WIFI_GOT_IP_BIT    (BIT0)

#define MQTT_SUBSCRIBED_BIT (BIT1)

#define LIGHT_SWITCH_BIT (BIT2)
#define MQTT_PROPPERTY_SET_BIT (BIT3)
#define ATH20_GET_HUMITEMP_BIT (BIT4)

#define MQTT_POST_PERPERTY_TOPIC "/sys/k1wfjO3sesH/myesp32/thing/event/property/post"
#define MQTT_REPLY_TOPIC "/sys/k1wfjO3sesH/myesp32/thing/event/property/post_reply"

#define MQTT_PROPERTY_SET_TOPIC "/sys/k1wfjO3sesH/myesp32/thing/service/property/set"
#define MQTT_PROPERTY_SET_REPLY_TOPIC "/sys/k1wfjO3sesH/myesp32/thing/service/property/set_reply"
extern float temp,humidity;
extern EventGroupHandle_t mqtt_event_group;
extern int app_light_state;

#endif
