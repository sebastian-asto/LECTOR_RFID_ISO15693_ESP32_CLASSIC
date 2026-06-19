#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "driver/gpio.h"

#include "pn5180.h"
#include "spi_manager.h"

static const char *TAG = "PN5180";

/* ============================================================
 * Inicialización
 * ============================================================ */
void init_pn5180(void)

{
    gpio_set_direction(PIN_BUSY, GPIO_MODE_INPUT);
    gpio_set_direction(PIN_RST,  GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_IRQ,  GPIO_MODE_INPUT);

    gpio_set_level(PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));

    gpio_set_level(PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    ESP_LOGI(TAG, "PN5180 inicializado");
}

/* ============================================================
 * Esperar BUSY = LOW
 * ============================================================ */
esp_err_t pn5180_wait_busy_low(uint32_t timeout_ms)
{
    TickType_t start = xTaskGetTickCount();

    while (gpio_get_level(PIN_BUSY)) {
        if ((xTaskGetTickCount() - start) > pdMS_TO_TICKS(timeout_ms)) {
            ESP_LOGE(TAG, "Timeout esperando BUSY LOW (%lu ms)", (unsigned long)timeout_ms);
            return ESP_ERR_TIMEOUT;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }

    return ESP_OK;
}

/* ============================================================
 * Registros
 * ============================================================ */
esp_err_t pn5180_read_register(uint8_t reg, uint32_t *value)
{
    if (value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err;

    uint8_t tx_cmd[2] = {
        PN5180_CMD_READ_REGISTER,
        reg
    };

    uint8_t dummy[4] = {
        0xFF, 0xFF, 0xFF, 0xFF
    };

    uint8_t rx[4] = {0};

    err = pn5180_wait_busy_low(100);
    if (err != ESP_OK) return err;

    err = spi_transfer(tx_cmd, NULL, sizeof(tx_cmd));
    if (err != ESP_OK) return err;

    err = pn5180_wait_busy_low(100);
    if (err != ESP_OK) return err;

    err = spi_transfer(dummy, rx, sizeof(rx));
    if (err != ESP_OK) return err;

    err = pn5180_wait_busy_low(100);
    if (err != ESP_OK) return err;

    *value = ((uint32_t)rx[0]) |
             ((uint32_t)rx[1] << 8) |
             ((uint32_t)rx[2] << 16) |
             ((uint32_t)rx[3] << 24);

    return ESP_OK;
}

esp_err_t pn5180_write_register(uint8_t reg, uint32_t value)
{
    uint8_t tx[6];

    tx[0] = PN5180_CMD_WRITE_REGISTER;
    tx[1] = reg;
    tx[2] = (uint8_t)(value & 0xFF);
    tx[3] = (uint8_t)((value >> 8) & 0xFF);
    tx[4] = (uint8_t)((value >> 16) & 0xFF);
    tx[5] = (uint8_t)((value >> 24) & 0xFF);

    esp_err_t err = pn5180_wait_busy_low(100);
    if (err != ESP_OK) return err;

    err = spi_transfer(tx, NULL, sizeof(tx));
    if (err != ESP_OK) return err;

    err = pn5180_wait_busy_low(100);
    return err;
}

/* ============================================================
 * EEPROM
 * ============================================================ */
esp_err_t pn5180_read_eeprom(uint8_t addr, uint8_t len, uint8_t *data)
{
    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err;

    uint8_t tx_cmd[3] = {
        PN5180_CMD_READ_EEPROM,
        addr,
        len
    };

    err = pn5180_wait_busy_low(100);
    if (err != ESP_OK) return err;

    err = spi_transfer(tx_cmd, NULL, sizeof(tx_cmd));
    if (err != ESP_OK) return err;

    err = pn5180_wait_busy_low(100);
    if (err != ESP_OK) return err;

    uint8_t dummy[256];
    memset(dummy, 0xFF, len);

    err = spi_transfer(dummy, data, len);
    if (err != ESP_OK) return err;

    err = pn5180_wait_busy_low(100);
    return err;
}

void pn5180_get_firmware_version(void)
{
    uint8_t version[2] = {0xFF, 0xFF};

    esp_err_t err = pn5180_read_eeprom(0x12, 2, version);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo leer firmware version");
        return;
    }

    ESP_LOGI(TAG, "Firmware version raw: 0x%02X 0x%02X", version[0], version[1]);

    if (version[0] == 0x00 && version[1] == 0x04) {
        ESP_LOGI(TAG, "PN5180 FW 4.0");
    } else if (version[0] == 0x01 && version[1] == 0x04) {
        ESP_LOGI(TAG, "PN5180 FW 4.1");
    } else if (version[0] == 0x0A && version[1] == 0x03) {
        ESP_LOGI(TAG, "PN5180 FW 3.A");
    } else {
        ESP_LOGW(TAG, "Firmware desconocido");
    }
}

/* ============================================================
 * RF básico
 * ============================================================ */
esp_err_t pn5180_load_rf_config(uint8_t tx_conf, uint8_t rx_conf)
{
    uint8_t cmd[3] = {
        PN5180_CMD_LOAD_RF_CONFIG,
        tx_conf,
        rx_conf
    };

    esp_err_t err = pn5180_wait_busy_low(100);
    if (err != ESP_OK) return err;

    err = spi_transfer(cmd, NULL, sizeof(cmd));
    if (err != ESP_OK) return err;

    err = pn5180_wait_busy_low(100);
    return err;
}

esp_err_t pn5180_rf_on(void)
{
    uint8_t cmd[2] = {
        PN5180_CMD_RF_ON,
        0x00
    };

    esp_err_t err = pn5180_wait_busy_low(100);
    if (err != ESP_OK) return err;

    err = spi_transfer(cmd, NULL, sizeof(cmd));
    if (err != ESP_OK) return err;

    err = pn5180_wait_busy_low(100);
    return err;
}

esp_err_t pn5180_rf_off(void)
{
    uint8_t cmd[2] = {
        PN5180_CMD_RF_OFF,
        0x00
    };

    esp_err_t err = pn5180_wait_busy_low(100);
    if (err != ESP_OK) return err;

    err = spi_transfer(cmd, NULL, sizeof(cmd));
    if (err != ESP_OK) return err;

    err = pn5180_wait_busy_low(100);
    return err;
}

/* ============================================================
 * SYSTEM_CONFIG.COMMAND
 * ============================================================ */
esp_err_t pn5180_set_command(uint8_t command)
{
    uint32_t sys_cfg = 0;

    esp_err_t err = pn5180_read_register(PN5180_REG_SYSTEM_CONFIG, &sys_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo leer SYSTEM_CONFIG");
        return err;
    }

    /*
     * COMMAND está en los bits bajos.
     * Limpiamos bits 0..2 y colocamos nuevo comando.
     */
    sys_cfg &= ~0x07;
    sys_cfg |= (command & 0x07);

    err = pn5180_write_register(PN5180_REG_SYSTEM_CONFIG, sys_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo escribir SYSTEM_CONFIG");
        return err;
    }

    return ESP_OK;
}

/* ============================================================
 * SEND_DATA
 *
 * Formato correcto:
 * byte 0 = 0x09
 * byte 1 = valid bits in last byte
 * byte 2..n = datos RF
 * ============================================================ */
esp_err_t pn5180_send_data(const uint8_t *tx_data, size_t tx_len, uint8_t valid_bits_last_byte)
{
    if (tx_data == NULL && tx_len > 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (tx_len > 260) {
        return ESP_ERR_INVALID_SIZE;
    }

    size_t frame_len = 2 + tx_len;

    uint8_t *frame = malloc(frame_len);
    if (frame == NULL) {
        return ESP_ERR_NO_MEM;
    }

    frame[0] = PN5180_CMD_SEND_DATA;
    frame[1] = valid_bits_last_byte & 0x07;

    if (tx_len > 0) {
        memcpy(&frame[2], tx_data, tx_len);
    }

    esp_err_t err = pn5180_wait_busy_low(100);
    if (err == ESP_OK) {
        err = spi_transfer(frame, NULL, frame_len);
    }

    free(frame);

    if (err != ESP_OK) {
        return err;
    }

    err = pn5180_wait_busy_low(100);
    return err;
}

esp_err_t pn5180_clear_irq(uint32_t mask)
{
    return pn5180_write_register(PN5180_REG_IRQ_CLEAR, mask);
}

esp_err_t pn5180_get_rx_length(size_t *rx_len)
{
    if (rx_len == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t rx_status = 0;
    esp_err_t err = pn5180_read_register(PN5180_REG_RX_STATUS, &rx_status);
    if (err != ESP_OK) {
        return err;
    }

    *rx_len = rx_status & PN5180_RX_NUM_BYTES_MASK;

    ESP_LOGI(TAG, "RX_STATUS=0x%08lX, RX_LEN=%u",
             (unsigned long)rx_status,
             (unsigned int)*rx_len);

    return ESP_OK;
}


esp_err_t pn5180_switch_mode_lpcd(uint16_t wakeup_ms)
{
    if (wakeup_ms == 0) {
        wakeup_ms = 100;
    }

    /*
     * Según datasheet:
     * SWITCH_MODE = 0x0B
     * Mode LPCD = 0x01
     * Wake-up counter value = 2 bytes
     */
    uint8_t cmd[4] = {
        PN5180_CMD_SWITCH_MODE,
        PN5180_SWITCH_MODE_LPCD,
        (uint8_t)(wakeup_ms & 0xFF),
        (uint8_t)((wakeup_ms >> 8) & 0xFF)
    };

    esp_err_t err = pn5180_wait_busy_low(100);
    if (err != ESP_OK) return err;

    err = spi_transfer(cmd, NULL, sizeof(cmd));
    if (err != ESP_OK) return err;

    err = pn5180_wait_busy_low(100);
    return err;
}

esp_err_t pn5180_prepare_lpcd(void)
{
    esp_err_t err;

    /*
     * 1. Apagar el campo RF antes de entrar a LPCD.
     */
    err = pn5180_rf_off();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo RF_OFF antes de LPCD: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(5));

    /*
     * 2. Limpiar interrupciones pendientes.
     */
    err = pn5180_clear_irq(PN5180_IRQ_CLEAR_ALL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo limpiando IRQ antes de LPCD: %s", esp_err_to_name(err));
        return err;
    }

    /*
     * 3. Deshabilitar IRQ_ENABLE.
     *
     * Esto evita que IDLE_IRQ salga por el pin IRQ y despierte
     * inmediatamente al ESP32-S3.
     *
     * LPCD_IRQ es no enmascarable, así que cuando haya detección LPCD
     * debería activar el IRQ igualmente.
     */
    err = pn5180_enable_irq_mask(0x00000000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo deshabilitando IRQ_ENABLE antes de LPCD: %s", esp_err_to_name(err));
        return err;
    }

    /*
     * 4. Limpiar otra vez por seguridad después de tocar IRQ_ENABLE.
     */
    err = pn5180_clear_irq(PN5180_IRQ_CLEAR_ALL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo limpiando IRQ final antes de LPCD: %s", esp_err_to_name(err));
        return err;
    }

    /*
     * 5. Entrar a LPCD.
     *
     * Después de este comando no se debe leer/escribir al PN5180 por SPI
     * hasta que el pin IRQ despierte al ESP32-S3.
     */
    err = pn5180_switch_mode_lpcd(3000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo entrando a LPCD: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t pn5180_enable_irq_mask(uint32_t mask)
{
    /*
     * IRQ_ENABLE controla qué interrupciones salen por el pin físico IRQ.
     * IRQ_STATUS seguirá registrando eventos aunque no estén habilitados al pin.
     */
    return pn5180_write_register(PN5180_REG_IRQ_ENABLE, mask);
}


void pn5180_debug_lpcd_eeprom(void)
{
    uint8_t data[8] = {0};

    esp_err_t err = pn5180_read_eeprom(0x34, 8, data);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo leer EEPROM LPCD: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "EEPROM LPCD 0x34..0x3B:");
    ESP_LOGI(TAG, "0x34 LPCD_REFERENCE_VALUE      = 0x%02X", data[0]);
    ESP_LOGI(TAG, "0x35 RFU                       = 0x%02X", data[1]);
    ESP_LOGI(TAG, "0x36 LPCD_FIELD_ON_TIME        = 0x%02X", data[2]);
    ESP_LOGI(TAG, "0x37 LPCD_THRESHOLD            = 0x%02X", data[3]);
    ESP_LOGI(TAG, "0x38 LPCD_REFVAL_GPO_CONTROL   = 0x%02X", data[4]);
    ESP_LOGI(TAG, "0x39 GPO_BEFORE_FIELD_ON       = 0x%02X", data[5]);
    ESP_LOGI(TAG, "0x3A GPO_AFTER_FIELD_OFF       = 0x%02X", data[6]);
    ESP_LOGI(TAG, "0x3B NFCLD_SENSITIVITY_VAL     = 0x%02X", data[7]);
}


/* ============================================================
 * READ_DATA
 *
 * En este punto de partida se lee una cantidad fija.
 * Para inventario ISO15693 single-slot normalmente son 10 bytes.
 * ============================================================ */
esp_err_t pn5180_read_data(uint8_t *rx_data, size_t rx_len)
{
    if (rx_data == NULL || rx_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (rx_len > 508) {
        return ESP_ERR_INVALID_SIZE;
    }

    esp_err_t err;

    /*
     * READ_DATA según PN5180:
     * byte 0 = 0x0A
     * byte 1 = 0x00 dummy parameter
     */
    uint8_t read_cmd[2] = {
        PN5180_CMD_READ_DATA,
        0x00
    };

    err = pn5180_wait_busy_low(100);
    if (err != ESP_OK) return err;

    err = spi_transfer(read_cmd, NULL, sizeof(read_cmd));
    if (err != ESP_OK) return err;

    err = pn5180_wait_busy_low(100);
    if (err != ESP_OK) return err;

    uint8_t *dummy = malloc(rx_len);
    if (dummy == NULL) {
        return ESP_ERR_NO_MEM;
    }

    /*
     * Para leer respuesta desde PN5180 mandamos clocks dummy.
     */
    memset(dummy, 0xFF, rx_len);
    memset(rx_data, 0x00, rx_len);

    err = spi_transfer(dummy, rx_data, rx_len);

    free(dummy);

    if (err != ESP_OK) return err;

    err = pn5180_wait_busy_low(100);
    return err;
}

/* ============================================================
 * Transceive simple con longitud fija de respuesta
 * ============================================================ */
esp_err_t pn5180_transceive_fixed(const uint8_t *tx_data,size_t tx_len,uint8_t *rx_data,size_t *rx_len,uint32_t wait_response_ms)
{
    if (tx_data == NULL || tx_len == 0 || rx_data == NULL || rx_len == NULL || *rx_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err;

    /* Limpiar IRQ anteriores */
    pn5180_clear_irq(0x000FFFFF);

    err = pn5180_set_command(PN5180_SYSTEM_CMD_TRANSCEIVE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo activar modo TRANSCEIVE");
        return err;
    }

    err = pn5180_send_data(tx_data, tx_len, 0x00);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo SEND_DATA");
        pn5180_set_command(PN5180_SYSTEM_CMD_IDLE);
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(wait_response_ms));

    uint32_t irq_status = 0;
    err = pn5180_read_register(PN5180_REG_IRQ_STATUS, &irq_status);
    if (err != ESP_OK) {
        pn5180_set_command(PN5180_SYSTEM_CMD_IDLE);
        return err;
    }

    ESP_LOGI(TAG, "IRQ_STATUS=0x%08lX", (unsigned long)irq_status);

    if (irq_status & PN5180_IRQ_GENERAL_ERROR) {
        ESP_LOGE(TAG, "GENERAL_ERROR_IRQ detectado");
        pn5180_clear_irq(irq_status);
        pn5180_set_command(PN5180_SYSTEM_CMD_IDLE);
        return ESP_FAIL;
    }

    if ((irq_status & PN5180_IRQ_RX) == 0) {
        //ESP_LOGW(TAG, "No hubo RX_IRQ. No hay respuesta RF real.");
        pn5180_clear_irq(irq_status);
        pn5180_set_command(PN5180_SYSTEM_CMD_IDLE);
        *rx_len = 0;
        return ESP_ERR_NOT_FOUND;
    }

    size_t real_rx_len = 0;
    err = pn5180_get_rx_length(&real_rx_len);
    if (err != ESP_OK) {
        pn5180_set_command(PN5180_SYSTEM_CMD_IDLE);
        return err;
    }

    if (real_rx_len == 0) {
        ESP_LOGW(TAG, "RX_IRQ activo, pero RX_LEN=0");
        pn5180_clear_irq(irq_status);
        pn5180_set_command(PN5180_SYSTEM_CMD_IDLE);
        *rx_len = 0;
        return ESP_ERR_NOT_FOUND;
    }

    if (real_rx_len > *rx_len) {
        ESP_LOGE(TAG, "Buffer insuficiente: real=%u, max=%u",
                 (unsigned int)real_rx_len,
                 (unsigned int)*rx_len);
        pn5180_clear_irq(irq_status);
        pn5180_set_command(PN5180_SYSTEM_CMD_IDLE);
        *rx_len = 0;
        return ESP_ERR_INVALID_SIZE;
    }

    err = pn5180_read_data(rx_data, real_rx_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo READ_DATA");
        pn5180_clear_irq(irq_status);
        pn5180_set_command(PN5180_SYSTEM_CMD_IDLE);
        return err;
    }

    *rx_len = real_rx_len;

    pn5180_clear_irq(irq_status);
    pn5180_set_command(PN5180_SYSTEM_CMD_IDLE);

    return ESP_OK;
}

/* ============================================================
 * Inventario ISO15693
 * ============================================================ */
esp_err_t pn5180_iso15693_inventory(uint8_t *uid_out, size_t *uid_len)
{
    if (uid_out == NULL || uid_len == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err;

    err = pn5180_load_rf_config(PN5180_RF_TX_ISO15693_ASK100,
                                PN5180_RF_RX_ISO15693_26KBPS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo LOAD_RF_CONFIG ISO15693");
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(5));

    err = pn5180_rf_on();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo RF_ON");
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t inventory_cmd[3] = {
        ISO15693_FLAG_INVENTORY_1SLOT_HIGH_RATE,
        ISO15693_CMD_INVENTORY,
        0x00
    };

    uint8_t rx[32] = {0};
    size_t rx_len = sizeof(rx);

    err = pn5180_transceive_fixed(inventory_cmd,
                                  sizeof(inventory_cmd),
                                  rx,
                                  &rx_len,
                                  50);

    pn5180_rf_off();

    if (err != ESP_OK) {
        //ESP_LOGW(TAG, "No se obtuvo respuesta ISO15693 real");
        return err;
    }

    ESP_LOGI(TAG, "Respuesta ISO15693 inventory, len=%u:", (unsigned int)rx_len);
    ESP_LOG_BUFFER_HEX(TAG, rx, rx_len);

    if (rx_len < ISO15693_INVENTORY_RESPONSE_LEN) {
        ESP_LOGW(TAG, "Respuesta muy corta para inventory ISO15693: %u", (unsigned int)rx_len);
        return ESP_ERR_INVALID_SIZE;
    }

    /*
     * Si todos son FF, no aceptar como UID válido.
     */
    bool all_ff = true;
    for (size_t i = 0; i < rx_len; i++) {
        if (rx[i] != 0xFF) {
            all_ff = false;
            break;
        }
    }

    if (all_ff) {
        ESP_LOGW(TAG, "Respuesta todo FF. Se descarta como lectura dummy.");
        return ESP_ERR_NOT_FOUND;
    }

    memcpy(uid_out, &rx[2], ISO15693_UID_LEN);
    *uid_len = ISO15693_UID_LEN;

    return ESP_OK;
}
