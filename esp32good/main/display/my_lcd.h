#ifndef _MY_LCD_H
#define _MY_LCD_H
#include <stdlib.h>
#include "driver/spi_master.h"
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

#define LCD_WIDTH 320
#define LCD_HEIGHT 240
#define LCD_VERTICAL 0 //0:Vertical 1:hroizental
#define SETX_CMD 0x2A
#define SETY_CMD 0x2B
#define READ_RAM_CMD 0x2E
#define WRITE_RAM_CMD 0x2C

//画笔颜色,背景颜色
// #define WHITE          0xFFFF
// #define BLACK          0x0000
// #define RED            0xF800
// #define GREEN          0x07E0
// #define BLUE           0x001F
// #define BRED           0XF81F
// #define GRED           0XFFE0
// #define GBLUE          0X07FF
// #define BROWN          0XBC40
// #define BRRED          0XFC07
// #define GRAY           0X8430

//以这个点为中心画圆点
#define LCD_DRAW_POINT(x,y,color,pointsize) \
    do{\
        draw_rect(spi2,x-pointsize/2,y-pointsize/2,pointsize,pointsize,color); \
    }while(0);

#define LCD_DRAW_VLINE(x,y,height,color) \
    do{\
        draw_rect(spi2,x,y,1,height,color); \
    }while(0);

#define LCD_DRAW_HLINE(x,y,width,color) \
    do{\
        draw_rect(spi2,x,y,width,1,color); \
    }while(0);


typedef struct
{
    uint8_t command;
    uint8_t data[16];
    uint8_t databyte;
}lcd_init_cmd_t; 

//LCD重要参数集
typedef struct  
{										    
	uint16_t width;			//LCD 宽度
	uint16_t height;			//LCD 高度
	uint8_t  dir;			  //横屏还是竖屏控制：0，竖屏；1，横屏。	
}_lcd_dev; 	

void lcd_init(void);
void draw_rect(spi_device_handle_t spi,int xpos, int ypos, int width, int height, uint16_t *linedata);
void draw_finish(spi_device_handle_t spi);
void lvgl_tick_timer_init(void);

extern _lcd_dev lcd_dev;
extern spi_device_handle_t spi2;
extern uint16_t drawbuf[320*240];

#endif // _MY_LCD_H