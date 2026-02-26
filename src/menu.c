#include "menu.h"
#include "display.h"
#include "rotaryEnc.h"

#include <stdio.h>

char buffer[32];

int encoderPos = 0;
int buttonCnt = 0;

void update_menu(){
    display_draw_clear();

    display_draw_rect(0,0, 128, 9, true, true);
    display_draw_string("Hello World!",0,1, false);
    
    sniprintf(buffer, sizeof(buffer), "CNT: %d   POS %d", buttonCnt, encoderPos);
    display_draw_string(buffer, 0, 40, true); 
    
    display_update();
}


InputEvent event;

void render_menu() {
    update_menu();

    while(1){
        
        bool update = false;
        while(xQueueReceive(input_evt_queue, &event, 0) == pdTRUE) {
            if(event.type == INPUT_EVENT_ROTATE) {
                encoderPos = event.value;
                
            } else if(event.type == INPUT_EVENT_BUTTON) {
                buttonCnt++;
            }
            update=true;
        }
        if(update){
            update_menu();
        }

        vTaskDelay(pdMS_TO_TICKS(30));
    }
}