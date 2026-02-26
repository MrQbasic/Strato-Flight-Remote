#include "st7920.h"
#include "display.h"
#include <string.h>


spi_device_handle_t spi;

uint8_t display_buffer[1024]; // 128x64 pixels / 8 bits per byte = 1024 bytes

void display_init(void){
    st7920_init(DISPLAY_PIN_DIO, DISPLAY_PIN_CLK);
}

void display_draw_clear() {
    memset(display_buffer, 0, sizeof(display_buffer));
}


void display_draw_pixel(int x, int y) {
    if (x >= 0 && x < 128 && y >= 0 && y < 64) {
        display_buffer[x/8 + (y*16)] |= (0x80 >> (x%8));
    }
}

void display_draw_line(int x0, int y0, int x1, int y1) {
    // Bresenham's line algorithm
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1; 
    int err = dx + dy, e2; /* error value e_xy */
    
    while (true) {
        display_draw_pixel(x0, y0);
        
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
        if (e2 <= dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
    }
}

bool auto_update_enabled = false;

void display_update_Task(void *pvParameters) {
    while(auto_update_enabled) {
        st7920_write_Buffer(display_buffer);
        vTaskDelay(pdMS_TO_TICKS(100)); // Update every 100ms
    }
}


void display_update_auto(bool state ){
    if(auto_update_enabled == state) return;

    if(state){
        auto_update_enabled = true;
        xTaskCreate(display_update_Task, "Display Update Task", 2048, NULL, 2, NULL);
    }else{
        auto_update_enabled = false;
    }
}

void display_update(void) {
    if(auto_update_enabled) return;
    st7920_write_Buffer(display_buffer);
}