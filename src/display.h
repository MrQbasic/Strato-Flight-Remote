#define DISPLAY_PIN_CLK 33
#define DISPLAY_PIN_DIO 32


void display_init(void);
void display_update(void);
void display_update_auto(bool state);

void display_draw_clear();
void display_draw_pixel(int x, int y);
void display_draw_line(int x0, int y0, int x1, int y1);
