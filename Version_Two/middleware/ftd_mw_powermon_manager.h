#ifndef FTD_MW_POWERMON_MANAGER_H
#define FTD_MW_POWERMON_MANAGER_H

#include "ftd_drv_ina236_i2c.h"



#define POWERMON_SAVE_ADDR_1  INA236_I2C_ADDR_3
#define POWERMON_SAVE_ADDR_2  INA236_I2C_ADDR_2
#define POWERMON_SAVE_ADDR_3  INA236_I2C_ADDR_4
#define POWERMON_SAVE_ADDR_4  INA236_I2C_ADDR_1

#define POWERMON_CURRENT_MAX 20.0f  //20mA
#define POWERMON_CURRENT_MIN 1.0f //1mA

#define POWERMON_BUS_VOLTAGE 3300.0f  //3.3Vbus
#define POWERMON_BUS_VOLTAGE_OFFSET 150.0f  //0.15Vshunt

void ftd_mw_powermon_manager_init(void); // Initialize power monitor manager
float ftd_mw_powermon_manager_get_current(uint8_t u8SlaveAddr);
float ftd_mw_powermon_manager_get_power(uint8_t u8SlaveAddr);
float ftd_mw_powermon_manager_get_shunt_voltage(uint8_t u8SlaveAddr);
void ftd_mw_powermon_manager_clear_alert_mask_flag(uint8_t u8SlaveAddr);

int8_t ftd_mw_powermon_manager_get_burn_current_status(uint8_t burn_ch, float current_min, float current_max);
int8_t ftd_mw_powermon_manager_get_burn_bus_voltage_status(uint8_t burn_ch, float voltage_target, float voltage_offset);

int ftd_mw_powermon_manager_get_burn_status(uint8_t burn_ch);
float ftd_mw_powermon_manager_get_channel_current(uint8_t burn_ch);

void ftd_mw_powermon_manager_test(void);
#endif

