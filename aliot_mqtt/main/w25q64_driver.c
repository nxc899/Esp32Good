#include "w25q64_driver.h"
#include "driver/spi_master.h"
#include "driver/spi_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"
#include "esp_err.h"
#include "esp_log.h"


const static char *W25Q64_TAG = "w25q64_driver";

static spi_device_handle_t w25q64_handle = NULL;

uint8_t test_buf[64] = "Hello, this is a test buffer for W25Q64!";

void w25q64_init(w25q64_config_t* w25q64_cfg)
{
    spi_bus_config_t buscfg = {
        .mosi_io_num = w25q64_cfg->mosi,
        .miso_io_num = w25q64_cfg->miso,
        .sclk_io_num = w25q64_cfg->clk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST,&buscfg,SPI_DMA_CH_AUTO));
    ESP_LOGI(W25Q64_TAG,"SPI BUS INIT FINISHI");

/*
在spi_bus_add_device()处发生错误，贼傻逼,可能是没有给定queue_size
*/

    spi_device_interface_config_t devcfg = {
        .clock_source = w25q64_cfg->clk_src,
        .mode = w25q64_cfg->mode,
        .spics_io_num = w25q64_cfg->cs,
        .clock_speed_hz = w25q64_cfg->spi_freq,
        .queue_size = 7,
    };

    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST,&devcfg,&w25q64_handle));
    ESP_LOGI(W25Q64_TAG,"spi DEV init finish!");
}

void w25q64_read_device_id(w25q64_read_data_t* read_data)
{
    memset(read_data, 0,sizeof(w25q64_read_data_t));
    read_data->datalen = DEVICE_ID_DATALEN;

    spi_transaction_t w25q64_ts;
    uint8_t data[6] = {0};
    data[0] = READ_MANUFACTOR_ID;
    memset(&w25q64_ts,0,sizeof(spi_transaction_t));
    w25q64_ts.length = 8*6;
    w25q64_ts.rx_buffer = data;
    w25q64_ts.tx_buffer = data;

    for(int i=0;i<sizeof(data);i++){
        ESP_LOGI(W25Q64_TAG,"send buffer:%x",data[i]);
    }
    
    ESP_ERROR_CHECK(spi_device_transmit(w25q64_handle,&w25q64_ts));

    memcpy(read_data->buf,&data[3],DEVICE_ID_DATALEN);

}

void w25q64_read_register_status(uint8_t reg,w25q64_read_data_t* read_data)
{
    memset(read_data, 0,sizeof(w25q64_read_data_t));
    read_data->datalen = STATE_REGISTER_DATALEN;

    spi_transaction_t w25q64_ts;
    uint8_t data[2] = {0};

    switch(reg){
        case STATE_REGISTER_1:
            data[0] = CMD_READ_REGISTER_STATUS_1;
            break;
        case STATE_REGISTER_2:
            data[0] = CMD_READ_REGISTER_STATUS_2;
            break;
        
        default:
            ESP_LOGI(W25Q64_TAG,"undefined register!");
    }
    memset(&w25q64_ts,0,sizeof(spi_transaction_t));
    w25q64_ts.length = 2*8;
    w25q64_ts.tx_buffer = data;
    w25q64_ts.rx_buffer = data;

    ESP_ERROR_CHECK(spi_device_transmit(w25q64_handle,&w25q64_ts));

    memcpy(read_data->buf,&data[1],STATE_REGISTER_DATALEN);
    
}


void w25q64_read_unique_id(w25q64_read_data_t* read_data)
{
    memset(read_data, 0,sizeof(w25q64_read_data_t));
    read_data->datalen = UNIQUE_ID_DATALEN;

    spi_transaction_t w25q64_ts;
    uint8_t data[13] = {0};

    data[0] = CMD_READ_UNIQUE_ID;
    memset(&w25q64_ts,0,sizeof(w25q64_ts));

    w25q64_ts.length = 8*13;
    w25q64_ts.tx_buffer = data;
    w25q64_ts.rx_buffer = data;

    spi_device_transmit(w25q64_handle,&w25q64_ts);

    memcpy(read_data->buf,&data[5],UNIQUE_ID_DATALEN);

}

