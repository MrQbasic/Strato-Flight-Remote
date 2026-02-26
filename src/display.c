#include "st7920.h"
#include "display.h"
#include <string.h>

#define SCREEN_WIDTH 128
#define SCREEN_HIGHT  64

#define CHAR_WIDTH 5
#define CHAR_HIGHT 6

#define MARGIN_X 1
#define MARGIN_Y 2

spi_device_handle_t spi;

uint8_t display_buffer[1024]; // 128x64 pixels / 8 bits per byte = 1024 bytes

// These are created by the linker. We use 'extern' to find them.
extern const uint8_t font[] asm("_binary_font_bin_start");

void display_init(void){
    st7920_init(DISPLAY_PIN_DIO, DISPLAY_PIN_CLK);
}

void display_draw_clear() {
    memset(display_buffer, 0, sizeof(display_buffer));
}


void display_draw_pixel(int x, int y, bool color) {
    if (x >= 0 && x < 128 && y >= 0 && y < 64) {
        if(color){
            display_buffer[x/8 + (y*16)] |= (0x80 >> (x%8));
        }else{
            display_buffer[x/8 + (y*16)] &= ~(0x80 >> (x%8));
        }
    }
}

void display_draw_line(int x0, int y0, int x1, int y1, bool color) {
    // Bresenham's line algorithm
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1; 
    int err = dx + dy, e2; /* error value e_xy */
    
    while (true) {
        display_draw_pixel(x0, y0, color);
        
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
        if (e2 <= dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
    }
}

void display_draw_rect(int x0, int y0, int x1, int y1, bool filled, bool color) {
    for(int y = y0; y <= y1; y++) {
        for(int x = x0; x <= x1; x++) {
            if(filled || y == y0 || y == y1 || x == x0 || x == x1) {
                display_draw_pixel(x, y, color);
            }
        }
    }
}

void display_draw_char(char c, int x, int y, bool color){
    if(c < 33 || c > 122) return;
    // Calculate start of the 7-byte block for this character
    const uint8_t *glyph = &font[(c - 33) * 7];
    for (int row = 0; row < 7; row++) {
        uint8_t data = glyph[row];
        for (int col = 0; col < 5; col++) {
            if (data & (0x80 >> col)) {
                display_draw_pixel(x + col, y + row, color);
            }
        }
    }
}

void display_draw_string(char* string, int x, int y, bool color){
    int index = -1;
    while(1){
        //get the next char and return if end
        index++;
        char c = string[index];
        if(c == '\0') break;
        //print the char
        display_draw_char(c, x, y, color);
        x += CHAR_WIDTH + MARGIN_X;
        if(x > SCREEN_WIDTH-CHAR_WIDTH){
            x = 0;
            y += CHAR_HIGHT + MARGIN_Y;
            if(y > SCREEN_HIGHT-CHAR_HIGHT){
                y=0;
            }
        }
    }
}

void display_update(void) {
    st7920_write_Buffer(display_buffer);
}