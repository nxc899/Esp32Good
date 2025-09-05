#include "bsp_i2c.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

/*************************************** 
 * 定义ath20和bmp280硬件接口
*/
#define I2C_MASTER_SCL_IO   GPIO_NUM_6
#define I2C_MASTER_SDA_IO   GPIO_NUM_7
#define I2C_MASTER_NUM  I2C_NUM_0

#define DEVICE_ADDR_BMP280  0x77
#define DEVICE_ADDR_AHT20  0x38

static const char* I2CTAG="bsp_i2c";

static i2c_master_bus_handle_t i2c_bus = NULL;
static i2c_master_dev_handle_t i2c_dev_aht20 = NULL;
static i2c_master_dev_handle_t i2c_dev_bmp280 = NULL;

/*定义bmp280寄存器地址*/
#define BMP280_TEMP_XLSB 0xFC
#define BMP280_TEMP_LSB  0xFB
#define BMP280_TEMP_MSB  0xFA
#define BMP280_PRESS_XLSB 0xF9
#define BMP280_PRESS_LSB  0xF8
#define BMP280_PRESS_MSB  0xF7
#define BMP280_CONFIG       0xF5
#define BMP280_CTRL_MEAS    0xF4  
#define BMP280_STATUS       0xF3  
#define BMP280_RESET        0xE0  
#define BMP280_CHIP_ID      0xD0

#define BMP280_SLEEP_MODE 0x00 //ctrl_meas register 00:sleep 01 and 10:force mode 11:Normal mode
#define BMP280_FORCE_MODE 0x01
#define BMO280_NORMAL_MODE 0X03

#define OVERSAMPLING_SKIPPED 0x00  //OVERSAMPLING SETTINGS
#define OVERSAMPLING_1X      0x01 
#define OVERSAMPLING_2X      0x02 
#define OVERSAMPLING_4X      0x03
#define OVERSAMPLING_8X      0x04
#define OVERSAMPLING_16X     0x05 

#define BMP280_IIRFILTER_SET 0x04

#define BMP280_CALIB_DATA_START      0x88

typedef struct{
    uint8_t id;
    uint8_t add;
    uint8_t mode;
    uint8_t temperature_oversampling;
    uint8_t pressure_oversampling;
    uint8_t spi_enable;
    uint8_t filter;
}bmp280_config_t;



typedef struct 
{
    uint16_t dig_T1;                                                                /* calibration T1 data */
    int16_t dig_T2;                                                                /* calibration T2 data */
    int16_t dig_T3;                                                                /* calibration T3 data */
    uint16_t dig_P1;                                                                /* calibration P1 data */
    int16_t dig_P2;                                                                /* calibration P2 data */
    int16_t dig_P3;                                                                /* calibration P3 data */
    int16_t dig_P4;                                                                /* calibration P4 data */
    int16_t dig_P5;                                                                /* calibration P5 data */
    int16_t dig_P6;                                                                /* calibration P6 data */
    int16_t dig_P7;                                                                /* calibration P7 data */
    int16_t dig_P8;                                                                /* calibration P8 data */
    int16_t dig_P9;                                                                /* calibration P9 data */
    int32_t t_fine;                                                               /* calibration t_fine data */
} bmp280Calib;

bmp280Calib calib = {0};
bmp280_device bmp280 = {0};

static void bmp280_data_raw(bmp280_device* bmp280);
static int32_t bmp280_compensate_t(int32_t adc_T);
static int32_t bmp280_compensate_p(int32_t adc_P);
void bsp_i2c_init(void)
{
    i2c_master_bus_config_t buscfg={
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .i2c_port = I2C_MASTER_NUM,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&buscfg,&i2c_bus));

    i2c_device_config_t aht20cfg={
        .device_address = DEVICE_ADDR_AHT20,
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .scl_speed_hz = 100000
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus,&aht20cfg,&i2c_dev_aht20));

    i2c_device_config_t bmp280cfg={
        .device_address = DEVICE_ADDR_BMP280,
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .scl_speed_hz = 100000
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus,&bmp280cfg,&i2c_dev_bmp280));
    ESP_LOGI(I2CTAG,"I2C bus opened successfully");
}

int i2c_read_ath20_data(float* humidity,float* temperature)
{
    uint8_t humitemp_data[6] = {0};
    uint8_t read_cmd[3] = {0xAC,0x33,0x00};
    //uint8_t check_cmd = 0x71;
    uint32_t data = 0;

    //发送测量命令
    i2c_master_transmit(i2c_dev_aht20,read_cmd,sizeof(read_cmd),10);
    vTaskDelay(pdMS_TO_TICKS(75));
    //手册说延迟75ms后就可以读，就不检测状态位了
    //接收的数据第0位是状态，后5位是温湿度数据的拼接
    i2c_master_receive(i2c_dev_aht20,humitemp_data,sizeof(humitemp_data),10);
    if((humitemp_data[0]&0x80) == 0x00)
    {
        ESP_LOGI(I2CTAG,"ath20 data not busy ,status:%x",humitemp_data[0]);

    }
    else
    {
        ESP_LOGE(I2CTAG,"ath20 busy");
        return -1;
    }

    data = ((humitemp_data[1]<<12)|(humitemp_data[2]<<4)|(humitemp_data[3]>>4));
    *humidity = data*100.0f/(1<<20);
    data = (((humitemp_data[3]&0x0f)<<16)|(humitemp_data[4]<<8)|(humitemp_data[5]));
    *temperature = data*200.0f/(1<<20)-50;
    return 0;
}


