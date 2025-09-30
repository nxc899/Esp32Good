#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "my_lcd.h"
#include "esp_log.h"
#include "FT6336G.h"

void app_main(void)
{
    uint8_t ts= 0;
    lcd_dev.dir = 2;
    lcd_init();
    memset(drawbuf,GREEN,sizeof(uint16_t)*4);
    LCD_DRAW_POINT(2,2,drawbuf,2);
    LCD_DRAW_POINT(2,22,drawbuf,2);
    touch_init();
    while(1)
    {
        ts = touch_scan();
        if(ts)
        {
            get_touch_pos();
            //ESP_LOGI(TAG,"x:%d,y:%d",touch_dev.x,touch_dev.y);
            memset(drawbuf,GREEN,sizeof(uint16_t)*4);
            LCD_DRAW_POINT(touch_dev.x,touch_dev.y,drawbuf,2);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
