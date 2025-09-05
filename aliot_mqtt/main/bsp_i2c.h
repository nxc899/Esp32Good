#ifndef __BSP_I2C_H__
#define __BSP_I2C_H__
#include <stdint.h>

typedef struct{
    float pressure;
    float temperature;
    int32_t raw_temperature;
    int32_t raw_pressure;
}bmp280_device;

extern bmp280_device bmp280;

void bsp_i2c_init(void);
int ath20_init(void);
int i2c_read_ath20_data(float* humidity,float* temperature);
int bmp280_init(void);
void i2c_read_bmp280_data(bmp280_device* bmp280);





#endif //__BSP_I2C_H__


