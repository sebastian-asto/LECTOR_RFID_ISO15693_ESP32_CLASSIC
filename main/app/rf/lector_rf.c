#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"

#include "spi_manager.h"
#include "pn5180.h"
#include "lector_rf.h"
#include "app/ble/ble_advertiser.h"
#include "app/status_led/status_led.h"
#include "max17048.h"

#define NO_TAG_COUNT_BEFORE_LPCD      2
#define LPCD_BATTERY_UPDATE_MS        30000
#define LPCD_WAKE_SETTLE_MS           20
#define LPCD_ENTRY_SETTLE_MS          80
#define TAG_PRESENT_DELAY_MS          1000
#define TAG_NOT_FOUND_DELAY_MS        300
#define PN5180_IRQ_INTR_FLAGS         0

static const char *TAG = "LECTOR_RF";
static uint8_t last_battery_percent;
static SemaphoreHandle_t pn5180_irq_sem;
static bool lpcd_irq_ready;

static void IRAM_ATTR pn5180_irq_isr_handler(void *arg)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    if (pn5180_irq_sem != NULL) {
        xSemaphoreGiveFromISR(pn5180_irq_sem, &higher_priority_task_woken);
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}

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

static esp_err_t lector_rf_init_irq_wakeup(void)
{
    pn5180_irq_sem = xSemaphoreCreateBinary();
    if (pn5180_irq_sem == NULL) {
        ESP_LOGE(TAG, "No se pudo crear semaforo IRQ PN5180");
        return ESP_ERR_NO_MEM;
    }

    gpio_config_t irq_pin = {
        .pin_bit_mask = (1ULL << PIN_IRQ),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };

    ESP_ERROR_CHECK(gpio_config(&irq_pin));

    esp_err_t err = gpio_install_isr_service(PN5180_IRQ_INTR_FLAGS);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "No se pudo instalar servicio ISR GPIO: %s",
                 esp_err_to_name(err));
        return err;
    }

    err = gpio_isr_handler_add(PIN_IRQ, pn5180_irq_isr_handler, NULL);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "No se pudo agregar ISR PN5180 IRQ: %s",
                 esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "IRQ PN5180 configurado en GPIO%d para LPCD", PIN_IRQ);
    lpcd_irq_ready = true;
    return ESP_OK;
}

static void lector_rf_drain_irq_events(void)
{
    if (pn5180_irq_sem == NULL) {
        return;
    }

    while (xSemaphoreTake(pn5180_irq_sem, 0) == pdTRUE) {
    }
}

static void lector_rf_wait_lpcd_wakeup(uint8_t battery_percent)
{
    if (!lpcd_irq_ready || pn5180_irq_sem == NULL) {
        ESP_LOGW(TAG, "LPCD no disponible. Continuando con polling normal.");
        vTaskDelay(pdMS_TO_TICKS(1000));
        return;
    }

    ble_advertiser_set_no_tag(battery_percent);
    status_led_set_tag_detected(false);
    lector_rf_drain_irq_events();

    while (1) {
        ESP_LOGI(TAG, "Entrando a LPCD. Esperando IRQ del PN5180...");

        esp_err_t err = pn5180_prepare_lpcd();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "No se pudo entrar a LPCD: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(1000));
            return;
        }

        /*
         * Al cambiar a LPCD el pin IRQ puede generar un flanco espurio.
         * Lo dejamos asentarse y drenamos ese evento antes de esperar una deteccion real.
         */
        vTaskDelay(pdMS_TO_TICKS(LPCD_ENTRY_SETTLE_MS));
        lector_rf_drain_irq_events();

        while (xSemaphoreTake(pn5180_irq_sem,
                              pdMS_TO_TICKS(LPCD_BATTERY_UPDATE_MS)) != pdTRUE) {
            battery_percent = lector_rf_get_battery_percent();
            ble_advertiser_set_no_tag(battery_percent);
            ESP_LOGI(TAG, "LPCD activo. Sin IRQ, BAT: %u%%",
                     (unsigned int)battery_percent);
        }

        vTaskDelay(pdMS_TO_TICKS(LPCD_WAKE_SETTLE_MS));

        uint32_t irq_status = 0;
        err = pn5180_read_register(PN5180_REG_IRQ_STATUS, &irq_status);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Wake LPCD, pero no se pudo leer IRQ_STATUS: %s",
                     esp_err_to_name(err));
            init_pn5180();
            return;
        }

        ESP_LOGI(TAG, "Wake LPCD por IRQ. IRQ_STATUS=0x%08lX",
                 (unsigned long)irq_status);

        if ((irq_status & PN5180_IRQ_LPCD) != 0) {
            pn5180_clear_irq(irq_status);
            return;
        }

        ESP_LOGW(TAG, "IRQ sin LPCD_IRQ. Se ignora y se reintenta LPCD.");
        if (irq_status != 0) {
            pn5180_clear_irq(irq_status);
        }
    }
}

static void lector_rf_task(void *arg)
{
    ESP_LOGI(TAG, "Inicializando lector RF para lectura ISO15693 con LPCD");

    spi_master_init();
    init_pn5180();
    esp_err_t irq_err = lector_rf_init_irq_wakeup();
    if (irq_err != ESP_OK) {
        ESP_LOGW(TAG, "LPCD queda deshabilitado por error IRQ: %s",
                 esp_err_to_name(irq_err));
    }

    vTaskDelay(pdMS_TO_TICKS(100));
    pn5180_get_firmware_version();
    pn5180_debug_lpcd_eeprom();

    ESP_LOGI(TAG, "Modo actual: LPCD PN5180 + lectura ISO15693 al despertar");
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

            vTaskDelay(pdMS_TO_TICKS(TAG_PRESENT_DELAY_MS));
        } else {
            ESP_LOGW(TAG,
                     "No se detecto tag ISO15693. ret=%s",
                     esp_err_to_name(err));

            if (++no_tag_count >= NO_TAG_COUNT_BEFORE_LPCD) {
                uint8_t battery_percent = lector_rf_get_battery_percent();

                ble_advertiser_set_no_tag(battery_percent);
                status_led_set_tag_detected(false);
                no_tag_count = 0;
                lector_rf_wait_lpcd_wakeup(battery_percent);
            }

            vTaskDelay(pdMS_TO_TICKS(TAG_NOT_FOUND_DELAY_MS));
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
