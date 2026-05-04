#include "menu.h"
#include "display.h"
#include "LoRa.h"
#include "rotaryEnc.h"

#include "menu/SensorData.h"
#include "menu/GPSData.h"
#include "menu/Selector.h"
#include "menu/LoRa_Menu.h"
#include "menu/Control.h"

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

        display_draw_clear();
        //render Window
        if(render_selector){
            if(buttonPressed) render_selector = false;

            menu_selected = render_menu_selector(global_enc_pos);
        }else{
            bool switchOK = true;
            switch (menu_selected){
                case 0:
                    render_SensorData_menu(); break;
                case 1:
                    render_GPS_menu(); break;
                case 2:
                    render_LoRa_menu(); break;
                case 3:
                    switchOK = render_Control(global_enc_pos, buttonPressed); break;
                default:
                    break;
            }
            
            //switch back to selector if needed
            if(buttonPressed && switchOK){
                render_selector = true;
                global_enc_pos = menu_selected;
            }

        }

        // Render link status in top right
        const char* status_String = "UNKNOWN";
        switch (Lora_Status.linkStatus){
            case LoRa_LINK_STATUS_DISCONNECTED:
                status_String = "NO LINK"; break;
            case LoRa_LINK_STATUS_CONNECTED:
                status_String = "OK LINK"; break;
            case LoRa_LINK_STATUS_ERROR:
                status_String = "ERROR"; break;
            case LoRa_LINK_STATUS_RX_ONLY:
                status_String = "RX ONLY"; break;
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