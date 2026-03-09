#include "menu.h"
#include "display.h"
#include "LLCC68.h"
#include "rotaryEnc.h"

#include "menu/SensorData.h"
#include "menu/GPSData.h"
#include "menu/Selector.h"

int global_enc_pos = 0;
int last_local_enc_pos = 0;
InputEvent event;

void render_menu() {

    bool render_selector = false;
    int menu_selected = 0;

    while(1){
        //parse input
        bool buttonPressed = false;
        while(xQueueReceive(input_evt_queue, &event, 0) == pdTRUE) {
            if(event.type == INPUT_EVENT_ROTATE) {
                int delta = event.value - last_local_enc_pos;
                //check for wrap around (assuming we cant turn farster then 50 steps between updates)
                //wrap from 99 to 0 => delta -99
                if(delta < -50){
                    delta += 100;
                }
                //wrap from -99 to 0 => delta +99 
                else if(delta > 50){
                    delta -= 100;
                }
                global_enc_pos += delta;
                last_local_enc_pos = event.value;
            } else if(event.type == INPUT_EVENT_BUTTON) {
                buttonPressed = true;
            }
        }

        //handle menu state switching
        if(render_selector && buttonPressed){
            render_selector = false;
        }else if(!render_selector && buttonPressed){
            render_selector = true;
            global_enc_pos = menu_selected; //reset to start at 0 in the list
        }

        //render window

        display_draw_clear();

        if(render_selector){
            menu_selected = render_menu_selector(global_enc_pos);
        }else{
            switch (menu_selected){
                case 0:
                    render_SensorData_menu(); break;
                case 1:
                    render_GPS_menu(); break;
                case 2:
                    //render_LoRa_menu(); break;
                case 3:
                    //render_Options_menu(); break;
                default:
                    break;
            }
        }

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


        vTaskDelay(pdMS_TO_TICKS(100));
    }
}