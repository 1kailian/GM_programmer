#include "ftd_mw_powermon_manager.h"
#include "ftd_mw_log_manager.h"

/**
 * @brief  Configure the register write for INA236 using I2C
 * @param u8SlaveAddr      Slave address of the INA236 sensor
 * @param ConfigWord       Configuration word to be written
 *
 * @return
 *     - length of data written
 */
static uint32_t ftd_mw_powermon_manager_set_config(uint8_t u8SlaveAddr, uint16_t ConfigWord)
{
    uint8_t SentTable[2];

    SentTable[0] = (ConfigWord & 0xFF00) >> 8;
    SentTable[1] = (ConfigWord & 0x00FF);

    return ftd_drv_ina236_i2c_write(u8SlaveAddr, INA236_CONFIG, SentTable, 2);  // Fixed: len should be 2, not 3
}

/**
 * @brief  Calibrate the register write for INA236 using I2C
 * @param u8SlaveAddr      Slave address of the INA236 sensor
 * @param CalibrationWord   Calibration word to be written
 *
 * @return
 *     - length of data written
 */
static uint32_t ftd_mw_powermon_manager_set_calibration(uint8_t u8SlaveAddr, uint16_t CalibrationWord)
{
    uint8_t SentTable[2];
    SentTable[0] = (CalibrationWord & 0xFF00) >> 8;
    SentTable[1] = (CalibrationWord & 0x00FF);
    return ftd_drv_ina236_i2c_write(u8SlaveAddr, INA236_CALIB, SentTable, 2);  // Fixed: len should be 2, not 3
}

/**
 * @brief  Mask the register write for INA236 using I2C
 * @param u8SlaveAddr      Slave address of the INA236 sensor
 * @param MaskWord         Mask word to be written
 *
 * @return
 *     - length of data written
 */
static uint32_t ftd_mw_powermon_manager_set_mask(uint8_t u8SlaveAddr, uint16_t MaskWord)
{
    uint8_t SentTable[2];
    SentTable[0] = (MaskWord & 0xFF00) >> 8;
    SentTable[1] = (MaskWord & 0x00FF);
    return ftd_drv_ina236_i2c_write(u8SlaveAddr, INA236_MASK, SentTable, 2);  // Fixed: len should be 2, not 3
}

/**
 * @brief  alertl the register write for INA236 using I2C
 * @param u8SlaveAddr      Slave address of the INA236 sensor
 * @param AlertlWord         alertl word to be written
 *
 * @return
 *     - length of data written
 */
static uint32_t ftd_mw_powermon_manager_set_alertl(uint8_t u8SlaveAddr, uint16_t AlertlWord)
{
    uint8_t SentTable[2];
    SentTable[0] = (AlertlWord & 0xFF00) >> 8;
    SentTable[1] = (AlertlWord & 0x00FF);
    return ftd_drv_ina236_i2c_write(u8SlaveAddr, INA236_ALERTL, SentTable, 2);  // Fixed: len should be 2, not 3
}

/**
 * @brief  Read the configuration register of INA236 using I2C
 * @param u8SlaveAddr      Slave address of the INA236 sensor
 *
 * @return
 *     - 16-bit value of the configuration register
 */
 // Read configuration register
static uint16_t ftd_mw_powermon_manager_get_config(uint8_t u8SlaveAddr)
{
    uint8_t ReceivedTable[2];
    if (ftd_drv_ina236_i2c_read(u8SlaveAddr, INA236_CONFIG, ReceivedTable, 2) == 0)
    {
        return 0xFFFF;
    }
    return ((uint16_t)ReceivedTable[0] << 8 | ReceivedTable[1]);
}



/**
 * @brief  Read the calibration register of INA236 using I2C
 * @param u8SlaveAddr      Slave address of the INA236 sensor
 *
 * @return
 *     - 16-bit value of the calibration register
 */
 // Read calibration register
static uint16_t ftd_mw_powermon_manager_get_calibration(uint8_t u8SlaveAddr)
{
    uint8_t ReceivedTable[2];
    if (ftd_drv_ina236_i2c_read(u8SlaveAddr, INA236_CALIB, ReceivedTable, 2) == 0)
    {
        return 0xFFFF;
    }
    return ((uint16_t)ReceivedTable[0] << 8 | ReceivedTable[1]);
}

