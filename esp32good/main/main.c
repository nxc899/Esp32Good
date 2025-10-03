#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "my_lcd.h"
#include "esp_log.h"
#include "FT6336G.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"


void app_main(void)
{
    //屏幕方向设置为横屏
    lcd_dev.dir = 0;

    lvgl_tick_timer_init();
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    
     /*在屏幕中间创建一个120*50大小的按钮*/
    lv_obj_t* switch_obj = lv_switch_create(lv_scr_act());
    lv_obj_set_size(switch_obj, 120, 50);
    lv_obj_align(switch_obj, LV_ALIGN_CENTER, 0, 0);

    while(1)
    {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
