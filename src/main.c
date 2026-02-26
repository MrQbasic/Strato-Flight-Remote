#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "rotaryEnc.h"
#include "display.h"

void app_main(void) {
    rotary_encoder_init();
    display_init();
    display_update_auto(false);

    int x = 0;
    while (1) {
        // A. Check Rotation (Polling the hardware counter)
        InputEvent event;
        

        while(xQueueReceive(input_evt_queue, &event, 0) == pdTRUE) {
            if(event.type == INPUT_EVENT_ROTATE) {
                ESP_LOGI("Main", "Rotated to position: %d", event.value);
                x = (event.value+100) / 2 % 100;
            } else if(event.type == INPUT_EVENT_BUTTON) {
                ESP_LOGI("Main", "Button Pressed!");
            }

            // Update the display with the new position
            display_draw_clear();
            display_draw_line(x, 10, 100, 50);
            display_update();
        }

        

        // Sleep for 20ms to prevent "spamming" the CPU
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}