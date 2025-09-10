#include "my_lcd.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include <stdbool.h>

#define TAG "my_lcd"

#define LCD_HOST    SPI2_HOST
//lcd pin define
#define PIN_NUM_MISO 6
#define PIN_NUM_MOSI 9
#define PIN_NUM_CLK  8
#define PIN_NUM_CS   25

#define PIN_NUM_DC   10
#define PIN_NUM_RST  26
#define PIN_NUM_BCKL 7

#define LCD_BK_LIGHT_ON_LEVEL   1

#define PARALLEL_LINES 16

spi_device_handle_t spi2;

typedef struct
{
    uint8_t command;
    uint8_t data[16];
    uint8_t databyte;
}lcd_init_cmd_t; 

void spi_transfer_hook(spi_transaction_t *trans)
{
    int dc = (int)trans->user;
    gpio_set_level(PIN_NUM_DC,dc);
}

void lcd_cmd(spi_device_handle_t dev,uint8_t cmd,bool keepcsactive)
{
    esp_err_t ret;
    spi_transaction_t t;

    memset(&t,0,sizeof(t));

    t.length = 8;
    //发送的是命令，cs拉低，由钩子函数在发送前把io口拉高
    t.user = (void*)0;
    t.tx_buffer = &cmd;
    //只在发送命令后需要接收相应数据时设为1
    if(keepcsactive){
        t.flags = SPI_TRANS_CS_KEEP_ACTIVE;
    }
    
    ret = spi_device_polling_transmit(dev,&t);
    assert(ret == ESP_OK);
}

void lcd_data(spi_device_handle_t dev,const uint8_t* data,uint8_t len)
{
    esp_err_t ret;
    spi_transaction_t t;
    if(len ==0)
    {
        return ;
    }
    memset(&t,0,sizeof(t));

    t.length = len*8;
    t.tx_buffer = data;
    t.user = (void*)1;

    ret = spi_device_polling_transmit(dev,&t);
    assert(ret == ESP_OK);
}

uint32_t lcd_getid(void)
{
    //发送过程有用到SPI_TRANS_CS_KEEP_ACTIVE时，需要先锁定总线
    spi_device_acquire_bus(spi2,portMAX_DELAY);

    lcd_cmd(spi2,0x04,true);

    //接收ID
    spi_transaction_t t;
    memset(&t,0,sizeof(t));
    t.length = 3*8;
    t.user = (void*)1;
    t.flags = SPI_TRANS_USE_RXDATA;

    esp_err_t ret = spi_device_polling_transmit(spi2,&t);
    assert(ret==ESP_OK);

    //获取之后必须释放
    spi_device_release_bus(spi2);


    return *(uint32_t*)t.rx_data;
}

void spi_init(void)
{
    esp_err_t ret;
    spi_bus_config_t spibuscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = PARALLEL_LINES*320*2+8,
    };
    
    ret = spi_bus_initialize(LCD_HOST,&spibuscfg,SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    spi_device_interface_config_t devcfg = {
        .mode = 0,
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 7,
        .pre_cb = spi_transfer_hook,
        .clock_speed_hz = 10 * 1000 * 1000,
    };

    ret = spi_bus_add_device(LCD_HOST,&devcfg,&spi2);
    ESP_ERROR_CHECK(ret);

  
}



