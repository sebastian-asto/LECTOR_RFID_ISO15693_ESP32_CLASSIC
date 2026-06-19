#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "spi_manager.h"
#include "pn5180.h"
#include "lector_rf.h"
#include "app/ble/ble_advertiser.h"
#include "app/status_led/status_led.h"
#include "max17048.h"

static const char *TAG = "LECTOR_RF";
static uint8_t last_battery_percent;

static uint8_t lector_rf_get_battery_percent(void)
{
    uint8_t battery_percent = last_battery_percent;

    esp_err_t err = max17048_get_battery_percent(&battery_percent);
    if (err == ESP_OK) {
        last_battery_percent = battery_percent;
    } else {
        ESP_LOGW(TAG, "No se pudo leer bateria MAX17048: %s",
                 esp_err_to_name(err));
    }

    return last_battery_percent;
}

static void lector_rf_task(void *arg)
{
    ESP_LOGI(TAG, "Inicializando lector RF para lectura ISO15693 directa");

    spi_master_init();
    init_pn5180();

    vTaskDelay(pdMS_TO_TICKS(100));
    pn5180_get_firmware_version();

    ESP_LOGI(TAG, "Modo actual: lectura directa ISO15693 sin LPCD y sin sleep");
    ESP_LOGI(TAG, "Acerca un tag ISO15693 a la antena...");

    uint8_t no_tag_count = 0;

    while (1) {
        uint8_t uid[8] = {0};
        size_t uid_len = 0;

        esp_err_t err = pn5180_iso15693_inventory(uid, &uid_len);

        if (err == ESP_OK && uid_len == 8) {
            uint8_t battery_percent = lector_rf_get_battery_percent();

            ESP_LOGI(TAG,
                     "TAG ISO15693 detectado. UID: %02X %02X %02X %02X %02X %02X %02X %02X, BAT: %u%%",
                     uid[0], uid[1], uid[2], uid[3],
                     uid[4], uid[5], uid[6], uid[7],
                     (unsigned int)battery_percent);

            ble_advertiser_set_uid(uid, uid_len, battery_percent);
            status_led_set_tag_detected(true);
            no_tag_count = 0;

            vTaskDelay(pdMS_TO_TICKS(1000));
        } else {
            ESP_LOGW(TAG,
                     "No se detecto tag ISO15693. ret=%s",
                     esp_err_to_name(err));

            if (++no_tag_count >= 5) {
                uint8_t battery_percent = lector_rf_get_battery_percent();

                ble_advertiser_set_no_tag(battery_percent);
                status_led_set_tag_detected(false);
                no_tag_count = 5;
            }

            vTaskDelay(pdMS_TO_TICKS(300));
        }
    }
}

void lector_rf_start(void)
{
    xTaskCreatePinnedToCore(
        lector_rf_task,
        "lector_rf_task",
        4096,
        NULL,
        5,
        NULL,
        0
    );
}
