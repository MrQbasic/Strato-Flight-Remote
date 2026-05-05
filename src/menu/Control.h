#include "display.h"

#include "LoRa.h"

#define spacing (CHAR_HEIGHT + MARGIN_Y + 1)

//used for a simple on / off state 
void renderStatus(int x, int y, bool state){
    if(state){
        display_draw_string("ON", x, y, true);
    }else{
        display_draw_string("OFF", x, y, true);
    }
}

bool buzzerStatus = false;
bool releaseStatus = false;


//return == true --> exit the menu 
bool render_Control(int global_pos, bool buttonPressed){
    int pos = global_pos % 4;
    if(pos < 0) pos = -pos;


    display_draw_string("Control:", 0, 0, true);
    display_draw_rect(0, spacing*(pos+2), 50, spacing*(pos+3)-1, true, true);

    display_draw_string("Buzzer", 1, spacing * 2 + 1, pos != 0);
    renderStatus(65, spacing*2+1, buzzerStatus);
    if((pos == 0) && buttonPressed){
        buzzerStatus ^= 1;
        Lora_Command cmd = {.cmd=0x01, .arg=(uint8_t)(buzzerStatus)};
        xQueueSend(command_evt_queue, &cmd, 0);
    }

    display_draw_string("Release", 1, spacing * 3 + 1, pos != 1);
    renderStatus(65, spacing*3+1, releaseStatus);
    if((pos == 1) && buttonPressed){
        releaseStatus ^= 1;
        Lora_Command cmd = {.cmd=0x02, .arg=(uint8_t)(releaseStatus)};
        xQueueSend(command_evt_queue, &cmd, 0);
    }


    display_draw_string("Test-2", 1, spacing * 4 + 1, pos != 2);
    display_draw_string("Exit"  , 1, spacing * 5 + 1, pos != 3);


    return (pos == 3) && buttonPressed;
}

