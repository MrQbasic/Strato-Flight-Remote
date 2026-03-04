#include "menu.h"
#include "display.h"
#include "rotaryEnc.h"
#include "LLCC68.h"


#include "menu/SensorData.h"

int encoderPos = 0;
int buttonCnt = 0;



InputEvent event;

void render_menu() {

    while(1){

        /*bool update = false;
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
        */
        display_draw_clear();

        render_SensorData_menu();

        display_update();


        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}