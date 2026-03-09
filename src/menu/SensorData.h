#include "display.h"
#include "LLCC68.h"

#include <stdio.h>
#include <string.h>

char buf[256];

void render_SensorData_menu(){
    display_draw_string("Sensor Data:", 0, 0, true);

    snprintf(buf, sizeof(buf), "TEMP_1: %.2f C\nTEMP_2: %.2f C\nPRESSURE: %.2f hPa\nHUMID: %.2f\nUV: %.2f", Data->temp_1, Data->temp_2, Data->pressure, Data->humidity, Data->uv);
    display_draw_string(buf, 0, 20, true);
}