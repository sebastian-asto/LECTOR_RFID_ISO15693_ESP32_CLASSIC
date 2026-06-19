#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ble_advertiser_init(void);
void ble_advertiser_set_uid(const uint8_t *uid, size_t uid_len, uint8_t battery_percent);
void ble_advertiser_set_no_tag(uint8_t battery_percent);

#ifdef __cplusplus
}
#endif
