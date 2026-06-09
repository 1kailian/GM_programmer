#ifndef FTD_MW_POWERMON_MANAGER_H
#define FTD_MW_POWERMON_MANAGER_H

#include "ftd_drv_ina236_i2c.h"



#define POWERMON_SAVE_ADDR_1 INA236_I2C_ADDR_1
#define POWERMON_SAVE_ADDR_2 INA236_I2C_ADDR_2
#define POWERMON_SAVE_ADDR_3 INA236_I2C_ADDR_3
#define POWERMON_SAVE_ADDR_4 INA236_I2C_ADDR_4



void ftd_mw_powermon_manager_init(void); // Initialize power monitor manager
float ftd_mw_powermon_manager_get_current(uint8_t u8SlaveAddr);
float ftd_mw_powermon_manager_get_power(uint8_t u8SlaveAddr);
void ftd_mw_powermon_manager_clear_alert_mask_flag(uint8_t u8SlaveAddr);


void ftd_mw_powermon_manager_test(void);
#endif

