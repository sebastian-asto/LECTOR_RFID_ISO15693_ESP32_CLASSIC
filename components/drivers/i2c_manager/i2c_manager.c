#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#include "i2c_manager.h"

static const char* TAG = "I2C_MANAGER";

static i2c_master_bus_handle_t i2c_bus_handle = NULL;

esp_err_t init_I2C_Master(void){

    i2c_master_bus_config_t config_i2c = {
        .i2c_port = I2C_MASTER_NUM,        // I2C_NUM_0 o I2C_NUM_1
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT, // fuente de reloj
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t init_bus_i2c = i2c_new_master_bus(&config_i2c,&i2c_bus_handle); 

    if (init_bus_i2c == ESP_OK) ESP_LOGI(TAG, "I2C inicializado correctamente");
    else ESP_LOGE(TAG, "Error al inicializar I2C: %s", esp_err_to_name(init_bus_i2c));

    return init_bus_i2c;
}

void scanI2C_Devices(void)
{
    ESP_LOGI(TAG, "Escaneando dispositivos I2C...");
    uint8_t device_count = 0;

    for (int address = 1; address < 127; address++) {
        esp_err_t ret = i2c_master_probe(i2c_bus_handle, address, 1000); // timeout = 1s
        if (ret == ESP_OK) {
            device_count++;
            ESP_LOGI(TAG, "%d - Dispositivo I2C encontrado en la dirección: 0x%02X", device_count, address);
        }
    }

    ESP_LOGI(TAG, "Escaneo I2C finalizado. Se encontraron %d dispositivos.", device_count);
}
