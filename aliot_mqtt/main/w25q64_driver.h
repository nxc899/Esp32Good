#ifndef __W25Q64_DRIVER_H__
#define __W25Q64_DRIVER_H__

#include "driver/gpio.h"
#include "esp_err.h"
#include "driver/spi_master.h"

#define READ_MANUFACTOR_ID 0x90
#define CMD_READ_UNIQUE_ID 0x4B
#define CMD_READ_REGISTER_STATUS_1 0x05
#define CMD_READ_REGISTER_STATUS_2 0x35
#define CMD_READ_REGISTER_STATUS_3 0x15
#define CMD_WRITE_ENABLE 0x06
#define CMD_WRITE_DISABLE 0x04
#define CMD_READ_DATA 0x03
#define CMD_PAGE_PROGRAM 0x02
#define CMD_WRITE_ENABLE 0x06
#define CMD_SECTOR_ERASE 0x20

#define STATE_REGISTER_1 0
#define STATE_REGISTER_2 1

#define STATE_REGISTER_DATALEN 1
#define UNIQUE_ID_DATALEN 8
#define DEVICE_ID_DATALEN 3



typedef struct{
    uint8_t datalen;
    uint8_t buf[16];
}w25q64_read_data_t;

typedef struct{
    spi_host_device_t host;
    gpio_num_t mosi;
    gpio_num_t miso;
    gpio_num_t clk;
    gpio_num_t cs;
    uint32_t spi_freq;
    uint8_t mode;
    spi_clock_source_t clk_src;
}w25q64_config_t;

extern uint8_t test_buf[64];

void w25q64_init(w25q64_config_t* w25q64_cfg);
void w25q64_read_device_id(w25q64_read_data_t* read_data);
void w25q64_read_unique_id(w25q64_read_data_t* read_data);
void w25q64_read_register_status(uint8_t reg,w25q64_read_data_t* read_data);
bool w25q64_is_busy(void);
void w25q64_write_enable(void);
int8_t w25q64_page_write(uint16_t address,uint8_t sector_no,uint8_t block_no,uint8_t *sendbuf,uint16_t buflen);
int8_t w25q64_page_read(uint16_t address,uint8_t sector_no,uint8_t block_no,uint8_t *recvbuf,uint16_t buflen);
void w25q64_erase_sector(uint16_t address,uint8_t sector_no,uint8_t block_no);

#endif
