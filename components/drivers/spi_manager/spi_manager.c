#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "spi_manager.h"

static const char* TAG = "SPI_MANAGER";

static spi_device_handle_t spi_device_handle;

void spi_master_init(void)
{
    spi_bus_config_t bus_config = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    spi_device_interface_config_t device_config = {
        .clock_speed_hz = 1000000, // 1 MHz (bien para iniciar)
        .mode = 0,
        .spics_io_num = PIN_CS,
        .queue_size = 1,
    };

    esp_err_t err = spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error inicializando bus SPI");
        return;
    }

    err = spi_bus_add_device(SPI2_HOST, &device_config, &spi_device_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error agregando dispositivo SPI");
        return;
    }

    ESP_LOGI(TAG, "SPI inicializado correctamente");
}


esp_err_t spi_transfer(uint8_t *tx, uint8_t *rx, size_t len)
{
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };

    // evitar NULL en RX
    uint8_t dummy_rx[256];

    if (rx == NULL) {
        memset(dummy_rx, 0x00, len); //inicializo los byts que se sobreescribiran 
        t.rx_buffer = dummy_rx;
    }

    return spi_device_transmit(spi_device_handle, &t);
}