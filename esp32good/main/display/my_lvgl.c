#include "my_lvgl.h "
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "my_wifi.h"
#include "esp_log.h"

static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED)
    {
        if(wifirdata.state == WIFI_DISCONNECT)
        {
            wifirdata.wifi_connect();
        }else if(wifirdata.state == WIFI_CONNECT)
        {
            ESP_LOGI("lvgl_log","wifi has connected");
        }
    }
}

void lv_src1_create(void)
{
    //创建屏幕
    lv_obj_t *src1 = lv_obj_create(NULL);
    lv_scr_load(src1);
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0xffffff), LV_PART_MAIN);

    lv_obj_t *btn = lv_button_create(src1);
    lv_obj_set_pos(btn,120,40);
    lv_obj_set_size(btn,80,35);
    lv_obj_add_event_cb(btn,btn_event_cb,LV_EVENT_CLICKED,NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label,"Connect");
    lv_obj_center(label);
}


