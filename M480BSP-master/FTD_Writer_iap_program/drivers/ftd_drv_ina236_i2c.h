#ifndef FTD_DRV_INA236_I2C_H
#define FTD_DRV_INA236_I2C_H

#include <stdint.h>
#include <stdbool.h>
#include "NuMicro.h"

#define I2C_INA236_PORT    I2C1
#define I2C_INA236_SPEED    300000
#define I2C_INA236_MODULE_CLK    I2C1_MODULE

#define INA236_I2C_ADDR_1 0x40
#define INA236_I2C_ADDR_2 0x41
#define INA236_I2C_ADDR_3 0x42 
#define INA236_I2C_ADDR_4 0x43


//分流电阻的阻值为2Ω
#define INA236_SHUNT_RESISTANCE 1
#define INA236_CONFIG_DEFAULT 0x4127 //配置寄存器默认值
#define INA236_CALIB_VAL 0x0800   //基于实际电流值计算得到的校准值
/*
    公式;
    (1) SHUNT_CAL = 0.00512 / (Current LSB * Shunt resistance)
    (2) current LSB = Maximum current / 2^15
    (3) current = CURRENT_CAL * current LSB
    (4) power = 32 * current LSB * POWER_CAL
    计算过程：
    Maximum voltage = 81.92mV；
    Maximum Current = Maximum voltage / Shunt resistance = 81.92mV / 2Ω = 40.96mA；
    根据(2)计算current LSB = 40.96mA / 2^15 = 0.00125mA/bit；
    根据(1)SHUNT_CAL = 0.00512 / (0.00125mA/bit * 2Ω) = 2048 = 0x0800；

        Maximum voltage = 81.92mV；
    Maximum Current = Maximum voltage / Shunt resistance = 81.92mV / 1Ω = 81.92mA；
    根据(2)计算current LSB = 81.82mA / 2^15 = 0.0025mA/bit；
    根据(1)SHUNT_CAL = 0.00512 / (0.0025mA/bit * 1Ω) = 2048 = 0x0800；

*/
#define INA236_VOLTAGE_FULL_SCALE 81.92f //mV
#define INA236_MAXIMUM_CURRENT    (INA236_VOLTAGE_FULL_SCALE / INA236_SHUNT_RESISTANCE) //mA
#define INA236_CURRENT_LSB_MA    (INA236_MAXIMUM_CURRENT / 32768) //mA/bit


#define INA236_CURRENT_LSB_UA INA236_CURRENT_LSB_MA*1000 // uA/bit
// #define INA236_CURRENT_LSB_MA 0.00125f // mA/bit
#define INA236_SHUNT_VOLTAGE_LSB_MV  0.0025f // mV/bit //根据手册得出的最低有效位LSB的阶跃幅度 2.5uV/bit
#define INA236_BUS_VOLTAGE_LSB_MV  1.6f // mV/bit //根据手册得出的最低有效位LSB的阶跃幅度 1.6mV/bit
#define INA236_CURRENT_LSB_INV 1/INA236_CURRENT_LSB_MA // bit/mA   
#define INA236_POWER_LSB         (INA236_CURRENT_LSB_MA*32) // mW/bit
//Power[w] = 32*INA236_CURRENT_LSB_MA*POWER

#define INA235_ALERTL_CURRENT_VAL 60   //30mA

#define INA236_CONFIG 	0x00 // Configuration Register (R/W)初始值4127
#define INA236_SHUNTV 	0x01 // Shunt Voltage (R)初始值0，分流电压测量值
#define INA236_BUSV 	0x02 // Bus Voltage (R)初始值0，总线电压测量值
#define INA236_POWER 	0x03 // Power (R)初始值0，输出功率测量值
#define INA236_CURRENT 	0x04 // Current (R)初始值0，分流电阻电流计算值
#define INA236_CALIB 	0x05 // Calibration (R/W)，设置全量程和电流LSB
#define INA236_MASK 	0x06 // Mask/Enable (R/W)，报警设置和转换准备标志
#define INA236_ALERTL 	0x07 // Alert Limit (R/W)，报警阈值
#define INA236_MANUF_ID 0x3E // Manufacturer ID (R)，0x5449
#define INA236_DIE_ID 	0x3F // Die ID (R),0xA080

