#pragma once

#define PIN_MOSI  GPIO_NUM_23
#define PIN_MISO  GPIO_NUM_19
#define PIN_SCLK  GPIO_NUM_18
#define PIN_CS    GPIO_NUM_5

#define PIN_BUSY  GPIO_NUM_4
#define PIN_RST   GPIO_NUM_2
#define PIN_IRQ   GPIO_NUM_27

void spi_master_init(void);
esp_err_t spi_transfer(uint8_t *tx, uint8_t *rx, size_t len);