/**
 * @brief  Read the mask register of INA236 using I2C
 * @param u8SlaveAddr      Slave address of the INA236 sensor
 *
 * @return
 *     - 16-bit value of the mask register
 */
static uint16_t ftd_mw_powermon_manager_get_mask(uint8_t u8SlaveAddr)
{
    uint8_t ReceivedTable[2];
    if (ftd_drv_ina236_i2c_read(u8SlaveAddr, INA236_MASK, ReceivedTable, 2) == 0)
    {
        return 0xFFFF;
    }
    return ((uint16_t)ReceivedTable[0] << 8 | ReceivedTable[1]);
}

/**
 * @brief  Read the alertl register of INA236 using I2C
 * @param u8SlaveAddr      Slave address of the INA236 sensor
 *
 * @return
 *     - 16-bit value of the alertl register
 */
 // Read alertl register
static uint16_t ftd_mw_powermon_manager_get_alertl(uint8_t u8SlaveAddr)
{
    uint8_t ReceivedTable[2];
    if (ftd_drv_ina236_i2c_read(u8SlaveAddr, INA236_ALERTL, ReceivedTable, 2) == 0)
    {
        return 0xFFFF;
    }
    return ((uint16_t)ReceivedTable[0] << 8 | ReceivedTable[1]);
}

/**
 * @brief  Read the bus voltage register of INA236 using I2C
 * @param u8SlaveAddr      Slave address of the INA236 sensor
 *
 * @return
 *     - 16-bit value of the bus voltage register
 */
static uint16_t ftd_mw_powermon_manager_get_bus_voltage_reg(uint8_t u8SlaveAddr)
{
    uint8_t ReceivedTable[2];
    if (ftd_drv_ina236_i2c_read(u8SlaveAddr, INA236_BUSV, ReceivedTable, 2) == 0) return 0xFFFF;
    else return ((uint16_t)ReceivedTable[0] << 8 | ReceivedTable[1]);
}

/**
 * @brief  Read the shunt voltage register of INA236 using I2C
 * @param u8SlaveAddr      Slave address of the INA236 sensor
 *
 * @return
 *     - 16-bit value of the shunt voltage register
 */
static uint16_t ftd_mw_powermon_manager_get_shunt_voltage_reg(uint8_t u8SlaveAddr)
{
    uint8_t ReceivedTable[2];
    if (ftd_drv_ina236_i2c_read(u8SlaveAddr, INA236_SHUNTV, ReceivedTable, 2) == 0) return 0xFFFF;
    else return ((uint16_t)ReceivedTable[0] << 8 | ReceivedTable[1]);
}
/**
 * @brief  Convert bus voltage register to mV
 * @param u8SlaveAddr      Slave address of the INA236 sensor
 *
 * @return
 *     - Bus voltage in mV
 */
 // Convert bus voltage register to mV
 // INA236 Bus Voltage LSB = 1.6 mV (from datasheet)
float ftd_mw_powermon_manager_get_bus_voltage(uint8_t u8SlaveAddr)
{
    uint16_t u16BusVReg = ftd_mw_powermon_manager_get_bus_voltage_reg(u8SlaveAddr);
    if (u16BusVReg == 0xFFFF)
    {
        return 0.0;
    }
    // Bus Voltage LSB = 1.6 mV/bit
    return (float)u16BusVReg * INA236_BUS_VOLTAGE_LSB_MV;
}

/**
 * @brief  Convert shunt voltage register to mV
 * @param u8SlaveAddr      Slave address of the INA236 sensor
 *
 * @return
 *     - Shunt voltage in mV
 */
 // Convert shunt voltage register to mV
 // INA236 Shunt Voltage is signed 16-bit, LSB = 2.5 ??V (from datasheet)
