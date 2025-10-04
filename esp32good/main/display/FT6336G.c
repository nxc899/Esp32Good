#include "FT6336G.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "my_lcd.h"

#define TOUCHI2C_SCL_PIN 1
#define TOUCHI2C_SDA_PIN 3
#define TOUCH_RESET_PIN 0
#define TOUCH_INT_PIN 2
#define TOUCH_I2C_ADDR 0x38

#define TOUCH_SENSE 22

i2c_master_bus_handle_t touchi2c_handle = NULL;
i2c_master_dev_handle_t dev_handle = NULL;

touch_dev_t touch_dev;

void touch_status(void)
{
    uint8_t wbuf[1] = {FT_REG_NUM_FINGER};
    uint8_t rbuf[1] = {0};

    i2c_master_transmit_receive(dev_handle,wbuf,1,rbuf,1,20);

    touch_dev.sta = *rbuf;
}

void get_touch_pos(void)
{
    uint8_t wbuf[1] = {FT_TP1_REG};//目前只支持一个触点
    uint8_t rbuf[4] = {0};

    i2c_master_transmit_receive(dev_handle,wbuf,1,rbuf,4,20);

    switch(lcd_dev.dir)
    {
        case 0:
            touch_dev.x = 240-(((uint16_t)((rbuf[0]&0x0F)<<8)) + rbuf[1]);
            touch_dev.y = 320-(((uint16_t)((rbuf[2]&0x0F)<<8)) + rbuf[3]);
        break;
        //x,y互换
        case 1:
            touch_dev.x = ((uint16_t)((rbuf[2]&0x0F)<<8)) + rbuf[3];
            touch_dev.y = ((uint16_t)((rbuf[0]&0x0F)<<8)) + rbuf[1];
        break;
    }
    

}

void touch_init(void)
{
    int ret = 0;
    uint8_t wbuf[2] = {0};

    gpio_config_t io_conf = {
        .pin_bit_mask = 1UL<<TOUCH_RESET_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = true,
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));

    i2c_master_bus_config_t buscfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .sda_io_num = TOUCHI2C_SDA_PIN,
        .scl_io_num = TOUCHI2C_SCL_PIN,
        .i2c_port = I2C_NUM_0,
        .flags.enable_internal_pullup = true,
        .glitch_ignore_cnt = 7,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&buscfg,&touchi2c_handle));

    i2c_device_config_t devcfg = {
        .device_address = TOUCH_I2C_ADDR,
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .scl_speed_hz = 400000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(touchi2c_handle,&devcfg,&dev_handle));


    gpio_set_level(TOUCH_RESET_PIN,0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(TOUCH_RESET_PIN,1);
    vTaskDelay(pdMS_TO_TICKS(500));

    ret = i2c_master_probe(touchi2c_handle,TOUCH_I2C_ADDR,10);

    if(ret == ESP_OK)
    {
        ESP_LOGI("TOUCH","Touch I2C device found!");
    }

    //设置触摸灵敏度
    *wbuf = FT_ID_G_THGROUP;
    *(wbuf+1) = 15;
    i2c_master_transmit(dev_handle,wbuf,2,10);

    //进入正常操作模式
    *wbuf = FT_DEVIDE_MODE;
    *(wbuf+1) = 0;
    i2c_master_transmit(dev_handle,wbuf,2,10);

    //进入正常操作模式
    *wbuf = FT_ID_G_MODE;
    i2c_master_transmit(dev_handle,wbuf,2,10);


    //进入正常操作模式
    *wbuf = FT_ID_G_PMODE;
    i2c_master_transmit(dev_handle,wbuf,2,10);

    //报点平吕
    *wbuf = FT_ID_G_PERIODACTIVE;
    *(wbuf+1) = 12;
    i2c_master_transmit(dev_handle,wbuf,2,10);
}



