#include <stdbool.h>

#define DISPLAY_PIN_CLK 33
#define DISPLAY_PIN_DIO 32


void display_init(void);
void display_update(void);

void display_draw_clear();
void display_draw_pixel(int x, int y, bool color);
void display_draw_line(int x0, int y0, int x1, int y1, bool color);
void display_draw_rect(int x0, int y0, int x1, int y1, bool filled, bool color);
void display_draw_char(char c, int x, int y, bool color);
void display_draw_string(char* string, int x, int y, bool color);