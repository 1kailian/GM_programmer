#include "ftd_drv_ina236_i2c.h"
#include "ftd_mw_log_manager.h"

void ftd_drv_ina236_i2c_init(void)
{
    SYS_UnlockReg();
    CLK_EnableModuleClock(I2C_INA236_MODULE_CLK);


    /* Open I2C1 and set clock to 400k */
    I2C_Open(I2C_INA236_PORT, I2C_INA236_SPEED);
    FTD_LOG_INFO("I2c init done\n");
    SYS_LockReg();
}

uint32_t ftd_drv_ina236_i2c_write(uint8_t u8SlaveAddr, uint8_t u8DataAddr, uint8_t* data, uint8_t len)
{
    return I2C_WriteMultiBytesOneReg(I2C_INA236_PORT, u8SlaveAddr, u8DataAddr, data, len);
}

uint32_t ftd_drv_ina236_i2c_read(uint8_t u8SlaveAddr, uint8_t u8DataAddr, uint8_t* data, uint8_t len)
{
    return I2C_ReadMultiBytesOneReg(I2C_INA236_PORT, u8SlaveAddr, u8DataAddr, data, len);
}

