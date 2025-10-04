#ifndef FT6336G_H
#define FT6336G_H
#include "stdint.h"

//FT5426 部分寄存器定义 
#define FT_DEVIDE_MODE           0x00         //FT6336模式控制寄存器
#define FT_REG_NUM_FINGER        0x02         //触摸状态寄存器
 
#define FT_TP1_REG               0X03         //第一个触摸点数据地址
#define FT_TP2_REG               0X09         //第二个触摸点数据地址
 
#define FT_ID_G_CIPHER_MID       0x9F         //芯片代号（中字节） 默认值0x26
#define FT_ID_G_CIPHER_LOW       0xA0         //芯片代号（低字节） 0x01: Ft6336G  0x02: Ft6336U 
#define FT_ID_G_LIB_VERSION      0xA1         //版本
#define FT_ID_G_CIPHER_HIGH      0xA3         //芯片代号（高字节） 默认0x64 
#define FT_ID_G_MODE             0xA4         //FT6636中断模式控制寄存器
#define FT_ID_G_PMODE            0XA5         //设置功耗
#define FT_ID_G_FOCALTECH_ID     0xA8         //VENDOR ID 默认值为0x11
#define FT_ID_G_THGROUP          0x80         //触摸有效值设置寄存器
#define FT_ID_G_PERIODACTIVE     0x88         //激活状态周期设置寄存器

// #define ENABLE_TOUCH_TEST 0

#define ARR_SIZE(a) sizeof(a)/sizeof(a[0])

typedef struct{
    uint8_t sta;    //从触摸状态寄存器读取的值
    uint16_t x;
    uint16_t y;
    uint8_t (*touch_status)(void);
    void (*touch_init)(void);
}touch_dev_t;

extern touch_dev_t touch_dev;

void touch_status(void);
void touch_init(void);
void get_touch_pos(void);

#endif