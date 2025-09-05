#include "app_global.h"
#include "mqtt_client.h"



float temp,humidity;
EventGroupHandle_t mqtt_event_group = NULL;

int app_light_state = 0;

