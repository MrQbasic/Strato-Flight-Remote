#include "display.h"
#include "LoRa.h"

#include <stdio.h>
#include <string.h>

char buf[256];

void render_SensorData_menu(){
    display_draw_string("Sensor Data:", 0, 0, true);

    snprintf(buf, sizeof(buf), "TEMP_1: %.2f C\nTEMP_2: %.2f C\nPRESSURE: %.2f hPa\nHUMID: %.2f\nUV: %.2f", Lora_Data->temp_1, Lora_Data->temp_2, Lora_Data->pressure, Lora_Data->humidity, Lora_Data->uv);
    display_draw_string(buf, 0, 20, true);
}