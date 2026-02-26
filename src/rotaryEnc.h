#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define ENC_A_GPIO 16
#define ENC_B_GPIO 17
#define ENC_SW_GPIO 25 

#define ENC_TASK_PRIORITY 5

extern QueueHandle_t input_evt_queue;

typedef enum {
    INPUT_EVENT_ROTATE,
    INPUT_EVENT_BUTTON
} InputEventType;

typedef struct {
    InputEventType type;
    int value; // For rotation, this is the count; ignored for button events
} InputEvent;

void rotary_encoder_init(void);

int rotary_encoder_get_position(void);