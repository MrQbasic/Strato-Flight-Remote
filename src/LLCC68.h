#pragma once

// Pin definitions
#define PIN_LORA_NSS 21
#define PIN_LORA_RST 19
#define PIN_LORA_SCK  22
#define PIN_LORA_MOSI 23
#define PIN_LORA_MISO 4
#define PIN_LORA_DIO1 13
#define PIN_LORA_DIO2 12
#define PIN_LORA_BUSY 14
#define PIN_LORA_RXEN 27
#define PIN_LORA_TXEN 26

#define LORA_RF_XTAL                                32000000
#define LORA_RF_FREQUENCY                           868000000 // Hz

//OPCODES
#define LLCC68_SET_STANDBY_OPCODE               0x80
#define LLCC68_SET_STANDBY_ARG_MODE_STBY_RC     0x00
#define LLCC68_SET_STANDBY_ARG_MODE_STBY_XOSC   0x01
//-
#define LLCC68_OPCODE_GET_STATUS         0xC0


#include <stdbool.h>
#include <stdint.h>

typedef enum {
    LLCC68_LINK_STATUS_DISCONNECTED = 0,
    LLCC68_LINK_STATUS_CONNECTED = 1,
    LLCC68_LINK_STATUS_ERROR = 2,
} LLCC68_Link_Status_t;

typedef struct LoraData {
    float temp_1;
    float temp_2;
    float pressure;
    float humidity;
    float uv;
    double gps_lat;
    double gps_lon;
    double gps_alt;
} __attribute__((packed)) LoraData;

extern LoraData* Data;
extern LLCC68_Link_Status_t Link_status;
extern int timeoutCounter;
extern int errorCounter;
extern int packetCounter;
extern int packetRSSI;

typedef enum{
    LLCC68_FREQUENCY_BAND_430_440 = 0,
    LLCC68_FREQUENCY_BAND_470_510 = 1,
    LLCC68_FREQUENCY_BAND_779_787 = 2,
    LLCC68_FREQUENCY_BAND_863_870 = 3,
    LLCC68_FREQUENCY_BAND_902_928 = 4,
} LLCC68_FREQUENCY_BAND_t;

void llcc68_calibrate_image(LLCC68_FREQUENCY_BAND_t frequency_band);


#define LLCC68_CALLIBRATE_ALL           0b01111111
#define LLCC68_CALLIBRATE_RC64          0b00000001
#define LLCC68_CALLIBRATE_RC13M         0b00000010
#define LLCC68_CALLIBRATE_PLL           0b00000100
#define LLCC68_CALLIBRATE_ADC_PULSE     0b00001000
#define LLCC68_CALLIBRATE_ADC_BULK_N    0b00010000
#define LLCC68_CALLIBRATE_ADC_BULK_P    0b00100000
#define LLCC68_CALLIBRATE_IMAGE         0b01000000

void llcc68_calibrate(uint8_t callibration_Setting);


#define LLCC68_STATUS_CMDSTATUS_RX_OK    0x02 // Packet was received and data is in buffer
#define LLCC68_STATUS_CMDSTATUS_TIMEOUT  0x03 // Timeout of command occurred
#define LLCC68_STATUS_CMDSTATUS_ERROR    0x04 // Error occurred during command execution (for example invalid opcode)
#define LLCC68_STATUS_CMDSTATUS_FAILED   0x05 // Command failed (for example if you try to transmit but the channel is busy)
#define LLCC68_STATUS_CMDSTATUS_TX_DONE  0x06 // Transmission completed
#define LLCC68_STATUS_CHIPMODE_STBY_RC   0x02  
#define LLCC68_STATUS_CHIPMODE_STBY_XOSC 0x03
#define LLCC68_STATUS_CHIPMODE_FS        0x04
#define LLCC68_STATUS_CHIPMODE_RX        0x05
#define LLCC68_STATUS_CHIPMODE_TX        0x06

//bit 0-3: command status, 4-7: chip mode
uint8_t llcc68_getStatus();


void llcc68_setFrequency(uint32_t frequency);


typedef enum {
    LLCC68_MODULATION_BW_125_KHZ = 0x04,
    LLCC68_MODULATION_BW_250_KHZ = 0x05,
    LLCC68_MODULATION_BW_500_KHZ = 0x06,
} LLCC68_MODULATION_BW_t;

typedef enum {
    LLCC68_MODULATION_CR_4_5    = 0x01,
    LLCC68_MODULATION_CR_4_6    = 0x02,
    LLCC68_MODULATION_CR_4_7    = 0x03,
    LLCC68_MODULATION_CR_4_8    = 0x04,
    LLCC68_MODULATION_CR_4_5_LI = 0x05,
    LLCC68_MODULATION_CR_4_6_LI = 0x06,
    LLCC68_MODULATION_CR_4_7_LI = 0x07,
    LLCC68_MODULATION_CR_4_8_LI = 0x08,
} LLCC68_MODULATION_CR_t;

typedef enum {
    LLCC68_MODULATION_SF_5 = 0x05,
    LLCC68_MODULATION_SF_6 = 0x06,
    LLCC68_MODULATION_SF_7 = 0x07,
    LLCC68_MODULATION_SF_8 = 0x08,
    LLCC68_MODULATION_SF_9 = 0x09,
    LLCC68_MODULATION_SF_10 = 0x0A,
    LLCC68_MODULATION_SF_11 = 0x0B,
} LLCC68_MODULATION_SF_t;

void llcc68_setModulationParams(LLCC68_MODULATION_SF_t sf, LLCC68_MODULATION_BW_t bw, LLCC68_MODULATION_CR_t cr, bool low_data_rate_optimization);

typedef enum {
    LLCC68_PACKET_TYPE_GFSK = 0x00,
    LLCC68_PACKET_TYPE_LORA = 0x01,
} LLCC68_PACKET_TYPE_t;

void llcc68_setPacketType(LLCC68_PACKET_TYPE_t type);

bool LLCC68_init(void);

void llcc68_listen();