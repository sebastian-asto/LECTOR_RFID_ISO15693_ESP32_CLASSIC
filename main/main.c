#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
//#include "esp_heap_caps.h"

//#include "esp_sleep.h"

#include "i2c_manager.h"
#include "app/sources/TPS61022_+5V.h"
#include "app/rf/lector_rf.h"

void init_led_state(void);
//void test_Mode_Light_sleep(uint32_t tiempo_ms);
static const char* TAG = "MAIN";

void app_main(void)
{
    init_I2C_Master();
    init_TPS61022_5V();
    enable_TPS61022(); //Habilita alimentacion del Pin TVSS-> PN5180
    
    scanI2C_Devices();
    lector_rf_start();
    init_led_state();

    while(1)
    {
        //enable_TPS61022();
        
        gpio_set_level(GPIO_NUM_14, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        /*disable_TPS61022();

        test_Mode_Light_sleep(10000); // el esp duerme por 1segundo

        enable_TPS61022();*/
        gpio_set_level(GPIO_NUM_14, 0);
        vTaskDelay(pdMS_TO_TICKS(2000));

        //disable_TPS61022();
        //vTaskDelay(pdMS_TO_TICKS(2000));
        //ESP_LOGI(TAG,"Task Core 0");
    }
}

void init_led_state(void){
    gpio_config_t PIN ={
        .pin_bit_mask = (1ULL << GPIO_NUM_14),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    gpio_config(&PIN);
}

/*void test_Mode_Light_sleep(uint32_t tiempo_ms)
{
    uint64_t tiempo_sleep_us = (uint64_t)tiempo_ms * 1000ULL;
    ESP_LOGI(TAG, "Entrando a Light-sleep por %lu ms...", tiempo_ms);
    esp_sleep_enable_timer_wakeup(tiempo_sleep_us);
    esp_light_sleep_start();
    ESP_LOGI(TAG, "Desperto el ESP");
}*/