float ftd_mw_powermon_manager_get_shunt_voltage(uint8_t u8SlaveAddr)
{
    uint16_t shunt_raw = ftd_mw_powermon_manager_get_shunt_voltage_reg(u8SlaveAddr);
    if (shunt_raw == 0xFFFF)
    {
        return 0.0;
    }
    // Shunt Voltage LSB = 2.5 uV/bit = 0.0025 mV/bit
    FTD_LOG_INFO("INA236 shunt voltage: %02x\n", shunt_raw); // Debug: Print raw shunt voltage
    return (float)shunt_raw * INA236_SHUNT_VOLTAGE_LSB_MV;
}

/**
 * @brief  Read the power register of INA236 using I2C
 * @param u8SlaveAddr      Slave address of the INA236 sensor
 *
 * @return
 *     - 16-bit value of the power register
 */
static uint16_t ftd_mw_powermon_manager_get_power_reg(uint8_t u8SlaveAddr)
{
    uint8_t ReceivedTable[2];
    if (ftd_drv_ina236_i2c_read(u8SlaveAddr, INA236_POWER, ReceivedTable, 2) == 0)
    {
        FTD_LOG_INFO("INA236 read power error %2x\n", ((uint16_t)ReceivedTable[0] << 8 | ReceivedTable[1]));
        return 0xFFFF;
    }
    else
    {
        return ((uint16_t)ReceivedTable[0] << 8 | ReceivedTable[1]);
    }
}

/**
 * @brief  Read the current register of INA236 using I2C
 * @param u8SlaveAddr      Slave address of the INA236 sensor
 *
 * @return
 *     - 16-bit value of the current register
 */
static int16_t ftd_mw_powermon_manager_get_current_reg(uint8_t u8SlaveAddr)
{
    uint8_t ReceivedTable[2];
    if (ftd_drv_ina236_i2c_read(u8SlaveAddr, INA236_CURRENT, ReceivedTable, 2) == 0)
    {
        return 0x7FFF;  // Return max positive value on error
    }
    else
    {
        // Combine bytes and treat as signed 16-bit (two's complement)
        int16_t current_raw = (int16_t)((ReceivedTable[0] << 8) | ReceivedTable[1]);
        return current_raw;
    }
}

/**
 * @brief  Convert current register to mA
 * @param u8SlaveAddr      Slave address of the INA236 sensor
 *
 * @return
 *     - Current in mA
 */
float ftd_mw_powermon_manager_get_current(uint8_t u8SlaveAddr)
{
    int16_t i16CurrentReg = ftd_mw_powermon_manager_get_current_reg(u8SlaveAddr);
    if (i16CurrentReg == 0x7FFF)
    {
        return 0.0;  // Return 0 on error
    }
    // Convert signed register value to current in mA (can be negative for reverse current)
    return (float)i16CurrentReg * INA236_CURRENT_LSB_MA;
}

/**
 * @brief  Convert power register to mW
 * @param u8SlaveAddr      Slave address of the INA236 sensor
 *
 * @return
 *     - Power in mW
 */
float ftd_mw_powermon_manager_get_power(uint8_t u8SlaveAddr)
{
    uint16_t u16PowerReg = ftd_mw_powermon_manager_get_power_reg(u8SlaveAddr);
    if (u16PowerReg == 0xFFFF)
    {
        FTD_LOG_INFO("INA236 read power error\n");
        return 0.0;
    }
    else {
        FTD_LOG_INFO("INA236 read power: %02X\n", u16PowerReg);
        return (float)u16PowerReg * INA236_POWER_LSB;
    }
}

/**
 * @brief  Clear alert flag of INA236   sensor
 *
 * @param u8SlaveAddr      Slave address of the INA236 sensor
 *
 */
void ftd_mw_powermon_manager_clear_alert_mask_flag(uint8_t u8SlaveAddr)
{
    uint16_t alertl = ftd_mw_powermon_manager_get_mask(u8SlaveAddr);
    FTD_LOG_INFO("INA236 clear alert mask: %02X\n", alertl);
}


/**
 * @brief  Initialize the INA236 power monitor manager
 *
 * This function initializes the INA236 power monitor manager by initializing the I2C driver
 * and verifying the presence and configuration of up to four INA236 devices on the I2C bus.
 */
