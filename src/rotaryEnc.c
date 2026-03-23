#include "rotaryEnc.h"

#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "driver/gpio_filter.h"
#include "esp_log.h"


pcnt_unit_handle_t pcnt_unit = NULL;
QueueHandle_t input_evt_queue = NULL;

// 1. THE INTERRUPT HANDLER (For the Button)
// This runs in IRAM (Internal RAM) for speed and safety

static void IRAM_ATTR gpio_isr_button(void* arg) {
    static uint32_t last_tick = 0;
    uint32_t now = xTaskGetTickCountFromISR();

    // Only send the event if at least 500ms (50 ticks at 100Hz) has passed
    if ((now - last_tick) > pdMS_TO_TICKS(500)) {
        InputEvent ev = { .type = INPUT_EVENT_BUTTON, .value = 1 };
        xQueueSendFromISR(input_evt_queue, &ev, NULL);
        last_tick = now;
    }
}

int last_position = 0;
void check_position_and_update(){
    while(1){
        int current_position = rotary_encoder_get_position();
        if(current_position != last_position){
            InputEvent evt = {
                .type = INPUT_EVENT_ROTATE,
                .value = current_position
            };
            xQueueSend(input_evt_queue, &evt, portMAX_DELAY);
            last_position = current_position;
        }
        vTaskDelay(pdMS_TO_TICKS(50)); // Check every 100ms
    }
}

void rotary_encoder_init(void){
    gpio_set_pull_mode(ENC_A_GPIO, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(ENC_B_GPIO, GPIO_PULLUP_ONLY);

    // --- PCNT (Rotation) Setup ---
    pcnt_unit_config_t unit_config = {
        .high_limit = 100,
        .low_limit = -100,
    };

    pcnt_new_unit(&unit_config, &pcnt_unit);

    pcnt_glitch_filter_config_t filter_config = { .max_glitch_ns = 5000 };
    pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config);

    pcnt_chan_config_t chan_config = {
        .edge_gpio_num = ENC_A_GPIO,
        .level_gpio_num = ENC_B_GPIO,
    };
    pcnt_channel_handle_t pcnt_chan = NULL;
    pcnt_new_channel(pcnt_unit, &chan_config, &pcnt_chan);

    pcnt_channel_set_edge_action(pcnt_chan, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD);
    pcnt_channel_set_level_action(pcnt_chan, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

    pcnt_unit_enable(pcnt_unit);
    pcnt_unit_start(pcnt_unit);

    //setup event queue
    input_evt_queue = xQueueCreate(10, sizeof(InputEvent));

    // --- GPIO (Button) Setup ---
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .pin_bit_mask = (1ULL << ENC_SW_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0
    };
    gpio_config(&io_conf);


    // This handles the "implicit declaration" error
    gpio_install_isr_service(0);
    gpio_isr_handler_add(ENC_SW_GPIO, gpio_isr_button, (void*) ENC_SW_GPIO);

    xTaskCreate(check_position_and_update, "check_position_task", 2048, NULL, ENC_TASK_PRIORITY, NULL);
}

int rotary_encoder_get_position(void) {
    if(pcnt_unit == NULL) return 0; // Safety check
    int count = 0;
    pcnt_unit_get_count(pcnt_unit, &count);
    return count;
}



