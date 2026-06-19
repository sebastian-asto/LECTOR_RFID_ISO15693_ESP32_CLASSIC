#pragma once

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

/* ============================================================
 * PN5180 - Comandos directos host interface
 * ============================================================ */
#define PN5180_CMD_WRITE_REGISTER      0x00
#define PN5180_CMD_WRITE_REGISTER_OR   0x01
#define PN5180_CMD_WRITE_REGISTER_AND  0x02
#define PN5180_CMD_READ_REGISTER       0x04
#define PN5180_CMD_WRITE_EEPROM        0x06
#define PN5180_CMD_READ_EEPROM         0x07
#define PN5180_CMD_WRITE_TX_DATA       0x08
#define PN5180_CMD_SEND_DATA           0x09
#define PN5180_CMD_READ_DATA           0x0A
#define PN5180_CMD_LOAD_RF_CONFIG      0x11
#define PN5180_CMD_RF_ON               0x16
#define PN5180_CMD_RF_OFF              0x17

//#----------

#define PN5180_REG_IRQ_STATUS          0x02
#define PN5180_REG_IRQ_CLEAR           0x03
#define PN5180_REG_RX_STATUS           0x13

#define PN5180_IRQ_RX                  (1UL << 0)
#define PN5180_IRQ_TX                  (1UL << 1)
#define PN5180_IRQ_IDLE                (1UL << 2)
#define PN5180_IRQ_GENERAL_ERROR       (1UL << 17)

#define PN5180_RX_NUM_BYTES_MASK       0x000001FFUL


//#-----------

#define PN5180_CMD_SWITCH_MODE         0x0B

#define PN5180_SWITCH_MODE_STANDBY     0x00
#define PN5180_SWITCH_MODE_LPCD        0x01
#define PN5180_SWITCH_MODE_AUTOCOLL    0x02

#define PN5180_IRQ_LPCD                (1UL << 19)


//#--------
#define PN5180_REG_IRQ_ENABLE          0x01

#define PN5180_IRQ_CLEAR_ALL           0x000FFFFF

#define PN5180_IRQ_MASK_LPCD_ONLY      PN5180_IRQ_LPCD
#define PN5180_IRQ_MASK_RX_TX_IDLE     (PN5180_IRQ_RX | PN5180_IRQ_TX | PN5180_IRQ_IDLE)


/* ============================================================
 * Registros básicos
 * ============================================================ */
#define PN5180_REG_SYSTEM_CONFIG       0x00

/* SYSTEM_CONFIG.COMMAND */
#define PN5180_SYSTEM_CMD_IDLE         0x00
#define PN5180_SYSTEM_CMD_TRANSCEIVE   0x03

/* ============================================================
 * RF Config
 * ============================================================ */
#define PN5180_RF_TX_ISO15693_ASK100   0x0D
#define PN5180_RF_RX_ISO15693_26KBPS   0x8D

#define PN5180_RF_TX_ISO15693_ASK10    0x0E
#define PN5180_RF_RX_ISO15693_53KBPS   0x8E

/* ============================================================
 * ISO15693
 * ============================================================ */
#define ISO15693_CMD_INVENTORY         0x01

/*
 * Flags ISO15693:
 * 0x26 = 0010 0110
 * bit1 = high data rate
 * bit2 = inventory flag
 * bit5 = 1 slot
 */
#define ISO15693_FLAG_INVENTORY_1SLOT_HIGH_RATE  0x26

/* Respuesta inventario ISO15693:
 * byte 0 = flags respuesta
 * byte 1 = DSFID
 * byte 2..9 = UID
 */
#define ISO15693_INVENTORY_RESPONSE_LEN 10
#define ISO15693_UID_LEN                8

#ifdef __cplusplus
extern "C" {
#endif

void init_pn5180(void);

esp_err_t pn5180_wait_busy_low(uint32_t timeout_ms);

esp_err_t pn5180_read_register(uint8_t reg, uint32_t *value);
esp_err_t pn5180_write_register(uint8_t reg, uint32_t value);

esp_err_t pn5180_read_eeprom(uint8_t addr, uint8_t len, uint8_t *data);
void pn5180_get_firmware_version(void);

esp_err_t pn5180_load_rf_config(uint8_t tx_conf, uint8_t rx_conf);
esp_err_t pn5180_rf_on(void);
esp_err_t pn5180_rf_off(void);

esp_err_t pn5180_set_command(uint8_t command);

esp_err_t pn5180_send_data(const uint8_t *tx_data, size_t tx_len, uint8_t valid_bits_last_byte);
esp_err_t pn5180_read_data(uint8_t *rx_data, size_t rx_len);

esp_err_t pn5180_transceive_fixed(const uint8_t *tx_data,
                                  size_t tx_len,
                                  uint8_t *rx_data,
                                  size_t *rx_len,
                                  uint32_t wait_response_ms);

esp_err_t pn5180_iso15693_inventory(uint8_t *uid_out, size_t *uid_len);


esp_err_t pn5180_clear_irq(uint32_t mask);
esp_err_t pn5180_get_rx_length(size_t *rx_len);

esp_err_t pn5180_switch_mode_lpcd(uint16_t wakeup_ms);
esp_err_t pn5180_prepare_lpcd(void);
esp_err_t pn5180_enable_irq_mask(uint32_t mask);
void pn5180_debug_lpcd_eeprom(void);
#ifdef __cplusplus
}
#endif
