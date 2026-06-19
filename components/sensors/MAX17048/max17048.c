#include <math.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"

#include "i2c_manager.h"
#include "max17048.h"

#define MAX17048_I2C_FREQ_HZ          100000
#define MAX17048_I2C_TIMEOUT_MS       100
#define BATTERY_EMPTY_MV              3000
#define BATTERY_FULL_MV               4200

static const char *TAG = "MAX17048";

static i2c_master_dev_handle_t max17048_dev_handle = NULL;

static esp_err_t max17048_read_reg16(uint8_t reg, uint16_t *value)
{
    uint8_t data[2] = {0};

    if (value == NULL || max17048_dev_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = i2c_master_transmit_receive(max17048_dev_handle,
                                                &reg,
                                                1,
                                                data,
                                                sizeof(data),
                                                MAX17048_I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        return err;
    }

    *value = ((uint16_t)data[0] << 8) | data[1];
    return ESP_OK;
}

esp_err_t max17048_init(void)
{
    i2c_master_bus_handle_t bus_handle = i2c_manager_get_bus_handle();

    if (bus_handle == NULL) {
        ESP_LOGE(TAG, "Bus I2C no inicializado");
        return ESP_ERR_INVALID_STATE;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MAX17048_I2C_ADDR,
        .scl_speed_hz = MAX17048_I2C_FREQ_HZ,
    };

    esp_err_t err = i2c_master_bus_add_device(bus_handle,
                                              &dev_cfg,
                                              &max17048_dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo agregar MAX17048 al bus I2C: %s",
                 esp_err_to_name(err));
        return err;
    }

    uint16_t version = 0;
    err = max17048_read_reg16(MAX17048_REG_VERSION, &version);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "MAX17048 inicializado. VERSION=0x%04X", version);
    } else {
        ESP_LOGW(TAG, "MAX17048 agregado, pero no se pudo leer VERSION: %s",
                 esp_err_to_name(err));
    }

    return ESP_OK;
}

esp_err_t max17048_read_vcell_mv(uint16_t *voltage_mv)
{
    uint16_t raw = 0;

    if (voltage_mv == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = max17048_read_reg16(MAX17048_REG_VCELL, &raw);
    if (err != ESP_OK) {
        return err;
    }

    uint16_t vcell_12bit = raw >> 4;
    *voltage_mv = (uint16_t)(((uint32_t)vcell_12bit * 1250U) / 1000U);
    return ESP_OK;
}

esp_err_t max17048_read_soc_percent(float *soc_percent)
{
    uint16_t raw = 0;

    if (soc_percent == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = max17048_read_reg16(MAX17048_REG_SOC, &raw);
    if (err != ESP_OK) {
        return err;
    }

    *soc_percent = (float)(raw >> 8) + ((float)(raw & 0xFF) / 256.0f);
    return ESP_OK;
}

esp_err_t max17048_get_battery_percent(uint8_t *battery_percent)
{
    uint16_t voltage_mv = 0;

    if (battery_percent == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = max17048_read_vcell_mv(&voltage_mv);
    if (err != ESP_OK) {
        return err;
    }

    if (voltage_mv <= BATTERY_EMPTY_MV) {
        *battery_percent = 0;
    } else if (voltage_mv >= BATTERY_FULL_MV) {
        *battery_percent = 100;
    } else {
        uint32_t scaled = ((uint32_t)(voltage_mv - BATTERY_EMPTY_MV) * 100U);
        *battery_percent = (uint8_t)((scaled + 600U) / 1200U);
    }

    ESP_LOGI(TAG, "Bateria: %u mV -> %u%%",
             (unsigned int)voltage_mv,
             (unsigned int)*battery_percent);

    return ESP_OK;
}
