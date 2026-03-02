#include "driver/spi_master.h"
#include "freertos/task.h"

spi_device_handle_t display_spi_handle;

void st7920_send(uint8_t is_data, uint8_t value) {
    uint8_t packet[3];
    // Start Byte: 111110 (Sync) + RS (1 for data, 0 for cmd) + RW (0 for write)
    packet[0] = 0xF8 | (is_data << 1); 
    // High Nibble
    packet[1] = value & 0xF0;
    // Low Nibble
    packet[2] = (value << 4) & 0xF0;
    
    spi_transaction_t t = {
        .length = 24, // 3 bytes * 8 bits
        .tx_buffer = packet,
    };
    spi_device_polling_transmit(display_spi_handle, &t);
}

void st7920_clear_GDRAM(){
    spi_device_acquire_bus(display_spi_handle, portMAX_DELAY);
    for (uint8_t y = 0; y < 32; y++) {
        // Clear Upper Half
        st7920_send(0, 0x80 | y);
        st7920_send(0, 0x80 | 0);
        for (uint8_t x = 0; x < 16; x++) {
            st7920_send(1, 0x00);
        }

        // Clear Lower Half
        st7920_send(0, 0x80 | y);
        st7920_send(0, 0x80 | 8);
        for (uint8_t x = 0; x < 16; x++) {
            st7920_send(1, 0x00);
        }
    }
    spi_device_release_bus(display_spi_handle);
}

void st7920_init(int data_pin, int clk_pin) {
    // 1. Setup SPI Bus
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = data_pin,
        .sclk_io_num = clk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000, 
        .mode = 3,                 
        .spics_io_num = -1,        
        .queue_size = 7,
    };
    spi_bus_add_device(SPI2_HOST, &devcfg, &display_spi_handle);

    //sending commands to the screen
    vTaskDelay(pdMS_TO_TICKS(50));
    spi_device_acquire_bus(display_spi_handle, portMAX_DELAY);
    st7920_send(0, 0x30); // Basic instruction set
    st7920_send(0, 0x01); // Clear screen
    spi_device_release_bus(display_spi_handle);
    vTaskDelay(pdMS_TO_TICKS(10));
    spi_device_acquire_bus(display_spi_handle, portMAX_DELAY);
    st7920_send(0, 0x36); // Extended instruction set + Graphic ON
    spi_device_release_bus(display_spi_handle);
    st7920_clear_GDRAM();
}


void st7920_write_Buffer(uint8_t* display_screenBuffer) {
    spi_device_acquire_bus(display_spi_handle, portMAX_DELAY);

    for (uint8_t y = 0; y < 32; y++) {
        st7920_send(0, 0x80 | y);
        st7920_send(0, 0x80 | 0);
        for (uint8_t x = 0; x < 16; x++)
            st7920_send(1, display_screenBuffer[y * 16 + x]);

        st7920_send(0, 0x80 | y);
        st7920_send(0, 0x80 | 8);
        for (uint8_t x = 0; x < 16; x++)
            st7920_send(1, display_screenBuffer[512 + (y * 16 + x)]);
    }

    spi_device_release_bus(display_spi_handle);
}

