#include "LoRa.h"
#include "LLCC68.h"

#include <string.h>
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static SemaphoreHandle_t lora_irq_sem = NULL;

LoraStatus Lora_Status;
LoraData* Lora_Data = NULL;

QueueHandle_t command_evt_queue;

//Interupt function for the int pin
static void IRAM_ATTR lora_dio1_isr(void *arg) {
    BaseType_t higher_priority_woken = pdFALSE;
    xSemaphoreGiveFromISR(lora_irq_sem, &higher_priority_woken);
    portYIELD_FROM_ISR(higher_priority_woken);
}

void lora_task(){
    lora_irq_sem = xSemaphoreCreateBinary();
    
    //setup interrupt for reciving data
    gpio_config_t dio1_conf = {
        .pin_bit_mask = (1ULL << PIN_LORA_DIO1),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type    = GPIO_INTR_POSEDGE,  // DIO1 goes high when IRQ fires
    };
    gpio_config(&dio1_conf);
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_isr_handler_add(PIN_LORA_DIO1, lora_dio1_isr, NULL);


    // 7. SetDioIrqParams - RxDone + Timeout + CRC error on DIO1
    uint8_t irq[] = {0x08,
        0x02, 0x63,  // IrqMask: RxDone(bit1) | Timeout(bit8) | CrcErr(bit6)
        0x03, 0xFF,  // DIO1 mask
        0x00, 0x00,  // DIO2 mask
        0x00, 0x00   // DIO3 mask
    };
    llcc68_cmd(irq, sizeof(irq));

    // 8. Set LoRa sync word - must match TX
    uint8_t sync[] = {0x0D, 0x07, 0x40, 0x14, 0x24};
    llcc68_cmd(sync, sizeof(sync));
    
    llcc68_setPacketParams_Lora(8, false, 255, true, false);  //255 is the max payload length as its only reciving here

    bool pushedCommand = false;

    //check if threr was a packet recived
    while(1){
        int startTime = xTaskGetTickCount();

        //wait 4000ms for a packet
        llcc68_rx(4000);

        if (xSemaphoreTake(lora_irq_sem, portMAX_DELAY) == pdPASS) {
            int endTime = xTaskGetTickCount();
            
            printf("Packet received in %lu ms\n",(int) (endTime - startTime) * portTICK_PERIOD_MS);

            //read IRQ status 
            uint8_t irq_status[] = {0x12, 0x00, 0x00, 0x00};
            llcc68_cmd(irq_status, sizeof(irq_status));
            uint16_t irq_flags = (irq_status[2] << 8) | irq_status[3];

            // clear IRQ
            uint8_t clear_irq[] = {0x02, 0xFF, 0xFF};
            llcc68_cmd(clear_irq, sizeof(clear_irq));

            if (irq_flags & 0x02) {
                //get packet status
                uint8_t packet_status[] = {0x14, 0x00, 0x00, 0x00, 0x00};
                llcc68_cmd(packet_status, sizeof(packet_status));

                Lora_Status.SNR = packet_status[3]/4.0f;

                // Response format: [Status, PayloadLength, RxStartBufferPointer]
                uint8_t get_buf_status[] = {0x13, 0x00, 0x00, 0x00};
                llcc68_cmd(get_buf_status, sizeof(get_buf_status));
                uint8_t len = get_buf_status[2]; // Length of received packet
                uint8_t ptr = get_buf_status[3]; // Start address in chip RAM

                llcc68_buffer_read(ptr, (uint8_t*) Lora_Data, len);

                int currentCommand = 0x00;
                int currentCommandArg = 0x00;

                //command confirmed (also gets send for the 0x00 NOP command)
                if(Lora_Data->lastPacketStatus == 0x00){    
                    //check if we ran a real command last time kick it from the list
                    if(pushedCommand){
                        Lora_Command dummy;
                        xQueueReceive(command_evt_queue, &dummy, 0);
                    }
                    Lora_Command cmd;
                    if(xQueuePeek(command_evt_queue, &cmd, 0) == pdTRUE) {
                        pushedCommand = true;
                        currentCommand = cmd.cmd;
                        currentCommandArg = cmd.arg;
                        printf("Sending command: %8x : %8x\n" ,currentCommand, currentCommandArg);
                    }else{
                        pushedCommand = false;
                    }
                    Lora_Status.linkStatus = LoRa_LINK_STATUS_CONNECTED;
                }else{
                    //if the TX didn't have any success then resend the command
                    Lora_Command cmd;
                    if(xQueuePeek(command_evt_queue, &cmd, 0) == pdTRUE) {
                        pushedCommand = true;
                        currentCommand = cmd.cmd;
                        currentCommandArg = cmd.arg;
                    }else{
                        pushedCommand = false;
                        //TODO add error message
                        //This is unexpected as we can't resend the command that just failed ?
                    }
                    Lora_Status.linkStatus = LoRa_LINK_STATUS_RX_ONLY;
                }

                Lora_Status.packetCount++;
                
                vTaskDelay(pdMS_TO_TICKS(2000)); //delay to ensure the response is sent after the packet is fully processed, may not be necessary

                //send response
                LoraResponse response = {
                    .packet_id = Lora_Data->packet_id,
                    .command = currentCommand, // ACK command
                    .arg = currentCommandArg, // No error
                    .command_repeat = currentCommand,
                    .arg_repeat = currentCommandArg,
                };
                llcc68_tx(&response, 8);
                vTaskDelay(pdMS_TO_TICKS(500)); //Let the tx tx  TODO Poll the status to make shure its ok to rx again

            }else if(irq_flags & 0x20){
                printf("LoRa Header Error!\n");
                Lora_Status.linkStatus = LoRa_LINK_STATUS_ERROR;
                Lora_Status.errorCount++;
            }else if (irq_flags & 0x40) {
                printf("LoRa CRC Error!\n");
                Lora_Status.linkStatus = LoRa_LINK_STATUS_ERROR;
                Lora_Status.errorCount++;
            } else if (irq_flags & 0x200) {
                printf("LoRa Timeout!\n");
                Lora_Status.linkStatus = LoRa_LINK_STATUS_DISCONNECTED;
                Lora_Status.timeoutCount++;
            }else if (irq_flags & 0x01){
                //TX done interrupt
                //response packet is send DONE ---> TODO add this to the TODO above and remove delay for performance and reliability!
            } else {
                printf("unexprected interrupt! 0x%02x\n", irq_flags);
                Lora_Status.linkStatus = LoRa_LINK_STATUS_ERROR;
                Lora_Status.errorCount++;
            }
            
        }
    }
}


void LoRa_setup(){
    command_evt_queue = xQueueCreate(10, sizeof(Lora_Command));

    Lora_Data = (struct LoraData*) malloc(256 * sizeof(uint8_t)); //alloc max buffer length to avoid segfaults if struct is wrong
    memset(Lora_Data, 0, 256 * sizeof(uint8_t));

    LLCC68_init();

    xTaskCreate(lora_task, "LORA_COMM", 2048, NULL, 20, NULL);
    
}