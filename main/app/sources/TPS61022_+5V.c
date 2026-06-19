#include <stdio.h> 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/gpio.h>
#include <esp_log.h>

#include "app/sources/TPS61022_+5V.h"

#define TPS61022_EN GPIO_NUM_12 
static const char* TAG = "SOURCE_+5V";

void init_TPS61022_5V(void){

    gpio_config_t PIN_EN ={
        .pin_bit_mask = (1ULL << TPS61022_EN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&PIN_EN);

    gpio_set_level(TPS61022_EN,0);  // Disable el TPS61022
    ESP_LOGI(TAG, "TPS61022 Inicializado");

}
void enable_TPS61022(void){
    gpio_set_level(TPS61022_EN,1);
    ESP_LOGI(TAG, "TPS61022 enabled");;
}

void disable_TPS61022(void){
    gpio_set_level(TPS61022_EN,0);
    ESP_LOGI(TAG, "TPS61022 disable");

}