#define INA236_MODE_POWER_DOWN 			(0<<0) // Power-Down
#define INA236_MODE_TRIG_SHUNT_VOLTAGE 	(1<<0) // Shunt Voltage, Triggered
#define INA236_MODE_TRIG_BUS_VOLTAGE 	(2<<0) // Bus Voltage, Triggered
#define INA236_MODE_TRIG_SHUNT_AND_BUS 	(3<<0) // Shunt and Bus, Triggered
#define INA236_MODE_POWER_DOWN2 		(4<<0) // Power-Down
#define INA236_MODE_CONT_SHUNT_VOLTAGE 	(5<<0) // Shunt Voltage, Continuous
#define INA236_MODE_CONT_BUS_VOLTAGE 	(6<<0) // Bus Voltage, Continuous
#define INA236_MODE_CONT_SHUNT_AND_BUS 	(7<<0) // Shunt and Bus, Continuous

// Shunt Voltage Conversion Time
#define INA236_VSH_140uS 	(0<<3)
#define INA236_VSH_204uS 	(1<<3)
#define INA236_VSH_332uS 	(2<<3)
#define INA236_VSH_588uS 	(3<<3)
#define INA236_VSH_1100uS 	(4<<3)
#define INA236_VSH_2116uS 	(5<<3)
#define INA236_VSH_4156uS 	(6<<3)
#define INA236_VSH_8244uS 	(7<<3)

// Bus Voltage Conversion Time (VBUS CT Bit Settings[6-8])
#define INA236_VBUS_140uS 	(0<<6)
#define INA236_VBUS_204uS 	(1<<6)
#define INA236_VBUS_332uS 	(2<<6)
#define INA236_VBUS_588uS 	(3<<6)
#define INA236_VBUS_1100uS 	(4<<6)
#define INA236_VBUS_2116uS 	(5<<6)
#define INA236_VBUS_4156uS 	(6<<6)
#define INA236_VBUS_8244uS 	(7<<6)

// Averaging Mode (AVG Bit Settings[9-11])
#define INA236_AVG_1 	(0<<9)
#define INA236_AVG_4 	(1<<9)
#define INA236_AVG_16 	(2<<9)
#define INA236_AVG_64 	(3<<9)
#define INA236_AVG_128 	(4<<9)
#define INA236_AVG_256 	(5<<9)
#define INA236_AVG_512 	(6<<9)
#define INA236_AVG_1024 (7<<9)

// Reset Bit (RST bit [15])
#define INA236_RESET_ACTIVE 	(1<<15)
#define INA236_RESET_INACTIVE 	(0<<15)

// Mask/Enable Register
#define INA236_MER_SOL 	(1<<15) // Shunt Voltage Over-Voltage
#define INA236_MER_SUL 	(1<<14) // Shunt Voltage Under-Voltage
#define INA236_MER_BOL 	(1<<13) // Bus Voltagee Over-Voltage
#define INA236_MER_BUL 	(1<<12) // Bus Voltage Under-Voltage
#define INA236_MER_POL 	(1<<11) // Power Over-Limit
#define INA236_MER_CNVR (1<<10) // Conversion Ready
#define INA236_MER_AFF 	(1<<4) // Alert Function Flag
#define INA236_MER_CVRF (1<<3) // Conversion Ready Flag
#define INA236_MER_OVF 	(1<<2) // Math Overflow Flag
#define INA236_MER_APOL (1<<1) // Alert Polarity Bit
#define INA236_MER_LEN 	(1<<0) // Alert Latch Enable

#define INA236_MANUF_ID_DEFAULT 	0x5449
#define INA236_DIE_ID_DEFAULT 		0xA080

void ftd_drv_ina236_i2c_init(void); // Initialize I2C
uint32_t ftd_drv_ina236_i2c_write(uint8_t u8SlaveAddr, uint8_t u8DataAddr, uint8_t* data, uint8_t len); // Write data to INA236
uint32_t ftd_drv_ina236_i2c_read(uint8_t u8SlaveAddr, uint8_t u8DataAddr, uint8_t* data, uint8_t len); // Read data from INA236

#endif
