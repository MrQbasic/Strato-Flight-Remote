#include <display.h>
#include "LLCC68.h"
#include <stdio.h>

void render_LoRa_menu() {
    display_draw_string("LoRa Status:", 0, 0, true);

    char buf[256];
    snprintf(buf, sizeof(buf), "Packets: %d\nErrors: %d\nTimeouts: %d\nRSSI: %d", packetCounter, errorCounter, timeoutCounter, packetRSSI);
    display_draw_string(buf, 0, 20, true);
}
