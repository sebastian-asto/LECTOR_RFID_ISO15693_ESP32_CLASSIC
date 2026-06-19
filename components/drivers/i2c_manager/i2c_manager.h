#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"

#define I2C_MASTER_SCL_IO           22           //pIN SCL
#define I2C_MASTER_SDA_IO           21           //PIn SDA
#define I2C_MASTER_NUM              I2C_NUM_0    //Puerto

esp_err_t init_I2C_Master(void);
i2c_master_bus_handle_t i2c_manager_get_bus_handle(void);
void scanI2C_Devices(void);
