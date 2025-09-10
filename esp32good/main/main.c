#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "my_lcd.h"
void app_main(void)
{
    lcd_init();
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
