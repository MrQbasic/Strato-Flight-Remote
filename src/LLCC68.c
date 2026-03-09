#include "LLCC68.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdio.h>

#include "esp_log.h"

static spi_device_handle_t lora_spi;

static SemaphoreHandle_t lora_irq_sem = NULL;

int timeoutCounter = 0;
int errorCounter = 0;
int packetCounter = 0;

LoraData* Data = NULL;
LLCC68_Link_Status_t Link_status = LLCC68_LINK_STATUS_DISCONNECTED;

static void llcc68_wait_busy(void){
    while(gpio_get_level(PIN_LORA_BUSY) == 1){
        vTaskDelay(1);
    }
}

static void llcc68_cmd(uint8_t *buf, size_t len) {
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

static void IRAM_ATTR lora_dio1_isr(void *arg) {
    BaseType_t higher_priority_woken = pdFALSE;
    xSemaphoreGiveFromISR(lora_irq_sem, &higher_priority_woken);
    portYIELD_FROM_ISR(higher_priority_woken);
}

void llcc68_listen(){
    lora_irq_sem = xSemaphoreCreateBinary();
    
    //setup interrupt for reciving data
    gpio_config_t dio1_conf = {
        .pin_bit_mask = (1ULL << PIN_LORA_DIO1),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type    = GPIO_INTR_POSEDGE,  // DIO1 goes high when IRQ fires
    };
    gpio_config(&dio1_conf);
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_isr_handler_add(PIN_LORA_DIO1, lora_dio1_isr, NULL);

    // 1. SetStandby
    uint8_t standby[] = {0x80, 0x00};
    llcc68_cmd(standby, sizeof(standby));

    // 2. SetPacketType - LoRa
    uint8_t pkt_type[] = {0x8A, 0x01};
    llcc68_cmd(pkt_type, sizeof(pkt_type));

    // 3. SetRfFrequency - must match TX (868MHz)
    uint64_t rfFreq = ((uint64_t)LORA_RF_FREQUENCY << 25) / LORA_RF_XTAL;
    uint8_t freq[5];
    freq[0] = 0x86;
    freq[1] = (uint8_t)((rfFreq >> 24) & 0xFF);
    freq[2] = (uint8_t)((rfFreq >> 16) & 0xFF);
    freq[3] = (uint8_t)((rfFreq >> 8)  & 0xFF);
    freq[4] = (uint8_t)((rfFreq >> 0)  & 0xFF);
    llcc68_cmd(freq, sizeof(freq));

    // 4. SetBufferBaseAddress - TX=0x00, RX=0x80
    uint8_t buf_base[] = {0x8F, 0x00, 0x80};
    llcc68_cmd(buf_base, sizeof(buf_base));

    // 5. SetModulationParams - must match TX exactly (SF7, BW125, CR4/5)
    uint8_t mod[] = {0x8B, 0x7, 0x05, 0x01, 0x00};
    llcc68_cmd(mod, sizeof(mod));

    // 6. SetPacketParams - must match TX (preamble=8, explicit header, CRC on)
    // payload length doesn't matter in explicit header mode, chip figures it out
    uint8_t pkt_params[] = {0x8C, 0x00, 0x08, 0x00, 0xFF, 0x01, 0x00};
    llcc68_cmd(pkt_params, sizeof(pkt_params));

    // 7. SetDioIrqParams - RxDone + Timeout + CRC error on DIO1
    uint8_t irq[] = {0x08,
        0x02, 0x63,  // IrqMask: RxDone(bit1) | Timeout(bit8) | CrcErr(bit6)
        0x03, 0xFF,  // DIO1 mask
        0x00, 0x00,  // DIO2 mask
        0x00, 0x00   // DIO3 mask
    };
    llcc68_cmd(irq, sizeof(irq));

    // 8. Set LoRa sync word - must match TX
    uint8_t sync[] = {0x0D, 0x07, 0x40, 0x14, 0x24};
    llcc68_cmd(sync, sizeof(sync));

    // 9. Enable RF switch for RX
    gpio_set_level(PIN_LORA_TXEN, 0);
    gpio_set_level(PIN_LORA_RXEN, 1);


    //check if threr was a packet recived
    while(1){
        // SetRx - disble timeout
        int timeout_ms = 1600000;
        uint32_t timeout_units = (timeout_ms == 0) ? 0xFFFFFF : (timeout_ms * 1000 / 15625);
        uint8_t set_rx[] = {0x82,
            (timeout_units >> 16) & 0xFF,
            (timeout_units >> 8)  & 0xFF,
            (timeout_units)       & 0xFF
        };
        llcc68_cmd(set_rx, sizeof(set_rx));


        if (xSemaphoreTake(lora_irq_sem, portMAX_DELAY) == pdPASS) {
            //read IRQ status 
            uint8_t irq_status[] = {0x12, 0x00, 0x00, 0x00};
            llcc68_cmd(irq_status, sizeof(irq_status));
            uint16_t irq_flags = (irq_status[2] << 8) | irq_status[3];

            // clear IRQ
            uint8_t clear_irq[] = {0x02, 0xFF, 0xFF};
            llcc68_cmd(clear_irq, sizeof(clear_irq));

            if (irq_flags & 0x02) {
                uint8_t get_rssi[] = {0x15, 0x00};
                llcc68_cmd(get_rssi, sizeof(get_rssi));
                printf("RSSI: %d dBm\n", -(get_rssi[1] / 2));

                // Response format: [Status, PayloadLength, RxStartBufferPointer]
                uint8_t get_buf_status[] = {0x13, 0x00, 0x00, 0x00};
                llcc68_cmd(get_buf_status, sizeof(get_buf_status));
                uint8_t len = get_buf_status[2]; // Length of received packet
                uint8_t ptr = get_buf_status[3]; // Start address in chip RAM

                // 2. Read the actual payload (Opcode 0x1E)
                // We need: Opcode + Offset + Dummy Byte + space for data
                uint8_t read_buffer[len + 3]; 
                memset(read_buffer, 0, sizeof(read_buffer));
                read_buffer[0] = 0x1E; 
                read_buffer[1] = ptr;
                read_buffer[2] = 0x00; // Mandatory dummy byte

                llcc68_cmd(read_buffer, sizeof(read_buffer));

                memcpy(Data, &read_buffer[3], len);

                Link_status = LLCC68_LINK_STATUS_CONNECTED;

                packetCounter++;

            }else if(irq_flags & 0x20){
                printf("LoRa Header Error!\n");
                Link_status = LLCC68_LINK_STATUS_ERROR;
                errorCounter++;
            }else if (irq_flags & 0x40) {
                printf("LoRa CRC Error!\n");
                Link_status = LLCC68_LINK_STATUS_ERROR;
                errorCounter++;
            } else if (irq_flags & 0x200) {
                printf("LoRa Timeout!\n");
                Link_status = LLCC68_LINK_STATUS_DISCONNECTED;
                timeoutCounter++;
            } else {
                printf("unexprected interrupt! 0x%02x\n", irq_flags);
                Link_status = LLCC68_LINK_STATUS_ERROR;
                errorCounter++; 
            }
            
        }
    }
}

static void llcc68_reset(void){
    gpio_set_level(PIN_LORA_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(PIN_LORA_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    llcc68_wait_busy();
}


bool LLCC68_init(void){

    Data = (struct LoraData*) malloc(256 * sizeof(uint8_t)); //alloc max buffer length to avoid segfaults if struct is wrong
    memset(Data, 0, 256 * sizeof(uint8_t));

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
    uint8_t standby[] = {LLCC68_SET_STANDBY_OPCODE, LLCC68_SET_STANDBY_ARG_MODE_STBY_RC};
    llcc68_cmd(standby, sizeof(standby));

    //check if the module is in standby
    uint8_t status[] = {LLCC68_OPCODE_GET_STATUS, 0x00};
    llcc68_cmd(status, sizeof(status));    
    uint8_t chip_mode     = (status[1] >> 4) & 0x07;
    if(chip_mode != 0x02){
        printf("ERROR THIS SHOULD NOT HAPPEN! CHECK YOUR WIRES!\n");    
        return false;
    }

    //Calibrate
    uint8_t callibrate[] = {0x89, 0x7F};
    llcc68_cmd(callibrate, sizeof(callibrate));
    //Calibrate Image (Europe)
    uint8_t callibrate_Image[] = {0x98, 0xD7, 0xDB};
    llcc68_cmd(callibrate_Image, sizeof(callibrate_Image));

    return true;
}