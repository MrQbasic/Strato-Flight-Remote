#include "menu.h"
#include "display.h"
#include "rotaryEnc.h"
#include "LLCC68.h"


#include "menu/SensorData.h"
#include "menu/GPSData.h"

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

        render_GPS_menu();
        //render_SensorData_menu();

        // Render link status in top right
        const char* status_String = "UNKNOWN";
        switch (Link_status){
            case LLCC68_LINK_STATUS_DISCONNECTED:
                status_String = "NO LINK"; break;
            case LLCC68_LINK_STATUS_CONNECTED:
                status_String = "OK LINK"; break;
            case LLCC68_LINK_STATUS_ERROR:
                status_String = "ERROR"; break;
            default:
                break;
        }
        int x_status = SCREEN_WIDTH - strlen(status_String) * (CHAR_WIDTH + MARGIN_X);
        display_draw_rect(x_status - MARGIN_X, 0, SCREEN_WIDTH, CHAR_HEIGHT + MARGIN_Y, true, true);
        display_draw_string((char*) status_String, x_status, 1, false);

        display_update();


        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}