void ftd_mw_powermon_manager_init(void)
{
    ftd_drv_ina236_i2c_init();

    // Configure all 4 INA236 devices with error checking
    uint32_t ret;
    uint8_t addr_list[4] = { POWERMON_SAVE_ADDR_1, POWERMON_SAVE_ADDR_2, POWERMON_SAVE_ADDR_3, POWERMON_SAVE_ADDR_4 };

    FTD_LOG_INFO("Starting INA236 device verification...\n");

    for (uint8_t i = 0; i < 4; i++)
    {
        bool device_ok = true;

        // Read Manufacturer ID to verify device presence
        uint8_t manuf_buf[2];
        if (ftd_drv_ina236_i2c_read(addr_list[i], INA236_MANUF_ID, manuf_buf, 2) == 2)
        {
            uint16_t manuf_id = (manuf_buf[0] << 8) | manuf_buf[1];
            if (manuf_id == INA236_MANUF_ID_DEFAULT)
            {
                FTD_LOG_INFO("  Ch%d (0x%02X): Manuf ID=0x%04X [OK]\n", i + 1, addr_list[i], manuf_id);
            }
            else
            {
                FTD_LOG_WARN("  Ch%d (0x%02X): Manuf ID=0x%04X [FAIL] Expected 0x5449\n",
                    i + 1, addr_list[i], manuf_id);
                device_ok = false;
            }
        }
        else
        {
            FTD_LOG_WARN("  Ch%d (0x%02X): Manuf ID read failed - Device not responding\n", i + 1, addr_list[i]);
            device_ok = false;
        }

        // Only configure if device verification passed
        if (device_ok)
        {
            // Write and verify configuration register
            ret = ftd_mw_powermon_manager_set_config(addr_list[i], INA236_CONFIG_DEFAULT);
            if (ret != 2)
            {
                FTD_LOG_WARN("  Ch%d (0x%02X): Config write failed (ret=%d)\n", i + 1, addr_list[i], ret);
            }
            else
            {
                // Read back to verify
                uint16_t config_readback = ftd_mw_powermon_manager_get_config(addr_list[i]);
                if (config_readback == INA236_CONFIG_DEFAULT)
                {
                    FTD_LOG_INFO("  Ch%d (0x%02X): Config verified 0x%04X [OK]\n", i + 1, addr_list[i], config_readback);
                }
                else
                {
                    FTD_LOG_WARN("  Ch%d (0x%02X): Config mismatch! Write=0x%04X Read=0x%04X [FAIL]\n",
                        i + 1, addr_list[i], INA236_CONFIG_DEFAULT, config_readback);
                }
            }

            // Write and verify calibration register
            ret = ftd_mw_powermon_manager_set_calibration(addr_list[i], INA236_CALIB_VAL);
            if (ret != 2)
            {
                FTD_LOG_WARN("  Ch%d (0x%02X): Calibration write failed (ret=%d)\n", i + 1, addr_list[i], ret);
            }
            else
            {
                // Read back to verify
                uint16_t calib_readback = ftd_mw_powermon_manager_get_calibration(addr_list[i]);
                if (calib_readback == INA236_CALIB_VAL)
                {
                    FTD_LOG_INFO("  Ch%d (0x%02X): Calibration verified 0x%04X [OK]\n", i + 1, addr_list[i], calib_readback);
                    FTD_LOG_INFO("  Ch%d (0x%02X): Configuration complete\n", i + 1, addr_list[i]);
                }
                else
                {
                    FTD_LOG_WARN("  Ch%d (0x%02X): Calibration mismatch! Write=0x%04X Read=0x%04X [FAIL]\n",
                        i + 1, addr_list[i], INA236_CALIB_VAL, calib_readback);
                }
            }


            // Set mask register
            ret = ftd_mw_powermon_manager_set_mask(addr_list[i], (INA236_MER_APOL | INA236_MER_SOL | INA236_MER_LEN));
            if (ret != 2)
            {
                FTD_LOG_WARN("  Ch%d (0x%02X): mask write failed (ret=%d)\n", i + 1, addr_list[i], ret);
            }
            else
            {
                uint16_t mask_readback = ftd_mw_powermon_manager_get_mask(addr_list[i]);
                if (mask_readback == (INA236_MER_APOL | INA236_MER_SOL | INA236_MER_LEN))
                {
                    FTD_LOG_INFO("  Ch%d (0x%02X): mask verified 0x%04X [OK]\n", i + 1, addr_list[i], mask_readback);
                    FTD_LOG_INFO("  Ch%d (0x%02X): Configuration complete\n", i + 1, addr_list[i]);
                }
                else
                {
                    FTD_LOG_WARN("  Ch%d (0x%02X): alertl mismatch! Write=0x%04X Read=0x%04X [FAIL]\n",
                        i + 1, addr_list[i], (INA236_MER_APOL | INA236_MER_SOL | INA236_MER_LEN), mask_readback);
                }
            }

            // Set alertl register
            uint16_t AlertlWord = (INA235_ALERTL_CURRENT_VAL * INA236_SHUNT_RESISTANCE) / INA236_SHUNT_VOLTAGE_LSB_MV; // Convert to mV
            FTD_LOG_INFO("  Ch%d (0x%02X): AlertlWord = 0x%04X\n", i + 1, addr_list[i], AlertlWord);
            ret = ftd_mw_powermon_manager_set_alertl(addr_list[i], AlertlWord);
            if (ret != 2)
            {
                FTD_LOG_WARN("  Ch%d (0x%02X): alertl write failed (ret=%d)\n", i + 1, addr_list[i], ret);
            }
            else
            {
                // Read back to verify
                uint16_t alertl_readback = ftd_mw_powermon_manager_get_alertl(addr_list[i]);
                if (alertl_readback == AlertlWord)
                {
                    FTD_LOG_INFO("  Ch%d (0x%02X): alertl verified 0x%04X [OK]\n", i + 1, addr_list[i], alertl_readback);
                    FTD_LOG_INFO("  Ch%d (0x%02X): Configuration complete\n", i + 1, addr_list[i]);
                }
                else
                {
                    FTD_LOG_WARN("  Ch%d (0x%02X): alertl mismatch! Write=0x%04X Read=0x%04X [FAIL]\n",
                        i + 1, addr_list[i], AlertlWord, alertl_readback);
                }
            }
        }
        else
        {
            FTD_LOG_WARN("  Ch%d (0x%02X): Skipping configuration due to verification failure\n", i + 1, addr_list[i]);
        }


    }

    //clear the alert mask flag
    for (int i = 0; i < sizeof(addr_list) / sizeof(addr_list[0]); i++)
    {
        ftd_mw_powermon_manager_clear_alert_mask_flag(addr_list[i]);
    }

    FTD_LOG_INFO("INA236 power monitor init done\n");
}