int8_t w25q64_page_write(uint16_t address,uint8_t sector_no,uint8_t block_no,uint8_t *sendbuf,uint16_t buflen)
{
    int cnt=0;
    if(buflen > 256){
        ESP_LOGE(W25Q64_TAG,"data length over 256 bytes!");
        return -1;
    }
    spi_transaction_t w25q64_ts;

    uint8_t data[4 + buflen];
    data[0] = CMD_PAGE_PROGRAM;
    data[1] = block_no;
    data[2] = (sector_no<<4)|(address>>8&0x0F);
    data[3] = (uint8_t)(address & 0xFF);

    ESP_LOGI(W25Q64_TAG,"address:%0x,%0x,%0x,%0x",data[0],data[1],data[2],data[3]);

    memcpy(&data[4],sendbuf,buflen);
    memset(&w25q64_ts, 0, sizeof(spi_transaction_t));
    w25q64_ts.length = 8 * (4 + buflen);
    w25q64_ts.tx_buffer = data;

    w25q64_write_enable();
    if(w25q64_is_busy()){
        ESP_LOGI(W25Q64_TAG,"W25Q64 is busy");
        return -1;
    }
    
    ESP_LOGI(W25Q64_TAG,"write buf:%s",test_buf);

    ESP_ERROR_CHECK(spi_device_transmit(w25q64_handle,&w25q64_ts));

    while(w25q64_is_busy()){
        vTaskDelay(pdMS_TO_TICKS(1));
        cnt++;
    }
    ESP_LOGI(W25Q64_TAG,"W25Q64 page write finish!,time cost:%d",cnt);
    return 1;
}

bool w25q64_is_busy(void)
{
    w25q64_read_data_t reg_status;
    w25q64_read_register_status(1,&reg_status);
    return (reg_status.buf[0] & 0x01);
}

void w25q64_write_enable(void)
{
    uint8_t data = CMD_WRITE_ENABLE;
    spi_transaction_t w25q64_ts;
    memset(&w25q64_ts, 0, sizeof(spi_transaction_t));
    w25q64_ts.length = 8;
    w25q64_ts.tx_buffer = &data;

    ESP_ERROR_CHECK(spi_device_transmit(w25q64_handle, &w25q64_ts));
}

int8_t w25q64_page_read(uint16_t address,uint8_t sector_no,uint8_t block_no,uint8_t *recvbuf,uint16_t buflen)
{
    if(buflen > 256){
        ESP_LOGE(W25Q64_TAG,"data length over 256 bytes!");
        return -1;
    }

    spi_transaction_t w25q64_ts;

    uint8_t data[4+buflen];
    data[0] = CMD_READ_DATA;
    data[1] = block_no;
    data[2] = (sector_no<<4)|(address>>8&0x0F);
    data[3] = (uint8_t)(address & 0xFF);

    memset(&w25q64_ts,0,sizeof(spi_transaction_t));
    w25q64_ts.length = 8 * (4 + buflen);
    w25q64_ts.tx_buffer = data;
    w25q64_ts.rx_buffer = data;

    ESP_ERROR_CHECK(spi_device_transmit(w25q64_handle,&w25q64_ts));

    memcpy(recvbuf,&data[4],buflen);

    return 1;
}

void w25q64_erase_sector(uint16_t address,uint8_t sector_no,uint8_t block_no)
{
    uint8_t cnt = 0;
    spi_transaction_t w25q64_ts;

    uint8_t data[4];
    data[0] = CMD_SECTOR_ERASE;
    data[1] = block_no;
    data[2] = (sector_no<<4)|(address>>8&0x0F);
    data[3] = (uint8_t)(address & 0xFF);
    
    w25q64_write_enable();

    memset(&w25q64_ts,0,sizeof(spi_transaction_t));
    w25q64_ts.length = 8 * 4;
    w25q64_ts.tx_buffer = data;

    ESP_ERROR_CHECK(spi_device_transmit(w25q64_handle,&w25q64_ts));

    while(w25q64_is_busy()){
        vTaskDelay(pdMS_TO_TICKS(1));
        cnt++;
    }
    ESP_LOGI(W25Q64_TAG,"W25Q64 sector erase finish!,time cost:%d",cnt);
}
