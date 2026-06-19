#pragma once

#define I2C_MASTER_SCL_IO           22           //pIN SCL
#define I2C_MASTER_SDA_IO           21           //PIn SDA
#define I2C_MASTER_NUM              I2C_NUM_0    //Puerto

esp_err_t init_I2C_Master(void);
void scanI2C_Devices(void);
