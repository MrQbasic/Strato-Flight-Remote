#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "rotaryEnc.h"
#include "display.h"
#include "menu.h"
#include "LLCC68.h"

void app_main(void) {
    rotary_encoder_init();

    display_init();

    LLCC68_init();

    xTaskCreate(llcc68_listen, "LORA_RX", 2048, NULL, 20, NULL);

    xTaskCreate(render_menu, "menu_Task", 2048, NULL, 2, NULL);

    char stats_buffer[1024];
    while(1) {
        vTaskGetRunTimeStats(stats_buffer);
        //printf("%s\n", stats_buffer);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}