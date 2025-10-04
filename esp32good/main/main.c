#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "my_lcd.h"
#include "esp_log.h"
#include "FT6336G.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "my_wifi.h"
#include "my_lvgl.h"


void app_main(void)
{
    //屏幕方向设置为横屏
    lcd_dev.dir = 0;

    lvgl_tick_timer_init();
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    lv_src1_create();

    my_wifi_start();

    while(1)
    {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
