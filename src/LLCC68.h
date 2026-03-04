#pragma once

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
#define LORA_SF 7f

//OPCODES
#define LLCC68_SET_STANDBY_OPCODE               0x80
#define LLCC68_SET_STANDBY_ARG_MODE_STBY_RC     0x00
#define LLCC68_SET_STANDBY_ARG_MODE_STBY_XOSC   0x01
//-
#define LLCC68_OPCODE_GET_STATUS                0xC0
#define LLCC68_STATUS_CHIPMODE_STBY_RC   0x02
#define LLCC68_STATUS_CHIPMODE_STBY_XOSC 0x03
#define LLCC68_STATUS_CHIPMODE_FS        0x04
#define LLCC68_STATUS_CHIPMODE_RX        0x05
#define LLCC68_STATUS_CHIPMODE_TX        0x06


#include <stdbool.h>
#include <stdint.h>

typedef enum {
    LLCC68_LINK_STATUS_DISCONNECTED = 0,
    LLCC68_LINK_STATUS_CONNECTED = 1,
    LLCC68_LINK_STATUS_ERROR = 2,
} LLCC68_Link_Status_t;

extern uint8_t Data[];
extern LLCC68_Link_Status_t Link_status;

bool LLCC68_init(void);

void llcc68_listen();