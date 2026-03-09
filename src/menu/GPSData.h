#include "display.h"
#include "LLCC68.h"

#include <stdio.h>
#include <string.h>

char buf[256];

void render_GPS_menu(){
    display_draw_string("GPS:", 0, 0, true);

    snprintf(buf, sizeof(buf), "LAT: %.6f\nLON: %.6f\nALT: %.1f", Data->gps_lat, Data->gps_lon, Data->gps_alt);
    display_draw_string(buf, 0, 20, true);
}