#include "LLCC68.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

#include <string.h>
#include <stdio.h>

#include "esp_log.h"

static spi_device_handle_t lora_spi;

static void llcc68_wait_busy(void){
    while(gpio_get_level(PIN_LORA_BUSY) == 1){
        vTaskDelay(1);
    }
}

void llcc68_cmd(uint8_t *buf, size_t len) {
    uint8_t tx_buf[len];
    memcpy(tx_buf, buf, len);

    llcc68_wait_busy();
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = tx_buf,
        .rx_buffer = buf,
    };

    gpio_set_level(PIN_LORA_NSS, 0);
    spi_device_transmit(lora_spi, &t);
    gpio_set_level(PIN_LORA_NSS, 1);
}


uint8_t imageCalibrationTable [5][2] = {
    {0x6B, 0x6F}, // 430-440 MHz
    {0x75, 0x81}, // 470-510 MHz
    {0xC1, 0xC5}, // 779-787 MHz
    {0xD7, 0xDB}, // 863-870 MHz
    {0xE1, 0xE9}  // 902-928 MHz
};

void llcc68_calibrate_image(LLCC68_FREQUENCY_BAND_t frequency_band){
    if(frequency_band > 5) return; //invalid frequency band
    uint8_t cmd[] = {0x98, imageCalibrationTable[frequency_band][0], imageCalibrationTable[frequency_band][1]};
    llcc68_cmd(cmd, sizeof(cmd));
}

void llcc68_calibrate(uint8_t callibration_Setting){
    uint8_t cmd[] = {0x98, callibration_Setting & 0x7F};
    llcc68_cmd(cmd, sizeof(cmd));
}

uint8_t llcc68_getStatus(){
    uint8_t cmd[] = {0xC0, 0x00};
    llcc68_cmd(cmd, sizeof(cmd));    
    return ((cmd[1] & 0b1110) >> 1) | (cmd[1] & 0b01110000); //rearrange bits to remove reserved bits
}

void llcc68_setFrequency(uint32_t frequency){
    uint64_t rfFreq = ((uint64_t)frequency << 25) / LORA_RF_XTAL;
    uint8_t cmd[5];
    cmd[0] = 0x86;
    cmd[1] = (uint8_t)((rfFreq >> 24) & 0xFF);
    cmd[2] = (uint8_t)((rfFreq >> 16) & 0xFF);
    cmd[3] = (uint8_t)((rfFreq >> 8)  & 0xFF);
    cmd[4] = (uint8_t)((rfFreq >> 0)  & 0xFF);
    llcc68_cmd(cmd, sizeof(cmd));
}

void llcc68_setModulationParams(LLCC68_MODULATION_SF_t sf, LLCC68_MODULATION_BW_t bw, LLCC68_MODULATION_CR_t cr, bool low_data_rate_optimization){
    uint8_t cmd[] = {0x8B, sf, bw, cr, (low_data_rate_optimization ? 0x01 : 0x00)};
    llcc68_cmd(cmd, sizeof(cmd));
}

void llcc68_setPacketType(LLCC68_PACKET_TYPE_t type){
    uint8_t cmd[] = {0x8A, type};
    llcc68_cmd(cmd, sizeof(cmd));
}

void llcc68_setStandby(LLCC68_STANDBY_MODE_t mode){
    uint8_t cmd[] = {0x80, mode};
    llcc68_cmd(cmd, sizeof(cmd));
}

void llcc68_setBufferBaseAddress(uint8_t tx_base, uint8_t rx_base){
    uint8_t cmd[] = {0x8F, tx_base, rx_base};
    llcc68_cmd(cmd, sizeof(cmd));
}

void llcc68_setPacketParams_Lora(uint16_t peramble_length, bool implicit_header, uint8_t payload_length, bool crc_enable, bool invert_iq){
    uint8_t cmd[] = {
        0x8C,
        (peramble_length >> 8) & 0xFF,
        (peramble_length >> 0) & 0xFF,
        (implicit_header ? 0x01 : 0x00),
        payload_length,
        (crc_enable ? 0x01 : 0x00),
        (invert_iq ? 0x01 : 0x00)
    };
    llcc68_cmd(cmd, sizeof(cmd));
}


void llcc68_rx(int timeout_ms){
    gpio_set_level(PIN_LORA_TXEN, 0);
    gpio_set_level(PIN_LORA_RXEN, 1);

    llcc68_setBufferBaseAddress(0x00, 0x00);

    uint32_t timeout_units = (timeout_ms == 0) ? 0xFFFFFF : (timeout_ms * 1000 * 1000 / 15625);
    uint8_t cmd[] = {0x82,
        (timeout_units >> 16) & 0xFF,
        (timeout_units >> 8)  & 0xFF,
        (timeout_units)       & 0xFF
    };
    llcc68_cmd(cmd, sizeof(cmd));
}

