#include "display.h"




#define spacing (CHAR_HEIGHT + MARGIN_Y + 1)

//return is the index of the current selected one ?
int render_menu_selector(int pos){
    //make sure we get a propper wrap around
    pos = pos % 4;
    if (pos < 0) pos += 4;

    display_draw_rect(0, pos*(spacing), 60, (spacing)*(pos+1)-1, true, true);
    display_draw_string("Sensors",      MARGIN_X,  spacing * 0 + 1, pos != 0);
    display_draw_string("GPS",      MARGIN_X,  spacing * 1 + 1, pos != 1);
    display_draw_string("LoRa",      MARGIN_X,  spacing * 2 + 1, pos != 2);
    display_draw_string("Options",      MARGIN_X,  spacing * 3 + 1, pos != 3);

    return pos;
}