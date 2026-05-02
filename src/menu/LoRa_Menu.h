#include <display.h>
#include "LoRa.h"
#include <stdio.h>

void render_LoRa_menu() {
    display_draw_string("LoRa Status:", 0, 0, true);

    char buf[256];
    snprintf(buf, sizeof(buf), "Packets: %d\nErrors: %d\nTimeouts: %d\nSNR: %.1f\nPacket ID: %d", Lora_Status.packetCount, Lora_Status.errorCount, Lora_Status.timeoutCount, Lora_Status.SNR, Lora_Data->packet_id);
    display_draw_string(buf, 0, 20, true);
}
