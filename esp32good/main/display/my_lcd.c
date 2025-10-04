#include "my_lcd.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include <stdbool.h>
#include <math.h>
#include "esp_timer.h"
#include "lvgl.h"

uint16_t drawbuf[320*240];


static esp_timer_handle_t lvgl_tick_timer = NULL;
spi_device_handle_t spi2;
_lcd_dev lcd_dev;

static void lv_tick_task(void* arg)
{
    lv_tick_inc(1);
}

void lvgl_tick_timer_init(void)
{
    const esp_timer_create_args_t timer_arg = {
        .arg = NULL,
        .callback = lv_tick_task,
        .name = "lv_tick_timer",
        .dispatch_method = ESP_TIMER_TASK,
    };

    esp_timer_create(&timer_arg,&lvgl_tick_timer);
    esp_timer_start_periodic(lvgl_tick_timer,1000);
}

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
        .clock_source = SPI_CLK_SRC_PLL_F160M,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 7,
        .pre_cb = spi_transfer_hook,
        .clock_speed_hz = 10 * 1000 * 1000,
    };

    ret = spi_bus_add_device(LCD_HOST,&devcfg,&spi2);
    ESP_ERROR_CHECK(ret);

}

void Lcd_direction(uint8_t direction)
{
    uint8_t dispcmd = 0x36;
    uint8_t dispbit = 0;
    switch(direction%4)
    {
        case 0:
        lcd_dev.width = LCD_HEIGHT;
        lcd_dev.height = LCD_WIDTH;
        dispbit = (0<<3)|(0<<6)|(1<<7);
        lcd_cmd(spi2,dispcmd,false);
        lcd_data(spi2,&dispbit,1);
        break;
        case 1:
        lcd_dev.width = LCD_WIDTH;
        lcd_dev.height = LCD_HEIGHT;
        dispbit = (0<<3)|(0<<7)|(1<<6)|(1<<5);
        lcd_cmd(spi2,dispcmd,false);
        lcd_data(spi2,&dispbit,1);
        break;
        case 2:
        lcd_dev.width = LCD_HEIGHT;
        lcd_dev.height = LCD_WIDTH;
        dispbit = (0<<3)|(1<<7)|(1<<6);
        lcd_cmd(spi2,dispcmd,false);
        lcd_data(spi2,&dispbit,1);
        break;
        case 3:
        lcd_dev.width = LCD_WIDTH;
        lcd_dev.height = LCD_HEIGHT;
        dispbit = (0<<3)|(1<<7)|(1<<5);
        lcd_cmd(spi2,dispcmd,false);
        lcd_data(spi2,&dispbit,1);
        break;
    }
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
    {0xF2, {0x00}, 1},
    /* Gamma set, curve 1 */
    {0x26, {0x01}, 1},
    /* Positive gamma correction */
    {0xE0, {0x0F, 0x35, 0x31, 0x0B, 0x0E, 0x06, 0x49, 0XA7, 0x33, 0x07, 0x0F, 0x03, 0x0C, 0x0A, 0x00}, 15},
    /* Negative gamma correction */
    {0XE1, {0x00, 0x0A, 0x0F, 0x04, 0x11, 0x08, 0x36, 0x58, 0x4D, 0x07, 0x10, 0x0C, 0x32, 0x34, 0x0F}, 15},
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

void draw_finish(spi_device_handle_t spi)
{
    spi_transaction_t *rtrans;
    esp_err_t ret;
    //Wait for all 6 transactions to be done and get back the results.
    for (int x = 0; x < 6; x++) {
        ret = spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
        assert(ret == ESP_OK);
        //We could inspect rtrans now if we received any info back. The LCD is treated as write-only, though.
    }
}

void draw_rect(spi_device_handle_t spi,int xpos, int ypos, int width, int height, uint16_t *linedata)
{
    esp_err_t ret;
    int x;
    //Transaction descriptors. Declared static so they're not allocated on the stack; we need this memory even when this
    //function is finished because the SPI driver needs access to it even while we're already calculating the next line.
    static spi_transaction_t trans[6];

    //In theory, it's better to initialize trans and data only once and hang on to the initialized
    //variables. We allocate them on the stack, so we need to re-init them each call.
    for (x = 0; x < 6; x++) {
        memset(&trans[x], 0, sizeof(spi_transaction_t));
        if ((x & 1) == 0) {
            //Even transfers are commands
            trans[x].length = 8;
            trans[x].user = (void*)0;
        } else {
            //Odd transfers are data
            trans[x].length = 8 * 4;
            trans[x].user = (void*)1;
        }
        trans[x].flags = SPI_TRANS_USE_TXDATA;
    }
    trans[0].tx_data[0] = 0x2A;         //Column Address Set
    trans[1].tx_data[0] = xpos >> 8;            //Start Col High
    trans[1].tx_data[1] = xpos & 0xff;            //Start Col Low
    trans[1].tx_data[2] = (xpos+width-1) >> 8;   //End Col High
    trans[1].tx_data[3] = (xpos+width-1) & 0xff; //End Col Low
    trans[2].tx_data[0] = 0x2B;         //Page address set
    trans[3].tx_data[0] = ypos >> 8;    //Start page high
    trans[3].tx_data[1] = ypos & 0xff;  //start page low
    trans[3].tx_data[2] = (ypos + height -1) >> 8; //end page high
    trans[3].tx_data[3] = (ypos + height -1) & 0xff; //end page low
    trans[4].tx_data[0] = 0x2C;         //memory write
    trans[5].tx_buffer = linedata;      //finally send the line data
    trans[5].length = 2 * 8 * width * height;//Data length, in bits
    trans[5].flags = 0; //undo SPI_TRANS_USE_TXDATA flag

    //Queue all transactions.
    for (x = 0; x < 6; x++) {
        ret = spi_device_queue_trans(spi, &trans[x], portMAX_DELAY);
        assert(ret == ESP_OK);
    }
}

void Clean_Screen(uint16_t *line)
{
    for(int i=0;i<240/PARALLEL_LINES;i++)
    {
        if(lcd_dev.dir == 0||lcd_dev.dir==2)
        {
            draw_rect(spi2,i*PARALLEL_LINES,0,PARALLEL_LINES,lcd_dev.height,line);
        }
        else{
            draw_rect(spi2,0,i*PARALLEL_LINES,lcd_dev.width,PARALLEL_LINES,line);
        }
        
        draw_finish(spi2);
    }
}

void lcd_init(void)
{
    // uint16_t* line = (uint16_t*)malloc(320*2*16);
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
    //开启颜色反转
    lcd_cmd(spi2,0x21,false);

    ///Enable backlight
    gpio_set_level(PIN_NUM_BCKL, LCD_BK_LIGHT_ON_LEVEL);

    //调整显示方向
    Lcd_direction(lcd_dev.dir);

    ESP_LOGI(TAG,"Lcd init Finish");
    // memset(line,WHITE,320*2*16);
    // Clean_Screen(line);
    // free(line);
}



