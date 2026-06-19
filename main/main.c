#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
//#include "esp_heap_caps.h"

//#include "esp_sleep.h"

#include "i2c_manager.h"
#include "app/sources/TPS61022_+5V.h"
#include "app/rf/lector_rf.h"
#include "app/ble/ble_advertiser.h"
#include "app/status_led/status_led.h"

//void test_Mode_Light_sleep(uint32_t tiempo_ms);

void app_main(void)
{
    init_I2C_Master();
    init_TPS61022_5V();
    enable_TPS61022(); //Habilita alimentacion del Pin TVSS-> PN5180
    ble_advertiser_init();
    
    scanI2C_Devices();
    lector_rf_start();
    status_led_init();

    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/*void test_Mode_Light_sleep(uint32_t tiempo_ms)
{
    uint64_t tiempo_sleep_us = (uint64_t)tiempo_ms * 1000ULL;
    ESP_LOGI(TAG, "Entrando a Light-sleep por %lu ms...", tiempo_ms);
    esp_sleep_enable_timer_wakeup(tiempo_sleep_us);
    esp_light_sleep_start();
    ESP_LOGI(TAG, "Desperto el ESP");
}*/
