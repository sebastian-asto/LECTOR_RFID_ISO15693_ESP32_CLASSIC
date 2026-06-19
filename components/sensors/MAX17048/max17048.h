#pragma once

#include <stdint.h>
#include "esp_err.h"

#define MAX17048_I2C_ADDR             0x36

#define MAX17048_REG_VCELL            0x02
#define MAX17048_REG_SOC              0x04
#define MAX17048_REG_MODE             0x06
#define MAX17048_REG_VERSION          0x08
#define MAX17048_REG_HIBRT            0x0A
#define MAX17048_REG_CONFIG           0x0C
#define MAX17048_REG_VALRT            0x14
#define MAX17048_REG_CRATE            0x16
#define MAX17048_REG_VRESET_ID        0x18
#define MAX17048_REG_STATUS           0x1A
#define MAX17048_REG_CMD              0xFE

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t max17048_init(void);
esp_err_t max17048_read_vcell_mv(uint16_t *voltage_mv);
esp_err_t max17048_read_soc_percent(float *soc_percent);
esp_err_t max17048_get_battery_percent(uint8_t *battery_percent);

#ifdef __cplusplus
}
#endif