void llcc68_tx(void* data, uint8_t length){
    gpio_set_level(PIN_LORA_TXEN, 1);
    gpio_set_level(PIN_LORA_RXEN, 0);

    llcc68_buffer_write(0x00, (uint8_t*) data, length);

    llcc68_setBufferBaseAddress(0x00, 0x00);

    llcc68_setPacketParams_Lora(8, false, length, true, false);

    uint8_t tx_cmd[] = {0x83, 0x00, 0x00, 0x00};
    llcc68_cmd(tx_cmd, sizeof(tx_cmd));
}


void llcc68_buffer_read(uint8_t offset, uint8_t* buf, uint8_t length){
    //alloc a buffer for the read
    uint8_t readBuffer[length + 3];
    memset(readBuffer, 0, sizeof(readBuffer));
    //set the header
    readBuffer[0] = 0x1E; 
    readBuffer[1] = offset;
    readBuffer[2] = 0x00;    //dummy byte
    //read the bytes
    llcc68_cmd(readBuffer, sizeof(readBuffer));
    //copy the data to the buffer
    memcpy(buf, &(readBuffer[3]), length);
}


void llcc68_buffer_write(uint8_t offset, uint8_t* buf, uint8_t length){
    //alloc a buffer for the write
    uint8_t writeBuffer[length + 2];
    memset(writeBuffer, 0, sizeof(writeBuffer));
    //set the header
    writeBuffer[0] = 0x0E; 
    writeBuffer[1] = offset;
    //copy the data to the buffer
    memcpy(&(writeBuffer[2]), buf, length);
    //write the bytes
    llcc68_cmd(writeBuffer, sizeof(writeBuffer));
}


float llcc68_getRSSI(){
    uint8_t cmd[] = {0x15, 0x00, 0x00};
    llcc68_cmd(cmd, sizeof(cmd));
    return -(cmd[2] / 2.0f);
}


void llcc68_setPaConfig(uint8_t paDutyCycle, uint8_t paHpMax){
    uint8_t cmd[] = {0x95, paDutyCycle, paHpMax, 0x00, 0x01};
    llcc68_cmd(cmd, sizeof(cmd));
}

static void llcc68_reset(void){
    gpio_set_level(PIN_LORA_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(PIN_LORA_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    llcc68_wait_busy();
}


bool LLCC68_init(void){
    gpio_set_direction(PIN_LORA_NSS, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_LORA_NSS, 1);

    gpio_set_direction(PIN_LORA_BUSY, GPIO_MODE_INPUT);

    gpio_set_direction(PIN_LORA_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_LORA_RST, 1);

    gpio_set_direction(PIN_LORA_TXEN, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_LORA_RXEN, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_LORA_TXEN, 0);
    gpio_set_level(PIN_LORA_RXEN, 0);

    //configure SPI bus 3
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_LORA_MISO,
        .mosi_io_num = PIN_LORA_MOSI,
        .sclk_io_num = PIN_LORA_SCK,
        .quadhd_io_num = -1, 
        .quadwp_io_num = -1,
    };
    esp_err_t error = spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if(error != ESP_OK) return false;

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 8 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = -1, //handled manually
        .queue_size = 1,
    };
    error = spi_bus_add_device(SPI3_HOST, &devcfg, &lora_spi);
    if(error != ESP_OK) return false;


    llcc68_reset();

    //set it into standby
    llcc68_setStandby(LLCC68_STANDBY_RC);

    //check if the module is in standby_RC
    if((llcc68_getStatus() >> 4) != LLCC68_STATUS_CHIPMODE_STBY_RC){
        printf("ERROR THIS SHOULD NOT HAPPEN! CHECK YOUR WIRES!\n");
        return false;
    }

    //Calibrate
    llcc68_calibrate(LLCC68_CALLIBRATE_ALL);

    //Calibrate Image (Europe)
    llcc68_calibrate_image(LLCC68_FREQUENCY_BAND_863_870);

    //setup params for communication
    vTaskDelay(pdMS_TO_TICKS(100));

    llcc68_setStandby(LLCC68_STANDBY_RC); // Use crystal oscillator for better stability during reception

    llcc68_setPacketType(LLCC68_PACKET_TYPE_LORA);

    llcc68_setFrequency(LORA_RF_FREQUENCY);

    llcc68_setModulationParams(LLCC68_MODULATION_SF_7, LLCC68_MODULATION_BW_125_KHZ, LLCC68_MODULATION_CR_4_8, true);

    llcc68_setPaConfig(0x01, 0x01); //max recommended power

    //set ocp
    uint8_t ocp_set[] = {0x0D, 0x08, 0xE7, 0x38}; //140mA
    llcc68_cmd(ocp_set, sizeof(ocp_set));

    return true;
}