DRAM_ATTR static const lcd_init_cmd_t ili_init_cmds[] = {
    /* Power control B, power control = 0, DC_ENA = 1 */
    {0xCF, {0x00, 0x83, 0X30}, 3},
    /* Power on sequence control,
     * cp1 keeps 1 frame, 1st frame enable
     * vcl = 0, ddvdh=3, vgh=1, vgl=2
     * DDVDH_ENH=1
     */
    {0xED, {0x64, 0x03, 0X12, 0X81}, 4},
    /* Driver timing control A,
     * non-overlap=default +1
     * EQ=default - 1, CR=default
     * pre-charge=default - 1
     */
    {0xE8, {0x85, 0x01, 0x79}, 3},
    /* Power control A, Vcore=1.6V, DDVDH=5.6V */
    {0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
    /* Pump ratio control, DDVDH=2xVCl */
    {0xF7, {0x20}, 1},
    /* Driver timing control, all=0 unit */
    {0xEA, {0x00, 0x00}, 2},
    /* Power control 1, GVDD=4.75V */
    {0xC0, {0x26}, 1},
    /* Power control 2, DDVDH=VCl*2, VGH=VCl*7, VGL=-VCl*3 */
    {0xC1, {0x11}, 1},
    /* VCOM control 1, VCOMH=4.025V, VCOML=-0.950V */
    {0xC5, {0x35, 0x3E}, 2},
    /* VCOM control 2, VCOMH=VMH-2, VCOML=VML-2 */
    {0xC7, {0xBE}, 1},
    /* Memory access control, MX=MY=0, MV=1, ML=0, BGR=1, MH=0 */
    {0x36, {0x28}, 1},
    /* Pixel format, 16bits/pixel for RGB/MCU interface */
    {0x3A, {0x55}, 1},
    /* Frame rate control, f=fosc, 70Hz fps */
    {0xB1, {0x00, 0x1B}, 2},
    /* Enable 3G, disabled */
    {0xF2, {0x08}, 1},
    /* Gamma set, curve 1 */
    {0x26, {0x01}, 1},
    /* Positive gamma correction */
    {0xE0, {0x1F, 0x1A, 0x18, 0x0A, 0x0F, 0x06, 0x45, 0X87, 0x32, 0x0A, 0x07, 0x02, 0x07, 0x05, 0x00}, 15},
    /* Negative gamma correction */
    {0XE1, {0x00, 0x25, 0x27, 0x05, 0x10, 0x09, 0x3A, 0x78, 0x4D, 0x05, 0x18, 0x0D, 0x38, 0x3A, 0x1F}, 15},
    /* Column address set, SC=0, EC=0xEF */
    {0x2A, {0x00, 0x00, 0x00, 0xEF}, 4},
    /* Page address set, SP=0, EP=0x013F */
    {0x2B, {0x00, 0x00, 0x01, 0x3f}, 4},
    /* Memory write */
    {0x2C, {0}, 0},
    /* Entry mode set, Low vol detect disabled, normal display */
    {0xB7, {0x07}, 1},
    /* Display function control */
    {0xB6, {0x0A, 0x82, 0x27, 0x00}, 4},
    /* Sleep out */
    {0x11, {0}, 0x80},
    /* Display on */
    {0x29, {0}, 0x80},
    {0, {0}, 0xff},
};

void lcd_init(void)
{
    int cmd = 0;
    const lcd_init_cmd_t* lcd_init_cmds = ili_init_cmds;
    spi_init();
    //初始化gpio
    gpio_config_t gpio = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = ((1ULL << PIN_NUM_DC) | (1ULL << PIN_NUM_RST) | (1ULL << PIN_NUM_BCKL)),
        .pull_up_en = true,
    };
    gpio_config(&gpio);

    //重启
    gpio_set_level(PIN_NUM_RST,0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(PIN_NUM_RST,1);
    vTaskDelay(pdMS_TO_TICKS(100));

    //获取Id
    uint32_t id = lcd_getid();
    ESP_LOGI(TAG,"LCD id:%04x",id);
    
    //Send all the commands
    while (lcd_init_cmds[cmd].databyte != 0xff) {
        lcd_cmd(spi2, lcd_init_cmds[cmd].command, false);
        lcd_data(spi2, lcd_init_cmds[cmd].data, lcd_init_cmds[cmd].databyte & 0x1F);
        if (lcd_init_cmds[cmd].databyte & 0x80) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        cmd++;
    }

    ///Enable backlight
    gpio_set_level(PIN_NUM_BCKL, LCD_BK_LIGHT_ON_LEVEL);
    ESP_LOGI(TAG,"Lcd init Finish");
}


