#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "rotaryEnc.h"
#include "display.h"
#include "menu.h"

void app_main(void) {
    rotary_encoder_init();

    display_init();

    xTaskCreate(render_menu, "menu_Task", 2048, NULL, 2, NULL);
    
    char stats_buffer[1024];
    while(1) {
        vTaskGetRunTimeStats(stats_buffer);
        printf("%s\n", stats_buffer);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}