int ath20_init(void)
{
    uint8_t state = 0;
    uint8_t tx_data = 0x71;
    if(ESP_OK == i2c_master_probe(i2c_bus,DEVICE_ADDR_AHT20,10))
    {
        ESP_LOGI(I2CTAG,"ath20 i2c probe succeed.");
    }    
    if(ESP_OK == i2c_master_probe(i2c_bus,DEVICE_ADDR_BMP280,10))
    {
        ESP_LOGI(I2CTAG,"bmp280 i2c probe succeed.");
    }
    vTaskDelay(pdMS_TO_TICKS(400));
    ESP_ERROR_CHECK(i2c_master_transmit_receive(i2c_dev_aht20,&tx_data,sizeof(uint8_t),&state,sizeof(uint8_t),10));
    ESP_LOGI(I2CTAG,"after send 0x71,get:%x",state);
    if((state&0x08) != 0x08)
    {
        uint8_t sendbuff[3] = {0xBE,0x80,0x00};
        ESP_LOGI(I2CTAG,"AHT20 need to initialize first.");
        i2c_master_transmit(i2c_dev_aht20,sendbuff,sizeof(sendbuff),10);
        ESP_LOGI(I2CTAG,"ath20 init succeed.");
    }
    return 0;
}


int bmp280_init(void)
{
    bmp280_config_t *bmp = (bmp280_config_t*)malloc(sizeof(bmp280_config_t));
    

    bmp->id = BMP280_CHIP_ID;

    i2c_master_transmit_receive(i2c_dev_bmp280,&(bmp->id),sizeof(uint8_t),&(bmp->id),sizeof(uint8_t),10);
    ESP_LOGI(I2CTAG,"%x read bmp_id",bmp->id);

    if(bmp == NULL)
    {
        ESP_LOGE(I2CTAG,"calib allocate failed");
        return -1;
    }
    uint8_t buf[6] = {0};
    bmp->mode = BMO280_NORMAL_MODE;
    bmp->pressure_oversampling = OVERSAMPLING_2X;
    bmp->temperature_oversampling = OVERSAMPLING_2X;
    bmp->spi_enable = 0;
    bmp->filter = BMP280_IIRFILTER_SET;
    bmp->add = DEVICE_ADDR_BMP280;
    bmp->id = 0x58;
    buf[0] = BMP280_CALIB_DATA_START;
    //读取校准数据
    i2c_master_transmit_receive(i2c_dev_bmp280,buf,sizeof(uint8_t),(uint8_t*)&calib,sizeof(bmp280Calib),10);//read calibration data
    //configuration
    buf[0] = BMP280_CTRL_MEAS;
    buf[1] = bmp->mode|bmp->pressure_oversampling<<2|bmp->temperature_oversampling<<5;
    ESP_LOGI(I2CTAG,"%x write to ctrl_meas!",(bmp->mode|bmp->pressure_oversampling<<2|bmp->temperature_oversampling<<5));
    i2c_master_transmit(i2c_dev_bmp280,buf,2,10);
    buf[0] = BMP280_CONFIG;
    buf[1] = (bmp->filter)<<2;
    ESP_LOGI(I2CTAG,"%x write to config!",((bmp->filter)<<2));
    i2c_master_transmit(i2c_dev_bmp280,buf,2,10);
    free(bmp);
    return 0;
}

void i2c_read_bmp280_data(bmp280_device* bmp280)
{
    bmp280_data_raw(bmp280);
    bmp280->temperature = bmp280_compensate_t(bmp280->raw_temperature)/100.0f;
    bmp280->pressure = (bmp280_compensate_p(bmp280->raw_pressure)>>8)/100.0f;
    return;
}

static void bmp280_data_raw(bmp280_device* bmp280)
{
    uint8_t data_raw[6];
    uint8_t digit_register = BMP280_PRESS_MSB;
    i2c_master_transmit_receive(i2c_dev_bmp280,&digit_register,sizeof(uint8_t),data_raw,sizeof(data_raw),10);
    bmp280->raw_pressure = (uint32_t)data_raw[0]<<12|(uint32_t)data_raw[1]<<4|(uint32_t)data_raw[2]>>4;
    bmp280->raw_temperature = (uint32_t)data_raw[3]<<12|(uint32_t)data_raw[4]<<4|(uint32_t)data_raw[5]>>4;
}

static int32_t bmp280_compensate_t(int32_t adc_T)
{
    int32_t var1,var2,t;
    var1 = ((((adc_T>>3)-((int32_t)calib.dig_T1<<1)))*((int32_t)calib.dig_T2))>>11;
    var2 = (((((adc_T>>4)-((int32_t)calib.dig_T1))*((adc_T>>4)-((int32_t)calib.dig_T1)))>>12)*((int32_t)calib.dig_T3))>>14;
    calib.t_fine = var1+var2;
    t = (calib.t_fine * 5 + 128) >> 8;
    return t;
}

static int32_t bmp280_compensate_p(int32_t adc_P)
{
    int64_t var1,var2,p;
    var1 = ((int64_t)calib.t_fine)-128000;
    var2 = var1*var1*(int64_t)calib.dig_P6;
    var2 = var2+((var1*(int64_t)calib.dig_P5)<<17);
    var2 = var2+(((int64_t)calib.dig_P4)<<35);
    var1 = ((var1*var1*(int64_t)calib.dig_P3)>>8)+((var1*(int64_t)calib.dig_P2)<<12);
    var1 = (((((int64_t)1)<<47)+var1))*((int64_t)calib.dig_P1)>>33;
    if(var1 == 0)
    {
        return 0;
    }
    p = 1048576-adc_P;
    p = (((p<<31)-var2)*3125)/var1;
    var1 = (((int64_t)calib.dig_P9)*(p>>13)*(p>>13))>>25;
    var2 = (((int64_t)calib.dig_P8)*p)>>19;
    p = ((p+var1+var2)>>8)+(((int64_t)calib.dig_P7)<<4);
    return (int32_t)p;
}
