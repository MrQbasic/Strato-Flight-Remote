#include "display.h"
#include "LLCC68.h"

#include <stdio.h>
#include <string.h>

char buf[256];

void render_SensorData_menu(){
    // Render link status in top right
    const char* status_String = "UNKNOWN";
    switch (Link_status){
        case LLCC68_LINK_STATUS_DISCONNECTED:
            status_String = "NO LINK"; break;
        case LLCC68_LINK_STATUS_CONNECTED:
            status_String = "OK LINK"; break;
        case LLCC68_LINK_STATUS_ERROR:
            status_String = "ERROR"; break;
        default:
            break;
    }
    int x_status = SCREEN_WIDTH - strlen(status_String) * (CHAR_WIDTH + MARGIN_X);
    display_draw_string((char*) status_String, x_status, 0, true);

    display_draw_string("Sensor Data:", 0, 0, true);

    snprintf(buf, sizeof(buf), "TEMP_1: %.2f C\nTEMP_2: %.2f C\nPRESSURE: %.2f hPa\nHUMID: %.2f\nUV: %.2f", ((float*)Data)[0], ((float*)Data)[1], ((float*)Data)[2], ((float*)Data)[3], ((float*)Data)[4]);
    display_draw_string(buf, 0, 20, true);
}