/**
 * @brief  Test the INA236 power monitor manager
 *
 * This function tests the INA236 power monitor manager by reading and printing the shunt
 * voltage, bus voltage, current, and power values for up to four INA236 devices on the I2C bus.
 */
void ftd_mw_powermon_manager_test(void)
{


    uint8_t addr_list[4] = { POWERMON_SAVE_ADDR_1, POWERMON_SAVE_ADDR_2, POWERMON_SAVE_ADDR_3, POWERMON_SAVE_ADDR_4 };
    static int count = 3;

    FTD_LOG_INFO("\n========== Channel %d (Addr 0x%02X) ==========\n", count + 1, addr_list[count]);

    // Read all voltage and power values
    float shunt_mv = ftd_mw_powermon_manager_get_shunt_voltage(addr_list[count]);
    float bus_mv = ftd_mw_powermon_manager_get_bus_voltage(addr_list[count]);
    float current_ma = ftd_mw_powermon_manager_get_current(addr_list[count]);
    float power_mw = ftd_mw_powermon_manager_get_power(addr_list[count]);

    // ftd_mw_powermon_manager_clear_alert_mask_flag(addr_list[count]);
    // Print measured values
    FTD_LOG_INFO("  Shunt Voltage : %+8.4f mV\n", shunt_mv);
    FTD_LOG_INFO("  Bus Voltage   : %8.2f mV\n", bus_mv);
    FTD_LOG_INFO("  Current       : %+8.4f mA\n", current_ma);
    FTD_LOG_INFO("  Power         : %8.4f mW\n", power_mw);

    count++;
    if (count == 4)
        count = 0;
}

