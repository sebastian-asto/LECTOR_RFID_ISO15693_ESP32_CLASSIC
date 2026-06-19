#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "spi_manager.h"
#include "pn5180.h"
#include "lector_rf.h"

static const char *TAG = "LECTOR_RF";

static void lector_rf_task(void *arg)
{
    ESP_LOGI(TAG, "Inicializando lector RF para lectura ISO15693 directa");

    /*
     * 1. Inicializar bus SPI y PN5180.
     */
    spi_master_init();
    init_pn5180();

    vTaskDelay(pdMS_TO_TICKS(100));

    /*
     * 2. Lecturas de diagnóstico.
     */
    pn5180_get_firmware_version();

    ESP_LOGI(TAG, "Modo actual: lectura directa ISO15693 sin LPCD y sin sleep");
    ESP_LOGI(TAG, "Acerca un tag ISO15693 a la antena...");

    while (1) {
        uint8_t uid[8] = {0};
        size_t uid_len = 0;

        esp_err_t err = pn5180_iso15693_inventory(uid, &uid_len);

        if (err == ESP_OK && uid_len == 8) {
            ESP_LOGI(TAG,
                     "TAG ISO15693 detectado. UID: %02X %02X %02X %02X %02X %02X %02X %02X",
                     uid[0], uid[1], uid[2], uid[3],
                     uid[4], uid[5], uid[6], uid[7]);

            /*
             * Pausa más larga cuando se detecta tag para no imprimirlo demasiado rápido.
             */
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else {
            /*
             * No se considera error crítico.
             * Puede significar simplemente que no hay tag presente.
             */
            ESP_LOGW(TAG,
                     "No se detecto tag ISO15693. ret=%s",
                     esp_err_to_name(err));

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