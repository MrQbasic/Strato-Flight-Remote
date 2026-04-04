#pragma once

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


typedef enum {
    LoRa_LINK_STATUS_DISCONNECTED = 0,
    LoRa_LINK_STATUS_CONNECTED = 1,
    LoRa_LINK_STATUS_ERROR = 2,
} LoRa_Link_Status_t;

typedef struct LoraStatus {
    float RSSI;
    int packetCount;
    int errorCount;
    int timeoutCount;
    LoRa_Link_Status_t linkStatus;
} LoraStatus;

extern LoraStatus Lora_Status;
extern LoraData*  Lora_Data;

void LoRa_setup();