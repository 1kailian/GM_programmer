/****************************************************************************
@FILENAME:  ftd_mw_slave_manager.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.23
@COPYRIGHT: FTD co.ltd
****************************************************************************/

#include "ftd_mw_sys_info_manager.h"
#include "ftd_mw_log_manager.h"
#include "ftd_mw_slave_manager.h"
#include "ftd_mw_fw_manager.h"
#include "ftd_mw_powermon_manager.h"
#include "ftd_mw_rf_manager.h"
#include "ftd_drv_gpio.h"
#include "ftd_drv_burn_uart.h"
#include "ftd_drv_rf_uart.h"
#include "ftd_drv_flash_access.h"
#include "ftd_drv_iap_fmc.h"
#include "ftd_mw_misc_manager.h"
#include "ftd_utils.h"

#include "NuMicro.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
static uint8_t SLAVE_CHN1_FUFF[256];

uint8_t burn_test_addr = 0;
/*---------------------------------------------------------------------------------------------------------*/
/* Define functions                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/* Chip Flash Address Map Table                                                                            */
/* Based on chip vendor datasheet - DO NOT modify unless chip spec changes                                 */
/*---------------------------------------------------------------------------------------------------------*/
static const CHIP_FLASH_ADDR_MAP g_chip_flash_map_table[] =
{
    // A Series Chips (FL100) - 2M/4M Flash only, no 1M Flash support
    // NO.1: FM124AM
    {
        .chip_name = "FM124AM",
        .chip_code = 0x2202,
        .chip_family = CHIP_FAMILY_FL100,
        .fw_bin_a_addr = 0x08008000,
        .fw_bin_b_addr = 0x08020000,
        .fw_size = 96 * 1024,       // 96KB
        .fw_bin_a_addr_1m = 0,               // Not supported
        .fw_bin_b_addr_1m = 0,               // Not supported
        .fw_size_1m = 0,               // Not supported
        .config_a_addr = 0x08004000,
        .config_b_addr = 0x08005000,
        .config_size = 4 * 1024,        // 4KB
        .author_addr = 0x08002000,
        .author_size = 40,              // 40 Bytes
        .burn_info_addr = 0x08002100,   // Burn info address
        .burn_info_size = 30,           // Burn info size (30 bytes)
    },
    // NO.2: FM128AM
    {
        .chip_name = "FM128AM",
        .chip_code = 0x2203,
        .chip_family = CHIP_FAMILY_FL100,
        .fw_bin_a_addr = 0x08008000,
        .fw_bin_b_addr = 0x08020000,
        .fw_size = 96 * 1024,       // 96KB
        .fw_bin_a_addr_1m = 0,
        .fw_bin_b_addr_1m = 0,
        .fw_size_1m = 0,
        .config_a_addr = 0x08004000,
        .config_b_addr = 0x08005000,
        .config_size = 4 * 1024,
        .author_addr = 0x08002000,
        .author_size = 40,
        .burn_info_addr = 0x08002100,   // Burn info address
        .burn_info_size = 30,           // Burn info size (30 bytes)
    },
    // NO.3: FL116AM
    {
        .chip_name = "FL116AM",
        .chip_code = 0x2204,
        .chip_family = CHIP_FAMILY_FL100,
        .fw_bin_a_addr = 0x08008000,
        .fw_bin_b_addr = 0x08020000,
        .fw_size = 96 * 1024,
        .fw_bin_a_addr_1m = 0,
        .fw_bin_b_addr_1m = 0,
        .fw_size_1m = 0,
        .config_a_addr = 0x08004000,
        .config_b_addr = 0x08005000,
        .config_size = 4 * 1024,
        .author_addr = 0x08002000,
        .author_size = 40,
        .burn_info_addr = 0x08002100,   // Burn info address
        .burn_info_size = 30,           // Burn info size (30 bytes)
    },
    // NO.4: FL110AM
    {
        .chip_name = "FL110AM",
        .chip_code = 0x2205,
        .chip_family = CHIP_FAMILY_FL100,
        .fw_bin_a_addr = 0x08008000,
        .fw_bin_b_addr = 0x08020000,
        .fw_size = 96 * 1024,
        .fw_bin_a_addr_1m = 0,
        .fw_bin_b_addr_1m = 0,
        .fw_size_1m = 0,
        .config_a_addr = 0x08004000,
        .config_b_addr = 0x08005000,
        .config_size = 4 * 1024,
        .author_addr = 0x08002000,
        .author_size = 40,
        .burn_info_addr = 0x08002100,   // Burn info address
        .burn_info_size = 30,           // Burn info size (30 bytes)
    },

    // B Series Chips (FL100B) - Support both 2M/4M Flash and 1M Flash
    // NO.5: FM124BMT
    {
        .chip_name = "FM124BMT",
        .chip_code = 0x2206,
        .chip_family = CHIP_FAMILY_FL100B,
        .fw_bin_a_addr = 0x08006000,
        .fw_bin_b_addr = 0x08020000,
        .fw_size = 96 * 1024,
        .fw_bin_a_addr_1m = 0,
        .fw_bin_b_addr_1m = 0,
        .fw_size_1m = 0,
        .config_a_addr = 0x08004000,
        .config_b_addr = 0x08005000,
        .config_size = 4 * 1024,
        .author_addr = 0x08002000,
        .author_size = 40,
        .burn_info_addr = 0x08002100,   // Burn info address
        .burn_info_size = 30,           // Burn info size (30 bytes)
    },
    // NO.6: FM128BMT
    {
        .chip_name = "FM128BMT",
        .chip_code = 0x2207,
        .chip_family = CHIP_FAMILY_FL100B,
        .fw_bin_a_addr = 0x08006000,
        .fw_bin_b_addr = 0x08020000,
        .fw_size = 96 * 1024,
        .fw_bin_a_addr_1m = 0,
        .fw_bin_b_addr_1m = 0,
        .fw_size_1m = 0,
        .config_a_addr = 0x08004000,
        .config_b_addr = 0x08005000,
        .config_size = 4 * 1024,
        .author_addr = 0x08002000,
        .author_size = 40,
        .burn_info_addr = 0x08002100,   // Burn info address
        .burn_info_size = 30,           // Burn info size (30 bytes)
    },
    // NO.7: FL116BMT
    {
        .chip_name = "FL116BMT",
        .chip_code = 0x2208,
        .chip_family = CHIP_FAMILY_FL100B,
        .fw_bin_a_addr = 0,
        .fw_bin_b_addr = 0,
        .fw_size = 0,
        .fw_bin_a_addr_1m = 0x08006000,
        .fw_bin_b_addr_1m = 0x08010000,
        .fw_size_1m = 40 * 1024,
        .config_a_addr = 0x08004000,
        .config_b_addr = 0x08005000,
        .config_size = 4 * 1024,
        .author_addr = 0x08002000,
        .author_size = 40,
        .burn_info_addr = 0x08002100,   // Burn info address
        .burn_info_size = 30,           // Burn info size (30 bytes)
    },
    // NO.8: FL110BMT
    {
        .chip_name = "FL110BMT",
        .chip_code = 0x2209,
        .chip_family = CHIP_FAMILY_FL100B,
        .fw_bin_a_addr = 0,
        .fw_bin_b_addr = 0,
        .fw_size = 0,
        .fw_bin_a_addr_1m = 0x08006000,
        .fw_bin_b_addr_1m = 0x08010000,
        .fw_size_1m = 40 * 1024,
        .config_a_addr = 0x08004000,
        .config_b_addr = 0x08005000,
        .config_size = 4 * 1024,
        .author_addr = 0x08002000,
        .author_size = 40,
        .burn_info_addr = 0x08002100,   // Burn info address
        .burn_info_size = 30,           // Burn info size (30 bytes)
    },

    // NO.8: FM124BMG
    {
        .chip_name = "FM124BMG",
        .chip_code = 0x2210,
        .chip_family = CHIP_FAMILY_FL100B,
        .fw_bin_a_addr = 0x08006000,
        .fw_bin_b_addr = 0x08020000,
        .fw_size = 96 * 1024,
        .fw_bin_a_addr_1m = 0,
        .fw_bin_b_addr_1m = 0,
        .fw_size_1m = 0,
        .config_a_addr = 0x08004000,
        .config_b_addr = 0x08005000,
        .config_size = 4 * 1024,
        .author_addr = 0x08002000,
        .author_size = 40,
        .burn_info_addr = 0x08002100,   // Burn info address
        .burn_info_size = 30,           // Burn info size (30 bytes)
    },
    // NO.10: FM128BMG
    {
        .chip_name = "FM128BMG",
        .chip_code = 0x2211,
        .chip_family = CHIP_FAMILY_FL100B,
        .fw_bin_a_addr = 0x08006000,
        .fw_bin_b_addr = 0x08020000,
        .fw_size = 96 * 1024,
        .fw_bin_a_addr_1m = 0,
        .fw_bin_b_addr_1m = 0,
        .fw_size_1m = 0,
        .config_a_addr = 0x08004000,
        .config_b_addr = 0x08005000,
        .config_size = 4 * 1024,
        .author_addr = 0x08002000,
        .author_size = 40,
        .burn_info_addr = 0x08002100,   // Burn info address
        .burn_info_size = 30,           // Burn info size (30 bytes)
    },
    // NO.11: FL116BMG
    {
        .chip_name = "FL116BMG",
        .chip_code = 0x2212,
        .chip_family = CHIP_FAMILY_FL100B,
        .fw_bin_a_addr = 0,
        .fw_bin_b_addr = 0,
        .fw_size = 0,
        .fw_bin_a_addr_1m = 0x08006000,
        .fw_bin_b_addr_1m = 0x08010000,
        .fw_size_1m = 40 * 1024,
        .config_a_addr = 0x08004000,
        .config_b_addr = 0x08005000,
        .config_size = 4 * 1024,
        .author_addr = 0x08002000,
        .author_size = 40,
        .burn_info_addr = 0x08002100,   // Burn info address
        .burn_info_size = 30,           // Burn info size (30 bytes)
    },
    // NO.12: FL110BMG
    {
        .chip_name = "FL110BMG",
        .chip_code = 0x2213,
        .chip_family = CHIP_FAMILY_FL100B,
        .fw_bin_a_addr = 0,
        .fw_bin_b_addr = 0,
        .fw_size = 0,
        .fw_bin_a_addr_1m = 0x08006000,
        .fw_bin_b_addr_1m = 0x08010000,
        .fw_size_1m = 40 * 1024,
        .config_a_addr = 0x08004000,
        .config_b_addr = 0x08005000,
        .config_size = 4 * 1024,
        .author_addr = 0x08002000,
        .author_size = 40,
        .burn_info_addr = 0x08002100,   // Burn info address
        .burn_info_size = 30,           // Burn info size (30 bytes)
    },
    // NO.13: FM128BMGS
    {
        .chip_name = "FM128BMGS",
        .chip_code = 0x2214,
        .chip_family = CHIP_FAMILY_FL100B,
        .fw_bin_a_addr = 0x08006000,
        .fw_bin_b_addr = 0x08020000,
        .fw_size = 96 * 1024,
        .fw_bin_a_addr_1m = 0,
        .fw_bin_b_addr_1m = 0,
        .fw_size_1m = 0,
        .config_a_addr = 0x08004000,
        .config_b_addr = 0x08005000,
        .config_size = 4 * 1024,
        .author_addr = 0x08002000,
        .author_size = 40,
        .burn_info_addr = 0x08002100,   // Burn info address
        .burn_info_size = 30,           // Burn info size (30 bytes)
    },
    // NO.14: FM128BMTS
    {
        .chip_name = "FM128BMTS",
        .chip_code = 0x2215,
        .chip_family = CHIP_FAMILY_FL100B,
        .fw_bin_a_addr = 0x08006000,
        .fw_bin_b_addr = 0x08020000,
        .fw_size = 96 * 1024,
        .fw_bin_a_addr_1m = 0,
        .fw_bin_b_addr_1m = 0,
        .fw_size_1m = 0,
        .config_a_addr = 0x08004000,
        .config_b_addr = 0x08005000,
        .config_size = 4 * 1024,
        .author_addr = 0x08002000,
        .author_size = 40,
        .burn_info_addr = 0x08002100,   // Burn info address
        .burn_info_size = 30,           // Burn info size (30 bytes)
    },
    // NO.9: FM01011A
    {
        .chip_name = "FM01011A",
        .chip_code = 0x1107,
        .chip_family = CHIP_FAMILY_PY32,
        .fw_bin_a_addr = 0x08002400,
        .fw_bin_b_addr = 0x08002400,
        .fw_size = 50 * 1024,
        .fw_bin_a_addr_1m = 0,
        .fw_bin_b_addr_1m = 0,
        .fw_size_1m = 0,
        .config_a_addr = 0x0800EC00,
        .config_b_addr = 0x0800EC00,
        .config_size = 4 * 1024,
        .author_addr = 0,
        .author_size = 0,
        .burn_info_addr = 0,           // Burn info address not supported
        .burn_info_size = 0,           // Burn info size not supported
    },
    // NO.10: FM02021A
    {
        .chip_name = "FM02021A",
        .chip_code = 0x2103,
        .chip_family = CHIP_FAMILY_CW32,
        .fw_bin_a_addr = 0x00002400,
        .fw_bin_b_addr = 0x00002400,
        .fw_size = 50 * 1024,
        .fw_bin_a_addr_1m = 0,
        .fw_bin_b_addr_1m = 0,
        .fw_size_1m = 0,
        .config_a_addr = 0x0000EC00,
        .config_b_addr = 0x0000EC00,
        .config_size = 4 * 1024,
        .author_addr = 0,
        .author_size = 0,
        .burn_info_addr = 0,           // Burn info address not supported
        .burn_info_size = 0,           // Burn info size not supported
    },
};

#define CHIP_FLASH_MAP_TABLE_SIZE   (sizeof(g_chip_flash_map_table) / sizeof(g_chip_flash_map_table[0]))

// Forward declaration of internal state machine function
static bool ftd_mw_slave_manager_start_calib_test(SLAVE_INFO* p_st_slave_info, CALIB_TEST_TYPE calib_type);

/**
 * @brief Get chip flash address map by chip code
 * @param chip_code: Chip code (e.g., 0x2202 for FM124AM)
 * @return Pointer to CHIP_FLASH_ADDR_MAP if found, NULL if not found
 */
const CHIP_FLASH_ADDR_MAP* ftd_mw_slave_get_chip_flash_map(uint16_t chip_code)
{
    for (uint8_t i = 0; i < CHIP_FLASH_MAP_TABLE_SIZE; i++)
    {
        if (g_chip_flash_map_table[i].chip_code == chip_code)
        {
            return &g_chip_flash_map_table[i];
        }
    }

    // Not found, return default (FM124AM - first entry)
    FTD_LOG_ERROR("Unknown chip_code: %d (0x%04X), using default FL116BM", chip_code, chip_code);
    return &g_chip_flash_map_table[0];
}

/**
 * @brief Get chip flash map table count
 * @return Number of entries in the chip flash map table
 */
uint8_t ftd_mw_slave_get_chip_flash_map_count(void)
{
    return CHIP_FLASH_MAP_TABLE_SIZE;
}

static void ftd_mw_slave_manager_power_off(uint8_t slave)
{
    ftd_drv_gpio_init();

    if (slave == 0)
    {
        GPIO_SET_LOW(PEN_CH1);
        GPIO_SET_HIGH(UART_EN_CH1);
    }
    else if (slave == 1)
    {
        GPIO_SET_LOW(PEN_CH2);
        GPIO_SET_HIGH(UART_EN_CH2);
    }
    else if (slave == 2)
    {
        GPIO_SET_LOW(PEN_CH3);
        GPIO_SET_HIGH(UART_EN_CH3);
    }
    else if (slave == 3)
    {
        GPIO_SET_LOW(PEN_CH4);
        GPIO_SET_HIGH(UART_EN_CH4);
    }
    else
        return;
}

static void ftd_mw_slave_manager_power_on(uint8_t slave)
{
    ftd_drv_gpio_init();

    if (slave == 0)
    {
        GPIO_SET_HIGH(PEN_CH1);
        GPIO_SET_LOW(UART_EN_CH1);
    }
    else if (slave == 1)
    {
        GPIO_SET_HIGH(PEN_CH2);
        GPIO_SET_LOW(UART_EN_CH2);
    }
    else if (slave == 2)
    {
        GPIO_SET_HIGH(PEN_CH3);
        GPIO_SET_LOW(UART_EN_CH3);
    }
    else if (slave == 3)
    {
        GPIO_SET_HIGH(PEN_CH4);
        GPIO_SET_LOW(UART_EN_CH4);
    }
    else
        return;
}

void ftd_mw_slave_manager_start_ld(SLAVE_INFO* p_st_slave_info)
{
    //uint8_t boot_loader_trigeer_cmd[10] = {0x55, 0x69, 0x55, 0x69, 0x55, 0x69, 0x55, 0x69, 0x55, 0x69}; //uart protocol
    uint8_t boot_loader_trigeer_cmd[2] = { 0x55, 0x96 }; //uart protocol

    //1.power off slave
    ftd_mw_slave_manager_power_off(p_st_slave_info->slave_uart_channel);
    CLK_SysTickDelay(10000); //10ms

    //2.power on slave,send bootloader trigger cmd five times(boot_loader_trigeer_cmd)
    ftd_mw_slave_manager_power_on(p_st_slave_info->slave_uart_channel);
    CLK_SysTickDelay(2000); //2ms

    for (uint8_t i = 0; i < SLAVE_BOOT_TRIGGER_CNT; i++)
    {
        ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, boot_loader_trigeer_cmd, sizeof(boot_loader_trigeer_cmd));
        CLK_SysTickDelay(SLAVE_BOOT_TRIGGER_DELAY);

        if (true == ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel))
        {
            FTD_LOG_DEBUG(" chn tx ");
            break;
        }
    }

    //CLK_SysTickDelay(10000); //10ms
}

//////////////////////////////////// for debug
static uint8_t ftd_mw_slave_manager_get_crc(uint8_t* buff, uint32_t len)
{
    uint8_t checksum = 0;
    uint32_t i;

    // Calculate XOR checksum over the data
    for (i = 0; i < len; i++)
    {
        checksum ^= buff[i];
    }

    return checksum;
}

uint8_t ftd_mw_slave_check_rx(uint8_t* rx_buffer, uint16_t len)
{
    uint16_t index = len;
    uint8_t  ret = 0x00;

    // the min length of cmd
    while (index > 1)
    {
        // check header: 0x4A 0x59 0x4D 0x43 
        if ((rx_buffer[0] == 0x4A) && (rx_buffer[1] == 0x59) && (rx_buffer[2] == 0x4D) && (rx_buffer[3] == 0x43))
        {
            // Check if buffer has enough data for the minimum command length
            if (index < 11)
            {
                index--;
                memcpy(&rx_buffer[0], &rx_buffer[1], index);  //drop one byte
                continue;
            }

            ret = 0xFF;

            // in bootloader mode?
            if ((rx_buffer[4] == 0xD4) && (rx_buffer[5] == 0x00) && (rx_buffer[6] == 0x03) && (rx_buffer[7] == 0x00)
                && (rx_buffer[8] == 0x47) && (rx_buffer[9] == 0x01) && (rx_buffer[10] == 0x8C)
                )
            {
                FTD_LOG_TRACE("SLAVE TRIGGER IN BOOTLOADER");
                ret = SLAVE_CMD_ENTER_BOOT_LOADER;
            }
            else if ((rx_buffer[4] == 0xD4) && (rx_buffer[5] == 0x00) && (rx_buffer[6] == 0x03) && (rx_buffer[7] == 0x00)
                && (rx_buffer[8] == 0x47) && (rx_buffer[9] == 0x02) && (rx_buffer[10] == 0x8F)
                )
            {
                FTD_LOG_TRACE("SLAVE TRIGGER IN BOOTLOADER");
                ret = SLAVE_CMD_ENTER_BOOT_LOADER;
            }
#if 0
            // reset flash??
            if ((rx_buffer[4] == 0xD4) && (rx_buffer[5] == 0x00) && (rx_buffer[6] == 0x03) && (rx_buffer[7] == 0x00)
                && (rx_buffer[8] == 0x47) && (rx_buffer[9] == 0x01) && (rx_buffer[10] == 0x8C)
                )
            {
                FTD_LOG_TRACE("SLAVE TRIGGER IN BOOTLOADER");
                FTD_LOG_DEBUG_BUFF(rx_buffer, sizeof(SLAVE_CHN1_FUFF), 11);
                ret = SLAVE_CMD_ENTER_BOOT_LOADER;
            }
#endif
            // FLASH status
            else if ((rx_buffer[4] == 0xD4) && (rx_buffer[5] == 0x00) && (rx_buffer[6] == 0x04)
                && (rx_buffer[7] == 0x00) && (rx_buffer[8] == 0x4A)
                )
            {
                uint8_t calc_crc = ftd_mw_slave_manager_get_crc(rx_buffer, SLAVE_FLASH_STATUS_ACK_LEN - CHECKSUM_SIZE);

                if (calc_crc == rx_buffer[SLAVE_FLASH_STATUS_ACK_LEN - CHECKSUM_SIZE])
                {
                    FTD_LOG_TRACE("FLASH STATUS OK");
                    ret = SLAVE_CMD_SET_FLASH_STATUS_REG;
                }
                else
                {
                    FTD_LOG_ERROR("calc_crc = %02x, buff_crc = %02x", calc_crc, rx_buffer[SLAVE_FLASH_ERASE_ACK_SIZE - CHECKSUM_SIZE]);
                    FTD_LOG_DEBUG_BUFF(rx_buffer, sizeof(SLAVE_CHN1_FUFF), 19);
                }
            }
            // FLASH erase
            else if ((rx_buffer[4] == 0xD4) && (rx_buffer[5] == 0x00) && (rx_buffer[6] == 0x0A)
                && (rx_buffer[7] == 0x00) && (rx_buffer[8] == 0x4F)
                )
            {
                uint8_t calc_crc = ftd_mw_slave_manager_get_crc(rx_buffer, SLAVE_FLASH_ERASE_ACK_SIZE - CHECKSUM_SIZE);

                if (calc_crc == rx_buffer[SLAVE_FLASH_ERASE_ACK_SIZE - CHECKSUM_SIZE])
                {
                    FTD_LOG_DEBUG("ERASE OK");
                    ret = SLAVE_CMD_ERASE_FLASH_SECTOR;
                }
                else
                {
                    FTD_LOG_ERROR("calc_crc = %02x, buff_crc = %02x", calc_crc, rx_buffer[SLAVE_FLASH_ERASE_ACK_SIZE - CHECKSUM_SIZE]);
                    FTD_LOG_DEBUG_BUFF(rx_buffer, sizeof(SLAVE_CHN1_FUFF), 19);
                }
            }
            // FLASH program
            else if ((rx_buffer[4] == 0xD4) && (rx_buffer[5] == 0x00) && (rx_buffer[6] == 0x06)
                && (rx_buffer[7] == 0x00) && (rx_buffer[8] == 0x51)
                )
            {
                uint8_t calc_crc = ftd_mw_slave_manager_get_crc(rx_buffer, SLAVE_FLASH_PROG_ACK_LEN - CHECKSUM_SIZE);

                if (calc_crc == rx_buffer[SLAVE_FLASH_PROG_ACK_LEN - CHECKSUM_SIZE])
                {
                    FTD_LOG_TRACE("PROG OK");
                    ret = SLAVE_CMD_WRITE_FLASH_DATA;
                }
                else
                {
                    FTD_LOG_ERROR("calc_crc = %02x, buff_crc = %02x", calc_crc, rx_buffer[SLAVE_FLASH_PROG_ACK_LEN - CHECKSUM_SIZE]);
                    FTD_LOG_DEBUG_BUFF(rx_buffer, sizeof(SLAVE_CHN1_FUFF), SLAVE_FLASH_PROG_ACK_LEN);
                }
            }
            // rom program
            else if ((rx_buffer[4] == 0xD4) && (rx_buffer[5] == 0x00) && (rx_buffer[6] == 0x06)
                && (rx_buffer[7] == 0x00) && (rx_buffer[8] == 0x52)
                )
            {
                uint8_t calc_crc = ftd_mw_slave_manager_get_crc(rx_buffer, SLAVE_FLASH_PROG_ACK_LEN - CHECKSUM_SIZE);

                if (calc_crc == rx_buffer[SLAVE_FLASH_PROG_ACK_LEN - CHECKSUM_SIZE])
                {
                    FTD_LOG_TRACE("PROG OK");
                    ret = SLAVE_CMD_WRITE_RAM_DATA;
                }
                else
                {
                    FTD_LOG_ERROR("calc_crc = %02x, buff_crc = %02x", calc_crc, rx_buffer[SLAVE_FLASH_PROG_ACK_LEN - CHECKSUM_SIZE]);
                    FTD_LOG_DEBUG_BUFF(rx_buffer, sizeof(SLAVE_CHN1_FUFF), SLAVE_FLASH_PROG_ACK_LEN);
                }
            }
            // rom program jump 
            else if ((rx_buffer[4] == 0xD4) && (rx_buffer[5] == 0x00) && (rx_buffer[6] == 0x06)
                && (rx_buffer[7] == 0x00) && (rx_buffer[8] == 0x53)
                )
            {
                uint8_t calc_crc = ftd_mw_slave_manager_get_crc(rx_buffer, SLAVE_FLASH_PROG_ACK_LEN - CHECKSUM_SIZE);

                if (calc_crc == rx_buffer[SLAVE_FLASH_PROG_ACK_LEN - CHECKSUM_SIZE])
                {
                    FTD_LOG_TRACE("PROG OK");
                    ret = SLAVE_CMD_SET_PROGRAM_JUMP;
                }
                else
                {
                    FTD_LOG_ERROR("calc_crc = %02x, buff_crc = %02x", calc_crc, rx_buffer[SLAVE_FLASH_PROG_ACK_LEN - CHECKSUM_SIZE]);
                    FTD_LOG_DEBUG_BUFF(rx_buffer, sizeof(SLAVE_CHN1_FUFF), SLAVE_FLASH_PROG_ACK_LEN);
                }
            }
            // FLASH read
            else if ((rx_buffer[4] == 0xD4) && (rx_buffer[7] == 0x00) && (rx_buffer[8] == 0x50)
                )
            {
                uint16_t read_len = (rx_buffer[5] << 8) + rx_buffer[6] - EXTEND_CMD_SIZE - SLAVE_FLASH_OP_ADDR_SIZE - SLAVE_FLASH_READ_LENGTH_SIZE;
                uint8_t  calc_crc = ftd_mw_slave_manager_get_crc(rx_buffer, (SLAVE_FLASH_READ_CMD_SIZE_MAX - CHECKSUM_SIZE + read_len));

                if (calc_crc == rx_buffer[SLAVE_FLASH_READ_CMD_SIZE_MAX - CHECKSUM_SIZE + read_len])
                {
                    FTD_LOG_TRACE("READ OK");
                    ret = SLAVE_CMD_READ_CODE_CRC;
                }
                else
                {
                    FTD_LOG_ERROR("calc_crc = %02x, buff_crc = %02x", calc_crc, rx_buffer[SLAVE_FLASH_READ_CMD_SIZE_MAX - CHECKSUM_SIZE + read_len]);
                }
            }
            else if ((rx_buffer[4] == 0xD4 && rx_buffer[5] == 0x00 && rx_buffer[6] == 0x06)
                && (rx_buffer[7] == 0x00) && (rx_buffer[8] == 0x4E))
            {
                uint8_t calc_crc = ftd_mw_slave_manager_get_crc(rx_buffer, 14 - CHECKSUM_SIZE);
                if (calc_crc == rx_buffer[14 - CHECKSUM_SIZE])
                {
                    FTD_LOG_TRACE("SLAVE_CMD_MODIFY_BAUD_RATE OK");
                    ret = SLAVE_CMD_MODIFY_BAUD_RATE;
                }
                else
                {
                    FTD_LOG_ERROR("calc_crc = %02x, buff_crc = %02x", calc_crc, rx_buffer[14 - CHECKSUM_SIZE]);
                    FTD_LOG_DEBUG_BUFF(rx_buffer, sizeof(SLAVE_CHN1_FUFF), 14);
                }
            }

            // todo: add abnormal process
            if (0xFF == ret)
            {
                index -= 4;
                memcpy(&rx_buffer[0], &rx_buffer[4], index);
                ret = 0;
            }
        }
        // Check for new 2-byte protocol: header 0x5A followed by a parameter
        else if (rx_buffer[0] == 0x5A)
        {
            // Check if buffer has enough data for 2-byte protocol
            if (index >= 2)
            {
                uint8_t param = rx_buffer[1];
                FTD_LOG_TRACE("NEW PROTOCOL: header 0x5A, param 0x%02x", param);

                // Map parameter to command code according to the new protocol
                switch (param)
                {
                    case 0x11:
                        ret = SLAVE_CMD_TEST_SLEEP_CURRENT;
                        break;
                    case 0x12:
                        ret = SLAVE_CMD_TEST_SLEEP_CURRENT;
                        break;
                    case 0x17:
                        ret = SLAVE_CMD_SEND_PACKET_ADDR;
                        break;
                    case 0x2A:
                        ret = SLAVE_CMD_TEST_CURRENT_AT_0DB;
                        break;
                    case 0x2B:
                        ret = SLAVE_CMD_TEST_CURRENT_AT_5DB;
                        break;
                    case 0x2C:
                        ret = SLAVE_CMD_TEST_CURRENT_AT_STATIC;
                        break;
                    case 0x2D:
                        ret = SLAVE_CMD_TEST_RECEIVE_SENSITIVITY;
                        break;
                    case 0x2E:
                        ret = SLAVE_CMD_CALIBRATE_MADC;
                        break;
                    case 0x2F:
                        ret = SLAVE_CMD_CALIBRATE_GPADC;
                        break;
                    case 0x30:
                        ret = SLAVE_CMD_CALIBRATE_CLOCK;
                        break;
                    case 0x31:
                        ret = SLAVE_CMD_CALIBRATE_MADC_VALUE;
                        break;
                    case 0x32:
                        ret = SLAVE_CMD_CALIBRATE_GPADC_VALUE;
                        break;
                    case 0x33:
                        ret = SLAVE_CMD_TEST_RECEIVE_SENSITIVITY_VALUE;
                        break;
                    case 0x34:
                        ret = SLAVE_CMD_CALIBRATE_CLOCK_VALUE;
                        break;
                    case 0x44:
                        ret = SLAVE_CMD_FT_WRITE_CHIP_ID;
                        break;
                    case 0x55:
                        ret = SLAVE_CMD_WAIT_CALIBRATION;
                        break;
                    case 0x66:
                        ret = SLAVE_CMD_JUMP_BOOTLOADER;
                        break;
                    default:
                        FTD_LOG_ERROR("Unknown parameter 0x%02x in new protocol", param);
                        ret = 0xFF;
                        break;
                }
            }
            else
            {
                // Not enough data, wait for more
                break;
            }
        }
        else
        {
            index--;
            //FTD_LOG_TRACE("drop 0x%x", rx_buffer[0]);
            memcpy(&rx_buffer[0], &rx_buffer[1], index);  //drop one byte
        }

        if (ret)
        {
            if (0xFF == ret)
            {
                FTD_LOG_DEBUG("____UNROCOGNIZED_CMD____[%d]", index);
                FTD_LOG_DEBUG_BUFF(rx_buffer, sizeof(SLAVE_CHN1_FUFF), index);
            }
            break;
        }
    }

    return ret;
}

extern void ftd_drv_burn_channel_start(uint8_t* tx_buf, uint16_t tx_len, uint8_t* rx_buf, uint16_t rx_len);

bool ftd_mw_slave_manager_start_ld_test(SLAVE_INFO* p_st_slave_info)
{
    bool    ret = false;
    // A series (FL100): 0x55, 0x96 pattern
    uint8_t boot_loader_cmd_fl100[] = { 0x55, 0x96, 0x55, 0x96, 0x55, 0x96 };
    // B series (FL100B): 0x40, 0x40 pattern
    uint8_t boot_loader_cmd_fl100b[] = { 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40 };
    uint8_t wait_time = 0;

    // Get chip family to select boot loader trigger command
    uint16_t chip_code = (uint16_t)p_st_slave_info->chip_id;
    const CHIP_FLASH_ADDR_MAP* p_chip_map = ftd_mw_slave_get_chip_flash_map(chip_code);

    FTD_LOG_INFO("CH%d: Map Chip: %s (0x%04X), Family: %d", p_st_slave_info->slave_uart_channel, p_chip_map->chip_name, p_chip_map->chip_code, p_chip_map->chip_family);

    // Select command based on chip family
    uint8_t* boot_loader_trigeer_cmd;
    uint8_t  boot_loader_cmd_len;
    if (p_chip_map->chip_family == CHIP_FAMILY_FL100B)
    {
        boot_loader_trigeer_cmd = boot_loader_cmd_fl100b;
        boot_loader_cmd_len = sizeof(boot_loader_cmd_fl100b);
        FTD_LOG_DEBUG("Using FL100B boot trigger (0x40)");
    }
    else
    {
        boot_loader_trigeer_cmd = boot_loader_cmd_fl100;
        boot_loader_cmd_len = sizeof(boot_loader_cmd_fl100);
        FTD_LOG_DEBUG("Using FL100 boot trigger (0x55,0x96)");
    }

    for (uint8_t i = 0; i < SLAVE_BOOT_TRIGGER_CNT; i++)
    {
#if 0
        // 1.power off slave
        ftd_mw_slave_manager_power_off(p_st_slave_info->slave_uart_channel);
        FTD_LOG_TRACE("power off end");

        // delay abt 1.2s to waitting target power off
        //for (uint16_t i = 0; i < 50; i++)
        CLK_SysTickLongDelay(8000000);
#endif
        // 2.power on slave, send bootloader trigger cmd five times(boot_loader_trigeer_cmd)
        ftd_mw_slave_manager_power_on(p_st_slave_info->slave_uart_channel);
        FTD_LOG_TRACE("power on end");
        CLK_SysTickLongDelay(50000); // power on waiting 50ms for slave chip execute reset process

        FTD_LOG_TRACE("tx start");
        memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));

        ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, 11);

        for (uint8_t cnt = 0; cnt < 5; cnt++)
        {
            ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, boot_loader_trigeer_cmd, boot_loader_cmd_len);

            //while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
            //FTD_LOG_TRACE("rx start");
            wait_time = 3;
            while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
            {
                CLK_SysTickLongDelay(1000);
            }

            //wait_time = 3;
            //while (wait_time-- > 0)
            //{
            //    CLK_SysTickLongDelay(1000);
            //}

            FTD_LOG_TRACE("rx end, wait_time:%d ", wait_time);
            FTD_LOG_TRACE_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));
            uint8_t slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));

            if (SLAVE_CMD_ENTER_BOOT_LOADER == slave_ack)
            {
                //FTD_LOG_DEBUG("ch:%d check bootloader ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
                FTD_LOG_DEBUG("ch:%d check bootloader ack [0x%02x] cnt:%d wait_time:%d ",
                    p_st_slave_info->slave_uart_channel, slave_ack, cnt, wait_time);
                ret = true;
                break;
            }
        }

        if (false == ret)
        {
            wait_time = 15;
            while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
            {
                CLK_SysTickLongDelay(1000);
            }

            FTD_LOG_TRACE("rx end, wait_time:%d ", wait_time);
            FTD_LOG_TRACE_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));
            uint8_t slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));

            if (SLAVE_CMD_ENTER_BOOT_LOADER == slave_ack)
            {
                FTD_LOG_DEBUG("ch:%d check bootloader ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
                ret = true;
                break;
            }
            else
            {
                FTD_LOG_ERROR("ch:%d slave check bootloader unack", p_st_slave_info->slave_uart_channel);
                ret = false;
            }
        }
        else
        {
            break;
        }

    }

    return ret;
}

bool ftd_mw_slave_manager_modify_baud_rate(SLAVE_INFO* p_st_slave_info, CHANNEL_BURN_STATUS* st_channel_burn_status)
{
    bool    ret = false;
    uint16_t wait_time = 0;
    // Use 921600bps baud rate command
    uint8_t send_cmd_buff[14] =
    { 0x4a, 0x59, 0x4d, 0x43,
        0x54,
        0x00, 0x06,
        0x00, 0x4E,
        0x00, 0x07,
        0x08, 0x00,
        0xFF }; //uart protocol 460800bps
    // uint8_t send_cmd_buff[14] =
    // { 0x4a, 0x59, 0x4d, 0x43,
    //     0x54,
    //     0x00, 0x06,
    //     0x00, 0x4E,
    //     0x00, 0x0E,
    //     0x10, 0x00,
    //     0xFF }; //uart protocol 921600bps

    SYS_INFO sys_info;

    // Get chip flash map by chip_code
    ftd_mw_sys_info_manager_get(&sys_info);
    uint16_t chip_code = (uint16_t)sys_info.st_fw_info[st_channel_burn_status->fw_num].chip_id;
    const CHIP_FLASH_ADDR_MAP* p_chip_map = ftd_mw_slave_get_chip_flash_map(chip_code);

    // For B version chip, return true directly
    if (p_chip_map->chip_family == CHIP_FAMILY_FL100B)
    {
        FTD_LOG_DEBUG("CH%d: B version chip, no need to modify baud rate", p_st_slave_info->slave_uart_channel);
        return true;
    }

    uint32_t start_time = ftd_mw_misc_manager_get_time_ms();

    send_cmd_buff[sizeof(send_cmd_buff) - CHECKSUM_SIZE] = ftd_mw_slave_manager_get_crc(send_cmd_buff, (sizeof(send_cmd_buff) - CHECKSUM_SIZE));

    FTD_LOG_DEBUG_BUFF(send_cmd_buff, sizeof(send_cmd_buff), sizeof(send_cmd_buff));


    FTD_LOG_TRACE("tx start");
    memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
    ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, sizeof(send_cmd_buff));
    ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_cmd_buff, sizeof(send_cmd_buff));
    while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
    FTD_LOG_TRACE("rx start");

    wait_time = 30000;
    while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }

    FTD_LOG_TRACE("rx end, wait_time:%d ", wait_time);

    FTD_LOG_TRACE_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));

    uint8_t slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));

    if (SLAVE_CMD_MODIFY_BAUD_RATE == slave_ack)
    {
        FTD_LOG_DEBUG("ch:%d check modify baud rate ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
        ret = true;

        // For A version chip, switch to 460800 baudrate after bootloader enters
        //ftd_drv_burn_uart_single_deinit(p_st_slave_info->slave_uart_channel);
        ftd_drv_burn_uart_single_init(p_st_slave_info->slave_uart_channel, TRANSFER_BAUDRATE);
        FTD_LOG_DEBUG("CH%d: Switched to 460800 baudrate for A version chip", p_st_slave_info->slave_uart_channel);
    }
    else
    {
        FTD_LOG_WARN("ch:%d slave modify baud rate unack", p_st_slave_info->slave_uart_channel);
        ret = false;
    }

    // Ensure at least 100ms from function start to end
    uint32_t elapsed_time = ftd_mw_misc_manager_get_time_ms() - start_time;
    if (elapsed_time < SLAVE_SEND_PACKET_DELAY_MS) {
        uint32_t delay_time = SLAVE_SEND_PACKET_DELAY_MS - elapsed_time;
        FTD_LOG_TRACE("Adding delay %dms to reach %dms total", delay_time, SLAVE_SEND_PACKET_DELAY_MS);
        for (uint32_t i = 0; i < delay_time; i++) {
            CLK_SysTickLongDelay(1000); // 1ms delay
        }
    }
    FTD_LOG_DEBUG("Total time from function start to end: %dms", ftd_mw_misc_manager_get_time_ms() - start_time);

    return ret;
}

bool ftd_mw_slave_manager_disable_wp(SLAVE_INFO* p_st_slave_info)
{
    bool    ret = false;
    uint8_t wait_time = 0;
    uint8_t send_cmd_buff[12] =
    { 0x4a, 0x59, 0x4d, 0x43,
        0x54,
        0x00, 0x04,
        0x00, 0x4A,
        0x00, 0x00,
        0xFF }; //uart protocol

    send_cmd_buff[sizeof(send_cmd_buff) - CHECKSUM_SIZE] = ftd_mw_slave_manager_get_crc(send_cmd_buff, (sizeof(send_cmd_buff) - CHECKSUM_SIZE));

    FTD_LOG_DEBUG_BUFF(send_cmd_buff, sizeof(send_cmd_buff), sizeof(send_cmd_buff));

    for (int i = 0; i < SLAVE_DISABLE_WP_TRIGGER_CNT; i++)
    {
        FTD_LOG_TRACE("tx start");
        memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
        ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, SLAVE_FLASH_STATUS_ACK_LEN);
        ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_cmd_buff, sizeof(send_cmd_buff));
        while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
        FTD_LOG_TRACE("rx start");

        wait_time = 300;
        while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
        {
            CLK_SysTickLongDelay(1000);
        }

        FTD_LOG_TRACE("rx end, wait_time:%d ", wait_time);

        FTD_LOG_DEBUG_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), SLAVE_FLASH_STATUS_ACK_LEN);

        uint8_t slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));

        if (SLAVE_CMD_SET_FLASH_STATUS_REG == slave_ack && (send_cmd_buff[10] == SLAVE_CHN1_FUFF[10]))
        {
            FTD_LOG_DEBUG("ch:%d check flash status ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
            ret = true;
            break;
        }
        else
        {
            FTD_LOG_WARN("ch:%d slave flash status unack", p_st_slave_info->slave_uart_channel);
            ret = false;
        }
    }


    return ret;
}

bool ftd_mw_slave_manager_erase_test(SLAVE_INFO* p_st_slave_info)
{
    bool    ret = false;
    uint32_t fw_size;
    uint32_t slv_addr;
    uint8_t wait_time;
    uint8_t boot_loader_trigeer_cmd[18] =
    { 0x4a, 0x59, 0x4d, 0x43,
        0x54,
        0x00, 0x0a,
        0x00, 0x4f,
        0x00, 0x00, 0x00, 0x00, // addr
        0x00, 0x00, 0x00, 0x00, // size
        0xFF }; // crc

    // Get chip flash map by chip_code
    uint16_t chip_code = (uint16_t)p_st_slave_info->chip_id;
    const CHIP_FLASH_ADDR_MAP* p_chip_map = ftd_mw_slave_get_chip_flash_map(chip_code);

    // set slave flash start addr
    if (0x00 == burn_test_addr)
    {
        slv_addr = p_chip_map->fw_bin_a_addr;
    }
    else
    {
        slv_addr = p_chip_map->fw_bin_b_addr + 0x1000; // Original SLAVE_FLASH_START_ADDR (0x08021000) was 0x08020000 + 0x1000
    }

    FTD_LOG_DEBUG("CH%d: Erase Test Map - Chip: %s, Erase Addr: 0x%08X", p_st_slave_info->slave_uart_channel, p_chip_map->chip_name, slv_addr);

    boot_loader_trigeer_cmd[9] = (slv_addr >> 24) & 0xFF;
    boot_loader_trigeer_cmd[10] = (slv_addr >> 16) & 0xFF;
    boot_loader_trigeer_cmd[11] = (slv_addr >> 8) & 0xFF;
    boot_loader_trigeer_cmd[12] = slv_addr & 0xFF;

    // set slave flash erase size
    fw_size = p_st_slave_info->flash_bin_size;
    boot_loader_trigeer_cmd[13] = (fw_size >> 24) & 0xFF;
    boot_loader_trigeer_cmd[14] = (fw_size >> 16) & 0xFF;
    boot_loader_trigeer_cmd[15] = (fw_size >> 8) & 0xFF;
    boot_loader_trigeer_cmd[16] = fw_size & 0xFF;
    FTD_LOG_TRACE("fw_size = %d, 0x%x, 0x%x, 0x%x, 0x%x", fw_size, boot_loader_trigeer_cmd[13], boot_loader_trigeer_cmd[14], boot_loader_trigeer_cmd[15], boot_loader_trigeer_cmd[16]);

    if (0 == fw_size)
    {
        FTD_LOG_INFO("send slave erase cmd fail, fw_size = %x", fw_size);
        return ret;
    }

    // add crc 
    boot_loader_trigeer_cmd[sizeof(boot_loader_trigeer_cmd) - CHECKSUM_SIZE] = ftd_mw_slave_manager_get_crc(boot_loader_trigeer_cmd, (sizeof(boot_loader_trigeer_cmd) - CHECKSUM_SIZE));
    FTD_LOG_TRACE_BUFF(boot_loader_trigeer_cmd, sizeof(boot_loader_trigeer_cmd), sizeof(boot_loader_trigeer_cmd));

    memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
    ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, boot_loader_trigeer_cmd, sizeof(boot_loader_trigeer_cmd));
    FTD_LOG_TRACE(" TX  ");
    while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));

    ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, 35); // 19 35
    FTD_LOG_TRACE(" TX D ");
    CLK_SysTickLongDelay(200000);

    FTD_LOG_TRACE(" WHILE RX ");
    wait_time = 200;
    while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }
    ftd_drv_burn_uart_rx_stop(p_st_slave_info->slave_uart_channel);
    FTD_LOG_TRACE(" WHILE RX DONE ");

    uint8_t ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));
    if (SLAVE_CMD_ERASE_FLASH_SECTOR == ack)
    {
        FTD_LOG_DEBUG("ch:%d slave erase ack [0x%02x]", p_st_slave_info->slave_uart_channel, ack);
        ret = true;

        // abt 20ms, wait for slave erase done
        CLK_SysTickLongDelay(200000);
    }
    else
    {
        FTD_LOG_WARN("ch:%d slave erase unack", p_st_slave_info->slave_uart_channel);
        FTD_LOG_DEBUG_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));
    }

    ftd_drv_burn_uart_rx_stop(p_st_slave_info->slave_uart_channel);

    return ret;
}

bool ftd_mw_slave_manager_program_test(SLAVE_INFO* p_st_slave_info, CHANNEL_BURN_STATUS* st_channel_burn_status)
{
    bool     ret = false;
    uint8_t  read_partition;
    uint32_t fw_read_length;
    uint32_t slv_addr;
    uint32_t fw_size;
    uint8_t  fw_buff[SLAVE_FLASH_PROG_SIZE];
    static uint8_t uart_buff[SLAVE_FLASH_PROG_CMD_SIZE_MAX] = { 0x4a, 0x59, 0x4d, 0x43, 0x54 };
    uint16_t uart_cmd_ttl_len = 0;
    uint16_t wait_time;
    SYS_INFO sys_info;

    // Get chip flash map by chip_code
    ftd_mw_sys_info_manager_get(&sys_info);
    uint16_t chip_code = (uint16_t)sys_info.st_fw_info[st_channel_burn_status->fw_num].chip_id;
    const CHIP_FLASH_ADDR_MAP* p_chip_map = ftd_mw_slave_get_chip_flash_map(chip_code);

    fw_size = p_st_slave_info->flash_bin_size;
    slv_addr = p_chip_map->fw_bin_a_addr;  // Use mapped address instead of hardcoded macro

    FTD_LOG_TRACE("fw_size:0x%08x last_pkt_valid_data_len:0x%08x", fw_size, (fw_size % SLAVE_FLASH_PROG_SIZE));

    read_partition = PARTITION_FW1_BIN + st_channel_burn_status->fw_num * (PARTITION_FW2_BIN - PARTITION_FW1_BIN);

    for (fw_read_length = 0; fw_read_length < fw_size; fw_read_length += SLAVE_FLASH_PROG_SIZE)
    {
        FTD_LOG_TRACE("read addr:0x%08x len:0x%08x slv_addr:0x%08x ", PARTITION_FW1_BIN, fw_read_length, slv_addr);
        if (ftd_drv_flash_operation((PARTITION_NAME)read_partition, OPERATION_READ, fw_read_length, SLAVE_FLASH_PROG_SIZE, fw_buff) == 0)
        {
            // send uart pkt
            // ext cmd
            uart_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET] = 0x00;
            uart_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET + 1] = 0x51;
            // slave flash addr
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET] = (slv_addr >> 24) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 1] = (slv_addr >> 16) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 2] = (slv_addr >> 8) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 3] = slv_addr & 0xFF;
            // data length (fixed 0x00 for alain)
            uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET] = 0x00;
            uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 1] = 0x00;
            uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 2] = 0x00;

            if (fw_read_length + SLAVE_FLASH_PROG_SIZE > fw_size)
            {
                // last page
                //FTD_LOG_TRACE_BUFF(fw_buff, sizeof(fw_buff), (fw_size % SLAVE_FLASH_PROG_SIZE));

                uint32_t data_len = fw_size % SLAVE_FLASH_PROG_SIZE;
                uint16_t parm_len = EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_OP_LENGTH_SIZE + data_len; // 4 + 4 + 2 + len

                // calc pram len
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET] = (parm_len >> 8) & 0xFF;
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] = parm_len & 0xFF;

                // copy bin
                memcpy(&uart_buff[SLAVE_FLASH_PROG_DATA_OFFSET], fw_buff, data_len);

                uart_cmd_ttl_len = SLAVE_FLASH_PROG_DATA_OFFSET + data_len + CHECKSUM_SIZE;
            }
            else
            {
                // calc pram len
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET] = (SLAVE_FLASH_PROG_PRAM_LEN_MAX >> 8) & 0xFF;
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] = SLAVE_FLASH_PROG_PRAM_LEN_MAX & 0xFF;

                // copy bin
                memcpy(&uart_buff[SLAVE_FLASH_PROG_DATA_OFFSET], fw_buff, SLAVE_FLASH_PROG_SIZE);

                uart_cmd_ttl_len = SLAVE_FLASH_PROG_CMD_SIZE_MAX;
            }

            // add crc 
            uart_buff[uart_cmd_ttl_len - CHECKSUM_SIZE] = ftd_mw_slave_manager_get_crc(uart_buff, (uart_cmd_ttl_len - CHECKSUM_SIZE));

            // for debug only
            // FTD_LOG_DEBUG(" PROG SEND[%d]:", uart_cmd_ttl_len);
            // FTD_LOG_DEBUG_BUFF(uart_buff, uart_cmd_ttl_len, uart_cmd_ttl_len);

            // send uart pkt to slave
            // [Bugfix] The compiler has assigned duplicate addresses to array SLAVE_CHN1_FUFF and uart_buff
            // so need use static array to avoid this issue
            memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
            ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, SLAVE_FLASH_PROG_ACK_LEN);
            ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, uart_buff, uart_cmd_ttl_len);
            FTD_LOG_TRACE("_TX_");
            while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
            FTD_LOG_TRACE("__D__");

            wait_time = 100;
            while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
            {
                CLK_SysTickLongDelay(1000); // 1ms
            }
            ftd_drv_burn_uart_rx_stop(p_st_slave_info->slave_uart_channel);

            //FTD_LOG_DEBUG_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));
            uint8_t ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));
            if (SLAVE_CMD_WRITE_FLASH_DATA == ack)
            {
                FTD_LOG_TRACE("ch:%d slave prog ack [0x%02x] cmd_len[%d] buf_len[%d]", p_st_slave_info->slave_uart_channel,
                    ack, uart_cmd_ttl_len, SLAVE_FLASH_PROG_CMD_SIZE_MAX);
                ret = true;
            }
            else
            {
                FTD_LOG_ERROR("ch:%d slave prog unack", p_st_slave_info->slave_uart_channel);
                ret = false;
                break;
            }
        }
        else
        {
            FTD_LOG_ERROR("ch:%d read fw fail", p_st_slave_info->slave_uart_channel);
            ret = false;
            break;
        }

        slv_addr += SLAVE_FLASH_PROG_SIZE;
    }

    return ret;
}


bool ftd_mw_slave_manager_program_jump(SLAVE_INFO* p_st_slave_info, uint32_t jump_addr)
{
    bool    ret = false;
    uint16_t wait_time = 0;
    uint8_t send_cmd_buff[] =
    { 0x4a, 0x59, 0x4d, 0x43,
        0x54,
        0x00, 0x06,
        0x00, 0x53,
        0x00, 0x00,
        0x00, 0x00,
        0xFF }; //uart protocol

    FTD_LOG_DEBUG("ch:%d program jump to address: 0x%08x", p_st_slave_info->slave_uart_channel, jump_addr);

    send_cmd_buff[SLAVE_FLASH_PROG_ADDR_OFFSET] = (jump_addr >> 24) & 0xFF;
    send_cmd_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 1] = (jump_addr >> 16) & 0xFF;
    send_cmd_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 2] = (jump_addr >> 8) & 0xFF;
    send_cmd_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 3] = jump_addr & 0xFF;

    send_cmd_buff[sizeof(send_cmd_buff) - CHECKSUM_SIZE] = ftd_mw_slave_manager_get_crc(send_cmd_buff, (sizeof(send_cmd_buff) - CHECKSUM_SIZE));

    FTD_LOG_DEBUG_BUFF(send_cmd_buff, sizeof(send_cmd_buff), sizeof(send_cmd_buff));

    FTD_LOG_TRACE("tx start");
    memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
    ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, sizeof(send_cmd_buff));
    ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_cmd_buff, sizeof(send_cmd_buff));
    while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
    FTD_LOG_TRACE("rx start");

    wait_time = 30;
    while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }

    FTD_LOG_TRACE("rx end, wait_time:%d ", wait_time);
    FTD_LOG_TRACE_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));
    uint8_t slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));

    if (SLAVE_CMD_SET_PROGRAM_JUMP == slave_ack)
    {
        FTD_LOG_DEBUG("ch:%d check slave program jump ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
        ret = true;
    }
    else
    {
        FTD_LOG_DEBUG("ch:%d slave program jump unack", p_st_slave_info->slave_uart_channel);
        ret = false;
    }
    //ftd_drv_burn_uart_single_deinit(p_st_slave_info->slave_uart_channel);
    ftd_drv_burn_uart_single_init(p_st_slave_info->slave_uart_channel, DEFAULT_BAUDRATE);

    if (ret == true)
    {
        FTD_LOG_TRACE("tx start");
        memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
        ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, 2);
        wait_time = 5000;
        while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
        {
            CLK_SysTickLongDelay(1000);
        }
        uint8_t slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));
        if (SLAVE_CMD_WAIT_CALIBRATION == slave_ack)
        {
            FTD_LOG_DEBUG("ch:%d check slave wait calibration ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
            ret = true;
        }
        else
        {
            FTD_LOG_DEBUG("ch:%d slave wait calibration unack failed", p_st_slave_info->slave_uart_channel);
            ret = false;
        }
    }

    return ret;
}

bool ftd_mw_slave_manager_enable_wp(SLAVE_INFO* p_st_slave_info)
{
    bool    ret = false;
    uint8_t wait_time = 0;
    uint8_t send_cmd_buff[12] =
    { 0x4a, 0x59, 0x4d, 0x43,
        0x54,
        0x00, 0x04,
        0x00, 0x4A,
        0x00, 0x28,
        0xFF }; //uart protocol

    send_cmd_buff[sizeof(send_cmd_buff) - CHECKSUM_SIZE] = ftd_mw_slave_manager_get_crc(send_cmd_buff, (sizeof(send_cmd_buff) - CHECKSUM_SIZE));

    FTD_LOG_TRACE_BUFF(send_cmd_buff, sizeof(send_cmd_buff), sizeof(send_cmd_buff));

    FTD_LOG_TRACE("tx start");
    memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
    ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, SLAVE_FLASH_STATUS_ACK_LEN);
    ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_cmd_buff, sizeof(send_cmd_buff));
    while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
    FTD_LOG_TRACE("rx start");

    wait_time = 30;
    while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }

    FTD_LOG_TRACE("rx end, wait_time:%d ", wait_time);

    FTD_LOG_TRACE_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));

    uint8_t slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));

    if (SLAVE_CMD_SET_FLASH_STATUS_REG == slave_ack && (send_cmd_buff[10] == SLAVE_CHN1_FUFF[10]))
    {
        FTD_LOG_DEBUG("ch:%d check flash status ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
        ret = true;
    }
    else
    {
        FTD_LOG_DEBUG("ch:%d slave flash status unack", p_st_slave_info->slave_uart_channel);
        ret = false;
    }

    return ret;
}

bool ftd_mw_slave_manager_read_crc_test(SLAVE_INFO* p_st_slave_info)
{
    bool     ret = false;
    SYS_INFO st_sys_info;
    uint32_t fw_read_length;
    uint32_t slv_addr;
    uint32_t fw_size;
    uint32_t read_len;
    uint8_t  uart_buff[SLAVE_FLASH_READ_CMD_SIZE_MAX] = { 0x4a, 0x59, 0x4d, 0x43, 0x54 };
    uint16_t uart_cmd_ttl_len = 0;
    uint16_t wait_time;
    uint16_t calc_fw_crc = 0xFFFF; // init crc value is fixed 0xFFFF

    uint8_t READ_BUFF[SLAVE_FLASH_READ_CMD_ACK_SIZE_MAX];
    ftd_mw_sys_info_manager_get(&st_sys_info);

    // Get chip flash map by chip_code (use fw_num=0 as default for test function)
    uint16_t chip_code = (uint16_t)p_st_slave_info->chip_id;
    const CHIP_FLASH_ADDR_MAP* p_chip_map = ftd_mw_slave_get_chip_flash_map(chip_code);

    FTD_LOG_DEBUG("CH%d: CRC Test Map - Chip: %s, Addr A: 0x%08X", p_st_slave_info->slave_uart_channel, p_chip_map->chip_name, p_chip_map->fw_bin_a_addr);

    fw_size = p_st_slave_info->flash_bin_size;
    FTD_LOG_TRACE("fw_size:0x%08x ", fw_size);

    slv_addr = p_chip_map->fw_bin_a_addr;  // Use mapped address instead of hardcoded macro

    FTD_LOG_DEBUG("fw_size:0x%08x last_page_valid_data_len:0x%08x", fw_size, (fw_size % SLAVE_FLASH_READ_SIZE));

    // package read cmd buffer
    // calc pram len
    uint16_t parm_len = EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_READ_LENGTH_SIZE; // 4 + 4 + 2
    uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET] = (parm_len >> 8) & 0xFF;
    uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] = parm_len & 0xFF;
    // ext cmd
    uart_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET] = (SLAVE_EXT_CMD_READ_FLASH >> 8) & 0xFF;//0x00;
    uart_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET + 1] = SLAVE_EXT_CMD_READ_FLASH & 0xFF;//0x50;

    //FTD_LOG_DEBUG("read buffer size:%d single read len:%d rx_len:%d", sizeof(READ_BUFF), SLAVE_FLASH_READ_SIZE, 
    //    (SLAVE_FLASH_READ_CMD_SIZE_MAX + read_len));

    for (fw_read_length = 0; fw_read_length < fw_size; fw_read_length += SLAVE_FLASH_READ_SIZE)
    {
        FTD_LOG_TRACE(" read_slv_addr:0x%08x ", slv_addr);

        // slave flash addr
        uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET] = (slv_addr >> 24) & 0xFF;
        uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 1] = (slv_addr >> 16) & 0xFF;
        uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 2] = (slv_addr >> 8) & 0xFF;
        uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 3] = slv_addr & 0xFF;

        // read length
        if (fw_read_length + SLAVE_FLASH_READ_SIZE > fw_size)
        {
            read_len = fw_size % SLAVE_FLASH_READ_SIZE;
        }
        else
        {
            read_len = SLAVE_FLASH_READ_SIZE;
        }
        // add a addr protection in here
        uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET] = (read_len >> 24) & 0xFF;
        uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 1] = (read_len >> 16) & 0xFF;
        uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 2] = (read_len >> 8) & 0xFF;
        uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 3] = read_len & 0xFF;

        uart_cmd_ttl_len = SLAVE_FLASH_READ_DATA_OFFSET + CHECKSUM_SIZE;

        // add crc 
        uart_buff[uart_cmd_ttl_len - CHECKSUM_SIZE] = ftd_mw_slave_manager_get_crc(uart_buff, (uart_cmd_ttl_len - CHECKSUM_SIZE));

        FTD_LOG_TRACE_BUFF(uart_buff, sizeof(uart_buff), uart_cmd_ttl_len);

        // send uart pkt to slave
        memset(READ_BUFF, 0x00, sizeof(READ_BUFF));
        ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, READ_BUFF, (SLAVE_FLASH_READ_CMD_SIZE_MAX + read_len));
        ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, uart_buff, uart_cmd_ttl_len);
        FTD_LOG_TRACE("TX__________ ");
        while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
        FTD_LOG_TRACE(" TX D __________");

        wait_time = 200; // [WARNNING!]read data size is large, so wait more time. 1024+18 Bytes will take abt 100ms
        while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
        {
            CLK_SysTickLongDelay(1000);
        }
        ftd_drv_burn_uart_rx_stop(p_st_slave_info->slave_uart_channel);

        FTD_LOG_TRACE_BUFF(READ_BUFF, sizeof(READ_BUFF), (SLAVE_FLASH_READ_CMD_SIZE_MAX + read_len));
        FTD_LOG_TRACE("  ");

        uint8_t ack = ftd_mw_slave_check_rx(READ_BUFF, (SLAVE_FLASH_READ_CMD_SIZE_MAX + read_len));

        if (SLAVE_CMD_READ_CODE_CRC == ack)
        {
            calc_fw_crc = crc16_ccitt_update(calc_fw_crc, (uint8_t*)&READ_BUFF[uart_cmd_ttl_len - CHECKSUM_SIZE], read_len);
            FTD_LOG_TRACE_BUFF((uint8_t*)&READ_BUFF[uart_cmd_ttl_len - CHECKSUM_SIZE], read_len, read_len);
            FTD_LOG_TRACE("  ");
            ret = true;
        }
        else
        {
            FTD_LOG_ERROR("ch:%d slave read unack", p_st_slave_info->slave_uart_channel);
            ret = false;
            break;
        }

        slv_addr += SLAVE_FLASH_READ_SIZE;
    }

    if (true == ret)
    {
        if (calc_fw_crc == st_sys_info.st_fw_info[0].st_bin_info.bin_crc16)
        {
            ret = true;
            FTD_LOG_INFO("CRC CHECK PASS, calc_crc:0x%08x bin_crc:0x%08x", calc_fw_crc, st_sys_info.st_fw_info[0].st_bin_info.bin_crc16);
        }
        else
        {
            ret = false;
            FTD_LOG_ERROR("READ CHECK ERR, calc_crc:0x%08x bin_crc:0x%08x", calc_fw_crc, st_sys_info.st_fw_info[0].st_bin_info.bin_crc16);
        }
    }

    FTD_LOG_INFO("READ SLAVE FLASH CRC %s ", ret ? "PASS" : "FAIL");

    return ret;
}

// write other data to slave
bool ftd_mw_slave_manager_disable_wp_parm(SLAVE_INFO* p_st_slave_info, CHANNEL_BURN_STATUS* st_channel_burn_status)
{
    bool    ret = false;
    uint8_t wait_time = 0;
    uint8_t send_cmd_buff[12] =
    { 0x4a, 0x59, 0x4d, 0x43,
        0x54,
        0x00, 0x04,
        0x00, 0x4A,
        0x00, 0x00,
        0xFF }; //uart protocol

    send_cmd_buff[sizeof(send_cmd_buff) - CHECKSUM_SIZE] = ftd_mw_slave_manager_get_crc(send_cmd_buff, (sizeof(send_cmd_buff) - CHECKSUM_SIZE));

    FTD_LOG_DEBUG_BUFF(send_cmd_buff, sizeof(send_cmd_buff), sizeof(send_cmd_buff));

    for (int i = 0; i < SLAVE_DISABLE_WP_TRIGGER_CNT; i++)
    {
        FTD_LOG_TRACE("tx start");
        memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
        ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, SLAVE_FLASH_STATUS_ACK_LEN);
        ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_cmd_buff, sizeof(send_cmd_buff));
        while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
        FTD_LOG_TRACE("rx start");

        wait_time = 30;
        while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
        {
            CLK_SysTickLongDelay(1000);
        }

        FTD_LOG_TRACE("rx end, wait_time:%d ", wait_time);

        FTD_LOG_TRACE_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));

        uint8_t slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));

        if (SLAVE_CMD_SET_FLASH_STATUS_REG == slave_ack && (send_cmd_buff[10] == SLAVE_CHN1_FUFF[10]))
        {
            FTD_LOG_DEBUG("ch:%d check flash status ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
            ret = true;
            break;
        }
        else
        {
            FTD_LOG_DEBUG("ch:%d slave flash status unack", p_st_slave_info->slave_uart_channel);
            ret = false;
        }
    }


    return ret;
}

bool ftd_mw_slave_manager_erase_parm(SLAVE_INFO* p_st_slave_info, CHANNEL_BURN_STATUS* st_channel_burn_status)
{
    bool    ret = false;
    uint32_t erase_size;
    uint8_t wait_time;
    uint8_t send_buff[18] =
    { 0x4a, 0x59, 0x4d, 0x43,
        0x54,
        0x00, 0x0a,
        0x00, 0x4f,
        0x08, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x87 }; //uart protocol

    // Get chip flash map by chip_code
    SYS_INFO sys_info;
    ftd_mw_sys_info_manager_get(&sys_info);
    uint16_t chip_code = (uint16_t)sys_info.st_fw_info[st_channel_burn_status->fw_num].chip_id;
    const CHIP_FLASH_ADDR_MAP* p_chip_map = ftd_mw_slave_get_chip_flash_map(chip_code);

    FTD_LOG_DEBUG("CH%d: Map Chip: %s, Author Addr: 0x%08X, Config A: 0x%08X, Config B: 0x%08X",
        p_st_slave_info->slave_uart_channel,
        p_chip_map->chip_name,
        p_chip_map->author_addr,
        p_chip_map->config_a_addr,
        p_chip_map->config_b_addr);

    // If both addresses are 0, return true directly
    if (p_chip_map->author_addr == 0 && p_chip_map->config_b_addr == 0)
    {
        FTD_LOG_INFO("CH%d: Both author and config addresses are 0, skip erase parm", p_st_slave_info->slave_uart_channel);
        return true;
    }

    // Determine erase range based on available addresses
    uint32_t erase_addr;
    if (p_chip_map->author_addr != 0 && p_chip_map->config_b_addr != 0)
    {
        // Both addresses exist, erase from author_addr to config_b_addr + 4KB
        erase_addr = p_chip_map->author_addr;
        erase_size = p_chip_map->config_b_addr - p_chip_map->author_addr + 0x1000;
        FTD_LOG_DEBUG("CH%d: Erase both author and config areas, addr: 0x%08X, size: 0x%X",
            p_st_slave_info->slave_uart_channel, erase_addr, erase_size);
    }
    else if (p_chip_map->author_addr != 0)
    {
        // Only author_addr exists, erase author area (4KB)
        erase_addr = p_chip_map->author_addr;
        erase_size = 0x1000; // 4KB
        FTD_LOG_DEBUG("CH%d: Erase only author area, addr: 0x%08X, size: 0x%X",
            p_st_slave_info->slave_uart_channel, erase_addr, erase_size);
    }
    else // p_chip_map->config_b_addr != 0
    {
        // Only config_b_addr exists, erase config area (4KB)
        erase_addr = p_chip_map->config_a_addr;
        erase_size = p_chip_map->config_b_addr - p_chip_map->config_a_addr + 0x1000;
        FTD_LOG_DEBUG("CH%d: Erase only config area, addr: 0x%08X, size: 0x%X",
            p_st_slave_info->slave_uart_channel, erase_addr, erase_size);
    }

    if (0 == erase_size)
    {
        FTD_LOG_INFO("CH%d: send slave erase cmd fail, erase_size = %x", p_st_slave_info->slave_uart_channel, erase_size);
        return ret;
    }

    // set erase addr
    send_buff[9] = (erase_addr >> 24) & 0xFF;
    send_buff[10] = (erase_addr >> 16) & 0xFF;
    send_buff[11] = (erase_addr >> 8) & 0xFF;
    send_buff[12] = erase_addr & 0xFF;

    // set slave flash erase size
    send_buff[13] = (erase_size >> 24) & 0xFF;
    send_buff[14] = (erase_size >> 16) & 0xFF;
    send_buff[15] = (erase_size >> 8) & 0xFF;
    send_buff[16] = erase_size & 0xFF;
    FTD_LOG_DEBUG("CH%d: erase_size = %d, 0x%x, 0x%x, 0x%x, 0x%x",
        p_st_slave_info->slave_uart_channel,
        erase_size,
        send_buff[13], send_buff[14], send_buff[15], send_buff[16]);

    // add crc 
    send_buff[sizeof(send_buff) - CHECKSUM_SIZE] = ftd_mw_slave_manager_get_crc(send_buff, (sizeof(send_buff) - CHECKSUM_SIZE));
    FTD_LOG_DEBUG_BUFF(send_buff, sizeof(send_buff), sizeof(send_buff));

    memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
    ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_buff, sizeof(send_buff));
    FTD_LOG_TRACE(" TX  ");
    while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));

    ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, 35); // 19 35
    FTD_LOG_TRACE(" TX D ");
    CLK_SysTickLongDelay(200000);

    FTD_LOG_TRACE(" WHILE RX ");
    wait_time = 30;
    while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }
    ftd_drv_burn_uart_rx_stop(p_st_slave_info->slave_uart_channel);
    FTD_LOG_TRACE(" WHILE RX DONE ");

    uint8_t ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));
    if (SLAVE_CMD_ERASE_FLASH_SECTOR == ack
        && send_buff[9] == SLAVE_CHN1_FUFF[9]
        && send_buff[10] == SLAVE_CHN1_FUFF[10]
        && send_buff[11] == SLAVE_CHN1_FUFF[11]
        && send_buff[12] == SLAVE_CHN1_FUFF[12]
        && send_buff[13] == SLAVE_CHN1_FUFF[13]
        && send_buff[14] == SLAVE_CHN1_FUFF[14]
        && send_buff[15] == SLAVE_CHN1_FUFF[15]
        && send_buff[16] == SLAVE_CHN1_FUFF[16]
        )
    {
        FTD_LOG_DEBUG("ch:%d slave erase ack [0x%02x]", p_st_slave_info->slave_uart_channel, ack);
        ret = true;

        // abt 20ms, wait for slave erase done
        CLK_SysTickLongDelay(200000);
    }
    else
    {
        FTD_LOG_DEBUG("ch:%d slave erase unack", p_st_slave_info->slave_uart_channel);
        FTD_LOG_DEBUG_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));
    }

    ftd_drv_burn_uart_rx_stop(p_st_slave_info->slave_uart_channel);

    return ret;
}

bool ftd_mw_slave_manager_program_parm(SLAVE_INFO* p_st_slave_info, CHANNEL_BURN_STATUS* st_channel_burn_status)
{
    bool     ret = false;
    uint8_t  read_partition;
    uint32_t read_length;
    uint32_t slv_addr;
    uint32_t data_size;
    //uint32_t rollcode;
    uint8_t  data_buff[SLAVE_FLASH_PROG_SIZE];
    static uint8_t send_buff[SLAVE_FLASH_PROG_CMD_SIZE_MAX] = { 0x4a, 0x59, 0x4d, 0x43, 0x54 };
    //uint16_t uart_cmd_ttl_len = 0;
    uint16_t send_buff_len = 0;
    uint16_t wait_time;
    SYS_INFO sys_info;


    ftd_mw_sys_info_manager_get(&sys_info);

    // Get chip flash map by chip_code
    uint16_t chip_code = (uint16_t)sys_info.st_fw_info[st_channel_burn_status->fw_num].chip_id;
    const CHIP_FLASH_ADDR_MAP* p_chip_map = ftd_mw_slave_get_chip_flash_map(chip_code);

    FTD_LOG_DEBUG("CH%d: Map Chip: %s, Author Addr: 0x%08X, Config A: 0x%08X", p_st_slave_info->slave_uart_channel, p_chip_map->chip_name, p_chip_map->author_addr, p_chip_map->config_a_addr);

    // If extra addresses are 0, return true directly
    if (p_chip_map->author_addr == 0 || p_chip_map->config_a_addr == 0)
    {
        FTD_LOG_INFO("CH%d: Extra addresses are 0, skip program parm", p_st_slave_info->slave_uart_channel);
        return true;
    }

    // 1. write authorization info to slave
    uint8_t author_prog_flag = false;
    uint8_t author_info[ROLLCODE_LEN + EASYMESH_PID_LEN + EASYMESH_RESERVED_LEN + TRIPLE_DATA_LEN + AUTHOR_INFO_CRC_LEN] = { 0 };
    data_size = ROLLCODE_LEN + EASYMESH_PID_LEN + EASYMESH_RESERVED_LEN + TRIPLE_DATA_LEN + AUTHOR_INFO_CRC_LEN;

    slv_addr = p_chip_map->author_addr;  // Use mapped address instead of hardcoded macro

    memset(author_info, 0xFF, sizeof(author_info));

    FTD_LOG_TRACE("data_size:0x%08x ", data_size);
    FTD_LOG_DEBUG("rollcode remain cnt:%d triple remain cnt:%d",
        sys_info.st_fw_info[st_channel_burn_status->fw_num].st_roll_code_info.remain_counts,
        sys_info.st_fw_info[st_channel_burn_status->fw_num].st_triple_info.remain_counts);

    if (0 != sys_info.st_fw_info[st_channel_burn_status->fw_num].st_roll_code_info.remain_counts)
    {
        uint32_t rollcode = sys_info.st_fw_info[st_channel_burn_status->fw_num].st_roll_code_info.end_value
            - sys_info.st_fw_info[st_channel_burn_status->fw_num].st_roll_code_info.remain_counts + 1;

        author_info[0] = 0x00;
        author_info[1] = 0x00;
        author_info[2] = (rollcode >> 24) & 0xFF;
        author_info[3] = (rollcode >> 16) & 0xFF;
        author_info[4] = (rollcode >> 8) & 0xFF;
        author_info[5] = rollcode & 0xFF;

        FTD_LOG_DEBUG("rollcode:0x%0x ", rollcode);
        sys_info.st_fw_info[st_channel_burn_status->fw_num].st_roll_code_info.remain_counts--;
        author_prog_flag = true;
    }

    uint8_t invalid_pid[EASYMESH_PID_LEN] = { 0x00, 0x00, 0x00, 0x00 };

    if (0 != memcmp(invalid_pid, sys_info.st_fw_info[st_channel_burn_status->fw_num].st_pid_info.easy_mesh_pid, EASYMESH_PID_LEN))
    {
        memcpy(&author_info[ROLLCODE_LEN], sys_info.st_fw_info[st_channel_burn_status->fw_num].st_pid_info.easy_mesh_pid, EASYMESH_PID_LEN);

        FTD_LOG_DEBUG_BUFF(sys_info.st_fw_info[st_channel_burn_status->fw_num].st_pid_info.easy_mesh_pid, EASYMESH_PID_LEN, EASYMESH_PID_LEN);
        author_prog_flag = true;
    }

    if (0 != sys_info.st_fw_info[st_channel_burn_status->fw_num].st_triple_info.remain_counts)
    {
        read_partition = PARTITION_FW1_TRIPLE + st_channel_burn_status->fw_num * (PARTITION_FW2_TRIPLE - PARTITION_FW1_TRIPLE);
        read_length = 0;
        uint16_t idx = 0;

        idx = sys_info.st_fw_info[st_channel_burn_status->fw_num].st_triple_info.remain_counts - 1;
        read_length = idx * TRIPLE_DATA_LEN;

        if (ftd_drv_flash_operation((PARTITION_NAME)read_partition, OPERATION_READ, read_length, TRIPLE_DATA_LEN, data_buff) == 0)
        {
            memcpy(&author_info[ROLLCODE_LEN + EASYMESH_PID_LEN + EASYMESH_RESERVED_LEN], data_buff, TRIPLE_DATA_LEN);

            FTD_LOG_DEBUG("triple idx:%d offset:%d", idx, read_length);
            FTD_LOG_TRACE_BUFF(data_buff, TRIPLE_DATA_LEN, TRIPLE_DATA_LEN);

            sys_info.st_fw_info[st_channel_burn_status->fw_num].st_triple_info.remain_counts--;
            author_prog_flag = true;
        }
        else
        {
            // error handle
            FTD_LOG_DEBUG("read errro. triple idx:%d offset:%d", idx, read_length);
            return ret;
        }
    }

    // send author info
    if (true == author_prog_flag)
    {
        uint16_t parm_len = EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_OP_LENGTH_SIZE + data_size; // 4 + 4 + 2 + len
        // add author info crc16
        uint16_t info_crc16 = crc16_ccitt_update(0xFFFF, (uint8_t*)&author_info, data_size - AUTHOR_INFO_CRC_LEN);
        author_info[data_size - 2] = (info_crc16 >> 8) & 0xFF;
        author_info[data_size - 1] = info_crc16 & 0xFF;
        FTD_LOG_TRACE_BUFF(&author_info[data_size - 2], data_size, AUTHOR_INFO_CRC_LEN);

        // save remain cnt to sys_info
        ftd_mw_sys_info_manager_set(&sys_info);

        // prepare uart send data
        send_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET] = 0x00;
        send_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET + 1] = 0x51;
        // slave flash addr
        send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET] = (slv_addr >> 24) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 1] = (slv_addr >> 16) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 2] = (slv_addr >> 8) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 3] = slv_addr & 0xFF;
        // data length (fixed 0x00 for alain)
        send_buff[SLAVE_FLASH_PROG_LEN_OFFSET] = 0x00;
        send_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 1] = 0x00;
        send_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 2] = 0x00;

        send_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET] = (parm_len >> 8) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] = parm_len & 0xFF;

        send_buff_len = SLAVE_FLASH_PROG_DATA_OFFSET + data_size + CHECKSUM_SIZE;
        memcpy(&send_buff[SLAVE_FLASH_PROG_DATA_OFFSET], &author_info, data_size);

        // add prog cmd crc 
        send_buff[send_buff_len - CHECKSUM_SIZE] = ftd_mw_slave_manager_get_crc(send_buff, (send_buff_len - CHECKSUM_SIZE));

        FTD_LOG_TRACE(" PROG SEND[%d]:", send_buff_len);
        FTD_LOG_TRACE_BUFF(send_buff, send_buff_len, send_buff_len);

        memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
        ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, SLAVE_FLASH_PROG_ACK_LEN);
        ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_buff, send_buff_len);
        FTD_LOG_TRACE("_TX_");
        while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
        FTD_LOG_TRACE("__D__");

        wait_time = 10;
        while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
        {
            CLK_SysTickLongDelay(1000); // 1ms
        }
        ftd_drv_burn_uart_rx_stop(p_st_slave_info->slave_uart_channel);

        //FTD_LOG_DEBUG_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));
        uint8_t ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));
        //ack = SLAVE_CMD_WRITE_FLASH_DATA;
        if (SLAVE_CMD_WRITE_FLASH_DATA == ack
            && send_buff[9] == SLAVE_CHN1_FUFF[9]
            && send_buff[10] == SLAVE_CHN1_FUFF[10]
            && send_buff[11] == SLAVE_CHN1_FUFF[11]
            && send_buff[12] == SLAVE_CHN1_FUFF[12])
        {
            FTD_LOG_TRACE("ch:%d slave prog ack [0x%02x] cmd_len[%d] buf_len[%d]", p_st_slave_info->slave_uart_channel,
                ack, send_buff_len, SLAVE_FLASH_PROG_CMD_SIZE_MAX);
            ret = true;
        }
        else
        {
            FTD_LOG_ERROR("ch:%d slave prog unack", p_st_slave_info->slave_uart_channel);
            ret = false;
            return ret;
        }
    }

    // 2. write burn info to slave
    uint8_t burn_info[MPN_LEN + EID_LEN + BURN_EVENT_ID_LEN + BURN_PROGRAMMER_LEN + BURN_EASYMESH_RESERVED_LEN + BURN_INFO_CRC_LEN] = { 0 };
    data_size = MPN_LEN + EID_LEN + BURN_EVENT_ID_LEN + BURN_PROGRAMMER_LEN + BURN_EASYMESH_RESERVED_LEN + BURN_INFO_CRC_LEN;

    slv_addr = p_chip_map->burn_info_addr;  // Use mapped address instead of hardcoded macro
    if (slv_addr != 0)
    {
        // Initialize burn info buffer with 0xFF
        memset(burn_info, 0xFF, sizeof(burn_info));

        // Fill MPN (Manufacturing Part Number)
        memcpy(&burn_info[0], sys_info.st_fw_info[st_channel_burn_status->fw_num].st_production_burn_info.mpn, MPN_LEN);

        // Fill EID (Electronic ID)
        memcpy(&burn_info[MPN_LEN], sys_info.st_fw_info[st_channel_burn_status->fw_num].st_production_burn_info.eip_num, EID_LEN);

        // Fill Burn Event ID
        memcpy(&burn_info[MPN_LEN + EID_LEN], sys_info.st_fw_info[st_channel_burn_status->fw_num].st_production_burn_info.burn_event_id, BURN_EVENT_ID_LEN);

        // BURN_PROGRAMMER_LEN and BURN_EASYMESH_RESERVED_LEN remain 0xFF as initialized

        // Calculate CRC16 for burn info
        uint16_t burn_info_crc16 = crc16_ccitt_update(0xFFFF, (uint8_t*)&burn_info, data_size - BURN_INFO_CRC_LEN);
        burn_info[data_size - 2] = (burn_info_crc16 >> 8) & 0xFF;
        burn_info[data_size - 1] = burn_info_crc16 & 0xFF;

        // Prepare UART send data
        uint16_t parm_len = EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_OP_LENGTH_SIZE + data_size; // 4 + 4 + 2 + len
        send_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET] = 0x00;
        send_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET + 1] = 0x51;
        // Slave flash address
        send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET] = (slv_addr >> 24) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 1] = (slv_addr >> 16) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 2] = (slv_addr >> 8) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 3] = slv_addr & 0xFF;
        // Data length (fixed 0x00 for alain)
        send_buff[SLAVE_FLASH_PROG_LEN_OFFSET] = 0x00;
        send_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 1] = 0x00;
        send_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 2] = 0x00;

        send_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET] = (parm_len >> 8) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] = parm_len & 0xFF;

        send_buff_len = SLAVE_FLASH_PROG_DATA_OFFSET + data_size + CHECKSUM_SIZE;
        memcpy(&send_buff[SLAVE_FLASH_PROG_DATA_OFFSET], &burn_info, data_size);

        // Add prog cmd CRC
        send_buff[send_buff_len - CHECKSUM_SIZE] = ftd_mw_slave_manager_get_crc(send_buff, (send_buff_len - CHECKSUM_SIZE));

        FTD_LOG_TRACE(" BURN INFO SEND[%d]:", send_buff_len);
        FTD_LOG_TRACE_BUFF(send_buff, send_buff_len, send_buff_len);

        memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
        ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, SLAVE_FLASH_PROG_ACK_LEN);
        ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_buff, send_buff_len);
        FTD_LOG_TRACE("_TX_");
        while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
        FTD_LOG_TRACE("__D__");

        wait_time = 10;
        while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
        {
            CLK_SysTickLongDelay(1000); // 1ms
        }
        ftd_drv_burn_uart_rx_stop(p_st_slave_info->slave_uart_channel);

        uint8_t ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));
        if (SLAVE_CMD_WRITE_FLASH_DATA == ack
            && send_buff[9] == SLAVE_CHN1_FUFF[9]
            && send_buff[10] == SLAVE_CHN1_FUFF[10]
            && send_buff[11] == SLAVE_CHN1_FUFF[11]
            && send_buff[12] == SLAVE_CHN1_FUFF[12])
        {
            FTD_LOG_TRACE("ch:%d slave burn info prog ack [0x%02x] cmd_len[%d] buf_len[%d]", p_st_slave_info->slave_uart_channel,
                ack, send_buff_len, SLAVE_FLASH_PROG_CMD_SIZE_MAX);
            ret = true;
        }
        else
        {
            FTD_LOG_ERROR("ch:%d slave burn info prog unack", p_st_slave_info->slave_uart_channel);
            ret = false;
            return ret;
        }
    }

    // 3. write config bin to slave
    data_size = sys_info.st_fw_info[st_channel_burn_status->fw_num].st_config_info.size;
    slv_addr = p_chip_map->config_a_addr;  // Use mapped address instead of hardcoded macro

    memset(data_buff, 0xFF, sizeof(data_size));
    read_partition = PARTITION_FW1_BIN + st_channel_burn_status->fw_num * (PARTITION_FW2_BIN - PARTITION_FW1_BIN);

    FTD_LOG_DEBUG("data_size:0x%08x last_pkt_valid_data_len:0x%08x", data_size, (data_size % SLAVE_FLASH_PROG_SIZE));

    for (read_length = p_st_slave_info->flash_bin_size; (read_length - p_st_slave_info->flash_bin_size) < data_size; read_length += SLAVE_FLASH_PROG_SIZE)
    {
        FTD_LOG_DEBUG("read addr:0x%08x len:0x%08x slv_addr:0x%08x ", read_partition, read_length, slv_addr);

        if (ftd_drv_flash_operation((PARTITION_NAME)read_partition, OPERATION_READ, read_length, SLAVE_FLASH_PROG_SIZE, data_buff) == 0)
        {
            // send uart pkt
            // ext cmd
            send_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET] = 0x00;
            send_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET + 1] = 0x51;
            // slave flash addr
            send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET] = (slv_addr >> 24) & 0xFF;
            send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 1] = (slv_addr >> 16) & 0xFF;
            send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 2] = (slv_addr >> 8) & 0xFF;
            send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 3] = slv_addr & 0xFF;
            // data length (fixed 0x00 for alain)
            send_buff[SLAVE_FLASH_PROG_LEN_OFFSET] = 0x00;
            send_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 1] = 0x00;
            send_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 2] = 0x00;

            if ((read_length - p_st_slave_info->flash_bin_size) + SLAVE_FLASH_PROG_SIZE > data_size)
            {
                // last page
                FTD_LOG_TRACE_BUFF(data_buff, sizeof(data_buff), (data_size % SLAVE_FLASH_PROG_SIZE));

                uint32_t data_len = data_size % SLAVE_FLASH_PROG_SIZE;
                uint16_t parm_len = EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_OP_LENGTH_SIZE + data_len; // 4 + 4 + 2 + len

                // calc pram len
                send_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET] = (parm_len >> 8) & 0xFF;
                send_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] = parm_len & 0xFF;

                // copy bin
                memcpy(&send_buff[SLAVE_FLASH_PROG_DATA_OFFSET], data_buff, data_len);

                send_buff_len = SLAVE_FLASH_PROG_DATA_OFFSET + data_len + CHECKSUM_SIZE;
            }
            else
            {
                FTD_LOG_TRACE_BUFF(data_buff, sizeof(data_buff), SLAVE_FLASH_PROG_SIZE);

                // calc pram len
                send_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET] = (SLAVE_FLASH_PROG_PRAM_LEN_MAX >> 8) & 0xFF;
                send_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] = SLAVE_FLASH_PROG_PRAM_LEN_MAX & 0xFF;

                // copy bin
                memcpy(&send_buff[SLAVE_FLASH_PROG_DATA_OFFSET], data_buff, SLAVE_FLASH_PROG_SIZE);

                send_buff_len = SLAVE_FLASH_PROG_CMD_SIZE_MAX;
            }

            // add crc 
            send_buff[send_buff_len - CHECKSUM_SIZE] = ftd_mw_slave_manager_get_crc(send_buff, (send_buff_len - CHECKSUM_SIZE));

            FTD_LOG_TRACE(" PROG SEND[%d]:", send_buff_len);
            FTD_LOG_TRACE_BUFF(send_buff, send_buff_len, send_buff_len);

            // send uart pkt to slave
            // [Bugfix] The compiler has assigned duplicate addresses to array SLAVE_CHN1_FUFF and send_buff
            // so need use static array to avoid this issue
            memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
            ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, SLAVE_FLASH_PROG_ACK_LEN);
            ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_buff, send_buff_len);
            FTD_LOG_TRACE("_TX_");
            while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
            FTD_LOG_TRACE("__D__");

            wait_time = 10;
            while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
            {
                CLK_SysTickLongDelay(1000); // 1ms
            }
            ftd_drv_burn_uart_rx_stop(p_st_slave_info->slave_uart_channel);

            //FTD_LOG_DEBUG_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));
            uint8_t ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));
            if (SLAVE_CMD_WRITE_FLASH_DATA == ack
                && send_buff[9] == SLAVE_CHN1_FUFF[9]
                && send_buff[10] == SLAVE_CHN1_FUFF[10]
                && send_buff[11] == SLAVE_CHN1_FUFF[11]
                && send_buff[12] == SLAVE_CHN1_FUFF[12])
            {
                FTD_LOG_TRACE("ch:%d slave prog ack [0x%02x] cmd_len[%d] buf_len[%d]", p_st_slave_info->slave_uart_channel,
                    ack, send_buff_len, SLAVE_FLASH_PROG_CMD_SIZE_MAX);
                ret = true;
            }
            else
            {
                FTD_LOG_ERROR("ch:%d slave prog unack", p_st_slave_info->slave_uart_channel);
                ret = false;
                break;
            }
        }
        else
        {
            FTD_LOG_ERROR("ch:%d read config bin fail", p_st_slave_info->slave_uart_channel);
            ret = false;
            break;
        }

        slv_addr += SLAVE_FLASH_PROG_SIZE;
    }

    return ret;
}

bool ftd_mw_slave_manager_read_crc_parm(SLAVE_INFO* p_st_slave_info, CHANNEL_BURN_STATUS* st_channel_burn_status)
{
    bool     ret = false;
    SYS_INFO sys_info;
    uint32_t read_ttl_len;
    uint32_t slv_addr;
    uint32_t data_size;
    uint32_t read_len;
    uint8_t  send_buff[SLAVE_FLASH_READ_CMD_SIZE_MAX] = { 0x4a, 0x59, 0x4d, 0x43, 0x54 };
    uint16_t send_buff_len = 0;
    uint16_t wait_time;
    uint16_t calc_fw_crc = 0xFFFF; // init crc value is fixed 0xFFFF
    uint8_t  READ_BUFF[SLAVE_FLASH_READ_CMD_ACK_SIZE_MAX];


    ftd_mw_sys_info_manager_get(&sys_info);

    // Get chip flash map by chip_code
    uint16_t chip_code = (uint16_t)sys_info.st_fw_info[st_channel_burn_status->fw_num].chip_id;
    const CHIP_FLASH_ADDR_MAP* p_chip_map = ftd_mw_slave_get_chip_flash_map(chip_code);

    FTD_LOG_DEBUG("CH%d: Map Chip: %s, Config A: 0x%08X", p_st_slave_info->slave_uart_channel, p_chip_map->chip_name, p_chip_map->config_a_addr);

    // If extra addresses are 0, return true directly
    if (p_chip_map->config_a_addr == 0)
    {
        FTD_LOG_INFO("CH%d: Extra addresses are 0, skip read crc parm", p_st_slave_info->slave_uart_channel);
        return true;
    }

    // 1. read authorization info from slave flash
    //data_size = ROLLCODE_LEN + EASYMESH_PID_LEN + TRIPLE_DATA_LEN + AUTHOR_INFO_CRC_LEN;
    //slv_addr  = SLAVE_FLASH_CHIP_0_MAP_0_AUTHOR_ADDR;

    // 2. read client info from slave

    // 3. read write config bin from slave
    data_size = sys_info.st_fw_info[st_channel_burn_status->fw_num].st_config_info.size;
    slv_addr = p_chip_map->config_a_addr;  // Use mapped address instead of hardcoded macro

    FTD_LOG_DEBUG("data_size:0x%08x last_page_valid_data_len:0x%08x", data_size, (data_size % SLAVE_FLASH_READ_SIZE));

    // package read cmd buffer
    // calc pram len
    uint16_t parm_len = EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_READ_LENGTH_SIZE; // 4 + 4 + 2
    send_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET] = (parm_len >> 8) & 0xFF;
    send_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] = parm_len & 0xFF;
    // ext cmd
    send_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET] = (SLAVE_EXT_CMD_READ_FLASH >> 8) & 0xFF;//0x00;
    send_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET + 1] = SLAVE_EXT_CMD_READ_FLASH & 0xFF;//0x50;

    for (read_ttl_len = 0; read_ttl_len < data_size; read_ttl_len += SLAVE_FLASH_READ_SIZE)
    {
        FTD_LOG_TRACE(" read_slv_addr:0x%08x ", slv_addr);

        // slave flash addr
        send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET] = (slv_addr >> 24) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 1] = (slv_addr >> 16) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 2] = (slv_addr >> 8) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 3] = slv_addr & 0xFF;

        // read length
        if (read_ttl_len + SLAVE_FLASH_READ_SIZE > data_size)
        {
            read_len = data_size % SLAVE_FLASH_READ_SIZE;
        }
        else
        {
            read_len = SLAVE_FLASH_READ_SIZE;
        }
        // add a addr protection in here
        send_buff[SLAVE_FLASH_PROG_LEN_OFFSET] = (read_len >> 24) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 1] = (read_len >> 16) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 2] = (read_len >> 8) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 3] = read_len & 0xFF;

        send_buff_len = SLAVE_FLASH_READ_DATA_OFFSET + CHECKSUM_SIZE;

        // add crc 
        send_buff[send_buff_len - CHECKSUM_SIZE] = ftd_mw_slave_manager_get_crc(send_buff, (send_buff_len - CHECKSUM_SIZE));

        FTD_LOG_DEBUG_BUFF(send_buff, sizeof(send_buff), send_buff_len);

        // send uart pkt to slave
        memset(READ_BUFF, 0x00, sizeof(READ_BUFF));
        FTD_LOG_DEBUG_BUFF(READ_BUFF, sizeof(READ_BUFF), (SLAVE_FLASH_READ_CMD_SIZE_MAX + read_len));
        ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, READ_BUFF, (SLAVE_FLASH_READ_CMD_SIZE_MAX + read_len));
        ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_buff, send_buff_len);
        FTD_LOG_TRACE("TX__________ ");
        while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
        FTD_LOG_TRACE(" TX D __________");

        wait_time = 30;
        while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
        {
            CLK_SysTickLongDelay(1000);
        }
        ftd_drv_burn_uart_rx_stop(p_st_slave_info->slave_uart_channel);

        FTD_LOG_DEBUG_BUFF(READ_BUFF, sizeof(READ_BUFF), (SLAVE_FLASH_READ_CMD_SIZE_MAX + read_len));
        FTD_LOG_TRACE("  ");

        uint8_t ack = ftd_mw_slave_check_rx(READ_BUFF, (SLAVE_FLASH_READ_CMD_SIZE_MAX + read_len));

        if (SLAVE_CMD_READ_CODE_CRC == ack)
        {
            calc_fw_crc = crc16_ccitt_update(calc_fw_crc, (uint8_t*)&READ_BUFF[send_buff_len - CHECKSUM_SIZE], read_len);
            FTD_LOG_TRACE_BUFF((uint8_t*)&READ_BUFF[send_buff_len - CHECKSUM_SIZE], read_len, read_len);
            FTD_LOG_TRACE("  ");
            ret = true;
        }
        else
        {
            FTD_LOG_ERROR("ch:%d slave read unack", p_st_slave_info->slave_uart_channel);
            ret = false;
            break;
        }

        slv_addr += SLAVE_FLASH_READ_SIZE;
    }

    if (calc_fw_crc == sys_info.st_fw_info[st_channel_burn_status->fw_num].st_config_info.config_crc16)
    {
        ret = true;
        FTD_LOG_INFO("CRC CHECK PASS, calc_crc:0x%08x bin_crc:0x%08x", calc_fw_crc, sys_info.st_fw_info[st_channel_burn_status->fw_num].st_config_info.config_crc16);
    }
    else
    {
        ret = false;
        FTD_LOG_ERROR("READ CHECK ERR, calc_crc:0x%08x bin_crc:0x%08x", calc_fw_crc, sys_info.st_fw_info[st_channel_burn_status->fw_num].st_config_info.config_crc16);
    }

    FTD_LOG_DEBUG("READ SLAVE FLASH DONE");

    return ret;
}

bool ftd_mw_slave_manager_sort(SLAVE_INFO* p_st_slave_info)
{
    // 1.power off slave
    ftd_mw_slave_manager_power_off(p_st_slave_info->slave_uart_channel);

    return true;
}

/******************************************************************************
 * Functions related to FT testing
 ******************************************************************************/
 //test send packet address
bool ftd_mw_slave_manager_test_send_packet_address(SLAVE_INFO* p_st_slave_info)
{
    bool ret = false;
    uint16_t wait_time = 0;
    uint8_t send_cmd_buff[6] = { 0x5A, 0x17, 0x00, 0x00, 0x00, 0x00 }; //uart protocol
    uint32_t packet_address = 0x00000000;
    packet_address = ftd_utils_slave_read_uid_build_addr();

    /* 修改地址低4位为通道号 */
    packet_address &= 0xFFFFFF00;
    packet_address |= (p_st_slave_info->slave_uart_channel & 0xFF);

    send_cmd_buff[2] = (uint8_t)((packet_address >> 24) & 0xFF);
    send_cmd_buff[3] = (uint8_t)((packet_address >> 16) & 0xFF);
    send_cmd_buff[4] = (uint8_t)((packet_address >> 8) & 0xFF);
    send_cmd_buff[5] = (uint8_t)(packet_address & 0xFF);


    FTD_LOG_TRACE("tx start");
    memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
    ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, 2);
    ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_cmd_buff, sizeof(send_cmd_buff));
    while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
    FTD_LOG_TRACE("rx start");

    //3. check send packet address ack
    wait_time = 30;
    while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }

    FTD_LOG_DEBUG("rx end, wait_time:%d ", wait_time);

    FTD_LOG_TRACE_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));

    uint8_t slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));

    FTD_LOG_DEBUG("slave_ack:%d ", slave_ack);
    if (SLAVE_CMD_SEND_PACKET_ADDR == slave_ack)
    {
        FTD_LOG_DEBUG("ch:%d check send packet address ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
        ret = true;
    }
    else
    {
        FTD_LOG_DEBUG("ch:%d slave send packet address unack", p_st_slave_info->slave_uart_channel);
        ret = false;
    }


    return ret;
}

bool ftd_mw_slave_manager_test_jump_bootloader(SLAVE_INFO* p_st_slave_info)
{
    bool ret = false;
    uint16_t wait_time = 0;
    uint8_t send_cmd_buff[2] = { 0x5A, 0x66 }; //uart protocol


    FTD_LOG_TRACE("tx start");
    memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
    ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, 2);
    ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_cmd_buff, sizeof(send_cmd_buff));
    while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
    FTD_LOG_TRACE("rx start");

    //3. check send packet address ack
    wait_time = 30;
    while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }

    FTD_LOG_DEBUG("rx end, wait_time:%d ", wait_time);

    FTD_LOG_TRACE_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));

    uint8_t slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));

    FTD_LOG_DEBUG("slave_ack:%d ", slave_ack);
    if (SLAVE_CMD_JUMP_BOOTLOADER == slave_ack)
    {
        FTD_LOG_DEBUG("ch:%d check jump bootloader ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
        ret = true;
    }
    else
    {
        FTD_LOG_DEBUG("ch:%d slave jump bootloader unack", p_st_slave_info->slave_uart_channel);
        ret = false;
    }


    return ret;
}

bool ftd_mw_slave_manager_write_chip_id(SLAVE_INFO* p_st_slave_info)
{
    bool    cmd_ret = false;

    uint16_t wait_time = 0;
    uint8_t  send_buff[4] = { 0x5A, 0x44, 0x00, 0x00 };

    // Store chip_code in little-endian format at positions 3 and 4 (indices 2 and 3)
    uint16_t chip_code = (uint16_t)p_st_slave_info->chip_id;
    send_buff[2] = (uint8_t)(chip_code & 0xFF);      // Low byte (little-endian)
    send_buff[3] = (uint8_t)((chip_code >> 8) & 0xFF); // High byte


    //1. send test calibrate madc command
    FTD_LOG_TRACE("tx start");
    memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
    ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, 2);
    ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_buff, sizeof(send_buff));
    while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
    FTD_LOG_TRACE("rx start");

    //2. check calibrate madc ack
    wait_time = 1000;
    while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }

    FTD_LOG_DEBUG("rx end, wait_time:%d ", wait_time);
    FTD_LOG_TRACE_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));
    uint8_t slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));
    if (SLAVE_CMD_FT_WRITE_CHIP_ID == slave_ack)
    {
        FTD_LOG_DEBUG("ch:%d check write chip id ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
        cmd_ret = true;
    }
    else
    {
        FTD_LOG_WARN("ch:%d slave write chip id unack", p_st_slave_info->slave_uart_channel);
        cmd_ret = false;
        return cmd_ret;
    }


    return cmd_ret;

}

#ifndef FT_TEST_ROM
bool ftd_mw_slave_ft_test_manager_program_test(SLAVE_INFO* p_st_slave_info, CHANNEL_BURN_STATUS* st_channel_burn_status)
{
    bool     ret = false;
    uint8_t  read_partition;
    uint32_t fw_read_length;
    uint32_t slv_addr;
    uint32_t fw_size;
    uint8_t  fw_buff[SLAVE_FLASH_PROG_SIZE];
    static uint8_t uart_buff[SLAVE_FLASH_PROG_CMD_SIZE_MAX] = { 0x4a, 0x59, 0x4d, 0x43, 0x54 };
    uint16_t uart_cmd_ttl_len = 0;
    uint16_t wait_time;
    SYS_INFO sys_info;

    // Get chip flash map by chip_code
    ftd_mw_sys_info_manager_get(&sys_info);
    uint16_t chip_code = (uint16_t)sys_info.st_fw_info[st_channel_burn_status->fw_num].chip_id;
    const CHIP_FLASH_ADDR_MAP* p_chip_map = ftd_mw_slave_get_chip_flash_map(chip_code);

    // fw_size = p_st_slave_info->flash_bin_size;
    fw_size = FT_TEST_SLAVE_BIN_SIZE;
    slv_addr = p_chip_map->fw_bin_a_addr;  // Use mapped address instead of hardcoded macro

    FTD_LOG_TRACE("fw_size:0x%08x last_pkt_valid_data_len:0x%08x", fw_size, (fw_size % SLAVE_FLASH_PROG_SIZE));

    read_partition = PARTITION_FW1_BIN + st_channel_burn_status->fw_num * (PARTITION_FW2_BIN - PARTITION_FW1_BIN);

    for (fw_read_length = 0; fw_read_length < fw_size; fw_read_length += SLAVE_FLASH_PROG_SIZE)
    {
        FTD_LOG_TRACE("read addr:0x%08x len:0x%08x slv_addr:0x%08x ", PARTITION_FW1_BIN, fw_read_length, slv_addr);
        if (ftd_drv_fmc_read_data(FT_TEST_SLAVE_ADDR, fw_read_length, fw_buff, SLAVE_FLASH_PROG_SIZE) == 0)
        {
            // send uart pkt
            // ext cmd
            uart_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET] = 0x00;
            uart_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET + 1] = 0x51;
            // slave flash addr
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET] = (slv_addr >> 24) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 1] = (slv_addr >> 16) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 2] = (slv_addr >> 8) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 3] = slv_addr & 0xFF;
            // data length (fixed 0x00 for alain)
            uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET] = 0x00;
            uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 1] = 0x00;
            uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 2] = 0x00;

            if (fw_read_length + SLAVE_FLASH_PROG_SIZE > fw_size)
            {
                // last page
                //FTD_LOG_TRACE_BUFF(fw_buff, sizeof(fw_buff), (fw_size % SLAVE_FLASH_PROG_SIZE));

                uint32_t data_len = fw_size % SLAVE_FLASH_PROG_SIZE;
                uint16_t parm_len = EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_OP_LENGTH_SIZE + data_len; // 4 + 4 + 2 + len

                // calc pram len
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET] = (parm_len >> 8) & 0xFF;
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] = parm_len & 0xFF;

                // copy bin
                memcpy(&uart_buff[SLAVE_FLASH_PROG_DATA_OFFSET], fw_buff, data_len);

                uart_cmd_ttl_len = SLAVE_FLASH_PROG_DATA_OFFSET + data_len + CHECKSUM_SIZE;
            }
            else
            {
                // calc pram len
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET] = (SLAVE_FLASH_PROG_PRAM_LEN_MAX >> 8) & 0xFF;
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] = SLAVE_FLASH_PROG_PRAM_LEN_MAX & 0xFF;

                // copy bin
                memcpy(&uart_buff[SLAVE_FLASH_PROG_DATA_OFFSET], fw_buff, SLAVE_FLASH_PROG_SIZE);

                uart_cmd_ttl_len = SLAVE_FLASH_PROG_CMD_SIZE_MAX;
            }

            // add crc 
            uart_buff[uart_cmd_ttl_len - CHECKSUM_SIZE] = ftd_mw_slave_manager_get_crc(uart_buff, (uart_cmd_ttl_len - CHECKSUM_SIZE));

            // for debug only
            // FTD_LOG_DEBUG(" PROG SEND[%d]:", uart_cmd_ttl_len);
            // FTD_LOG_DEBUG_BUFF(uart_buff, uart_cmd_ttl_len, uart_cmd_ttl_len);

            // send uart pkt to slave
            // [Bugfix] The compiler has assigned duplicate addresses to array SLAVE_CHN1_FUFF and uart_buff
            // so need use static array to avoid this issue
            memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
            ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, SLAVE_FLASH_PROG_ACK_LEN);
            ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, uart_buff, uart_cmd_ttl_len);
            FTD_LOG_TRACE("_TX_");
            while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
            FTD_LOG_TRACE("__D__");

            wait_time = 100;
            while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
            {
                CLK_SysTickLongDelay(1000); // 1ms
            }
            ftd_drv_burn_uart_rx_stop(p_st_slave_info->slave_uart_channel);

            //FTD_LOG_DEBUG_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));
            uint8_t ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));
            if (SLAVE_CMD_WRITE_FLASH_DATA == ack)
            {
                FTD_LOG_TRACE("ch:%d slave prog ack [0x%02x] cmd_len[%d] buf_len[%d]", p_st_slave_info->slave_uart_channel,
                    ack, uart_cmd_ttl_len, SLAVE_FLASH_PROG_CMD_SIZE_MAX);
                ret = true;
            }
            else
            {
                FTD_LOG_ERROR("ch:%d slave prog unack", p_st_slave_info->slave_uart_channel);
                ret = false;
                break;
            }
        }
        else
        {
            FTD_LOG_ERROR("ch:%d read fw fail", p_st_slave_info->slave_uart_channel);
            ret = false;
            break;
        }

        slv_addr += SLAVE_FLASH_PROG_SIZE;
    }

    return ret;
}
#else 
bool ftd_mw_slave_ft_test_manager_program_test(SLAVE_INFO* p_st_slave_info, CHANNEL_BURN_STATUS* st_channel_burn_status)
{
    bool     ret = false;
    uint8_t  read_partition;
    uint32_t fw_read_length;
    uint32_t slv_addr;
    uint32_t fw_size;
    uint8_t  fw_buff[SLAVE_FLASH_PROG_SIZE];
    static uint8_t uart_buff[SLAVE_FLASH_PROG_CMD_SIZE_MAX] = { 0x4a, 0x59, 0x4d, 0x43, 0x54 };
    uint16_t uart_cmd_ttl_len = 0;
    uint16_t wait_time;
    SYS_INFO sys_info;

    // Get chip flash map by chip_code
    ftd_mw_sys_info_manager_get(&sys_info);
    uint16_t chip_code = (uint16_t)sys_info.st_fw_info[st_channel_burn_status->fw_num].chip_id;
    const CHIP_FLASH_ADDR_MAP* p_chip_map = ftd_mw_slave_get_chip_flash_map(chip_code);

    // fw_size = p_st_slave_info->flash_bin_size;
    fw_size = FT_TEST_SLAVE_BIN_SIZE;
    slv_addr = FT_TEST_SLAVE_BIN_ADDR;  // Use mapped address instead of hardcoded macro

    FTD_LOG_TRACE("fw_size:0x%08x last_pkt_valid_data_len:0x%08x", fw_size, (fw_size % SLAVE_FLASH_PROG_SIZE));

    read_partition = PARTITION_FW1_BIN + st_channel_burn_status->fw_num * (PARTITION_FW2_BIN - PARTITION_FW1_BIN);

    for (fw_read_length = 0; fw_read_length < fw_size; fw_read_length += SLAVE_FLASH_PROG_SIZE)
    {
        FTD_LOG_TRACE("read addr:0x%08x len:0x%08x slv_addr:0x%08x ", PARTITION_FW1_BIN, fw_read_length, slv_addr);
        if (ftd_drv_fmc_read_data(FT_TEST_SLAVE_ADDR, fw_read_length, fw_buff, SLAVE_FLASH_PROG_SIZE) == 0)
        {
            // send uart pkt
            // ext cmd
            uart_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET] = 0x00;
            uart_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET + 1] = 0x52;
            // slave flash addr
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET] = (slv_addr >> 24) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 1] = (slv_addr >> 16) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 2] = (slv_addr >> 8) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 3] = slv_addr & 0xFF;


            if (fw_read_length + SLAVE_FLASH_PROG_SIZE > fw_size)
            {
                // last page
                //FTD_LOG_TRACE_BUFF(fw_buff, sizeof(fw_buff), (fw_size % SLAVE_FLASH_PROG_SIZE));

                uint32_t data_len = fw_size % SLAVE_FLASH_PROG_SIZE;
                uint16_t parm_len = EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_OP_LENGTH_SIZE - 3 + data_len; // 4 + 4 + 2 + len

                // calc pram len
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET] = (parm_len >> 8) & 0xFF;
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] = parm_len & 0xFF;

                // copy bin
                memcpy(&uart_buff[SLAVE_FLASH_PROG_DATA_OFFSET - 3], fw_buff, data_len);

                uart_cmd_ttl_len = SLAVE_FLASH_PROG_DATA_OFFSET - 3 + data_len + CHECKSUM_SIZE;
            }
            else
            {
                // calc pram len
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET] = ((SLAVE_FLASH_PROG_PRAM_LEN_MAX - 3) >> 8) & 0xFF;
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] = (SLAVE_FLASH_PROG_PRAM_LEN_MAX - 3) & 0xFF;

                // copy bin
                memcpy(&uart_buff[SLAVE_FLASH_PROG_DATA_OFFSET - 3], fw_buff, SLAVE_FLASH_PROG_SIZE);

                uart_cmd_ttl_len = SLAVE_FLASH_PROG_CMD_SIZE_MAX - 3;
            }

            // add crc 
            uart_buff[uart_cmd_ttl_len - CHECKSUM_SIZE] = ftd_mw_slave_manager_get_crc(uart_buff, (uart_cmd_ttl_len - CHECKSUM_SIZE));

            // for debug only
            // FTD_LOG_DEBUG(" PROG SEND[%d]:", uart_cmd_ttl_len);
            // FTD_LOG_DEBUG_BUFF(uart_buff, uart_cmd_ttl_len, uart_cmd_ttl_len);

            // send uart pkt to slave
            // [Bugfix] The compiler has assigned duplicate addresses to array SLAVE_CHN1_FUFF and uart_buff
            // so need use static array to avoid this issue
            memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
            ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, SLAVE_FLASH_PROG_ACK_LEN);
            ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, uart_buff, uart_cmd_ttl_len);
            FTD_LOG_TRACE("_TX_");
            while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
            FTD_LOG_TRACE("__D__");

            wait_time = 100;
            while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
            {
                CLK_SysTickLongDelay(1000); // 1ms
            }
            ftd_drv_burn_uart_rx_stop(p_st_slave_info->slave_uart_channel);

            //FTD_LOG_DEBUG_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));
            uint8_t ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));
            if (SLAVE_CMD_WRITE_RAM_DATA == ack)
            {
                FTD_LOG_TRACE("ch:%d slave ram prog ack [0x%02x] cmd_len[%d] buf_len[%d]", p_st_slave_info->slave_uart_channel,
                    ack, uart_cmd_ttl_len, SLAVE_FLASH_PROG_CMD_SIZE_MAX - 3);
                ret = true;
            }
            else
            {
                FTD_LOG_ERROR("ch:%d slave ram prog unack", p_st_slave_info->slave_uart_channel);
                ret = false;
                break;
            }
        }
        else
        {
            FTD_LOG_ERROR("ch:%d read fw fail", p_st_slave_info->slave_uart_channel);
            ret = false;
            break;
        }

        slv_addr += SLAVE_FLASH_PROG_SIZE;
    }

    return ret;
}
#endif

// bool ftd_mw_slave_manager_test_current_at_0db(SLAVE_INFO* p_st_slave_info)
// {
//     bool    cmd_ret = false;
//     bool    cur_ret = false;
//     bool    signal_ret = false;
//     bool    signal_RSSI_ret = false;
//     uint32_t packet_address = 0x00000000;
//     int8_t RSSI_value = 0;
//     float cur_value = 0.0f;
//     uint16_t wait_time = 0;
//     uint8_t send_cmd_buff[2] = { 0x5A, 0x2A }; //uart protocol
//     // Use separate buffers for RF and UART to avoid data corruption
//     uint8_t rf_rx_buff[32] = { 0 };  // Buffer for RF UART reception
//     uint8_t uart_rx_buff[32] = { 0 }; // Buffer for burn UART reception
//     // uint8_t rf_send_buff[] = { 0x4A, 0x59, 0x4D, 0x43, 0x54, 0x00, 0x0A, 0x00, 0x88,
//     //                         0x11, 0x22, 0x33, 0x44, 0x03, TX_POWER_0DB, RF_SEND_PACKET_MAX_NUM, FTD_CMD_RECEIVE_PACKET, 0xFF };
//     uint8_t rf_send_buff[] = { 0x4A, 0x59, 0x4D, 0x43, 0x54, 0x00, 0x0A, 0x00, 0x88,
//                                 0xFF, 0xFF, 0xFF, 0xFF, 0x03, TX_POWER_0DB, RF_SEND_PACKET_MAX_NUM, FTD_CMD_RECEIVE_PACKET, 0xFF };
//     packet_address = ftd_utils_slave_read_uid_build_addr();
//     /* 修改地址低4位为通道号 */
//     packet_address &= 0xFFFFFF00;
//     packet_address |= (p_st_slave_info->slave_uart_channel & 0xFF);
//     FTD_LOG_DEBUG("packet_address:0x%08x", packet_address);
//     rf_send_buff[9] = (uint8_t)((packet_address >> 24) & 0xFF);
//     rf_send_buff[10] = (uint8_t)((packet_address >> 16) & 0xFF);
//     rf_send_buff[11] = (uint8_t)((packet_address >> 8) & 0xFF);
//     rf_send_buff[12] = (uint8_t)((packet_address >> 0) & 0xFF);
//     rf_send_buff[sizeof(rf_send_buff) - 1] = ftd_mw_rf_manager_get_crc(rf_send_buff, sizeof(rf_send_buff) - 1);
//     //1.send rf  receives 0 dB broadcast packet signal
//     FTD_LOG_TRACE("tx start");
//     memset(rf_rx_buff, 0x00, sizeof(rf_rx_buff));
//     FTD_LOG_DEBUG_BUFF(rf_send_buff, sizeof(rf_send_buff), sizeof(rf_send_buff));
//     ftd_drv_rf_uart_rx_start(rf_rx_buff, sizeof(rf_send_buff));
//     ftd_drv_rf_uart_tx_start(rf_send_buff, sizeof(rf_send_buff));
//     while (!ftd_drv_rf_uart_is_tx_complete());
//     FTD_LOG_TRACE("rx start");
//     //2. check rf 0 dB broadcast packet signal ack
//     wait_time = 1000;
//     while ((ftd_drv_rf_uart_is_rx_complete() == false) && (wait_time-- > 0))
//     {
//         CLK_SysTickLongDelay(1000);
//     }
//     FTD_LOG_DEBUG_BUFF(rf_rx_buff, sizeof(rf_rx_buff), sizeof(rf_send_buff));
//     FTD_LOG_DEBUG("wait_time:%d", wait_time);
//     uint8_t slave_ack = ftd_mw_rf_check_rx(rf_rx_buff, sizeof(rf_send_buff));
//     FTD_LOG_DEBUG("slave_ack:%d ", slave_ack);
//     if (FTD_RF_EVENT_RECEIVE_PACKET == slave_ack)
//     {
//         FTD_LOG_DEBUG("check signal ack [0x%02x]", slave_ack);
//         signal_ret = true;
//     }
//     else {
//         FTD_LOG_WARN("ch:%d check signal unack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
//         FTD_LOG_DEBUG("Received data when unack:");
//         FTD_LOG_INFO_BUFF(rf_rx_buff, sizeof(rf_rx_buff), sizeof(rf_send_buff));
//         signal_ret = false;
//         ftd_drv_rf_uart_stop();
//         return signal_ret;
//     }
//     memset(rf_rx_buff, 0x00, sizeof(rf_rx_buff));
//     ftd_drv_rf_uart_rx_start(rf_rx_buff, 16);
//     //3. send test current at 0db command
//     FTD_LOG_TRACE("tx start");
//     memset(uart_rx_buff, 0x00, sizeof(uart_rx_buff));
//     ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, uart_rx_buff, sizeof(send_cmd_buff));
//     ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_cmd_buff, sizeof(send_cmd_buff));
//     while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
//     FTD_LOG_TRACE("rx start");
//     //4. check tset current at 0db ack
//     wait_time = 1000;
//     while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
//     {
//         CLK_SysTickLongDelay(1000);
//     }
//     FTD_LOG_DEBUG("rx end, wait_time:%d ", wait_time);
//     FTD_LOG_TRACE_BUFF(uart_rx_buff, sizeof(uart_rx_buff), sizeof(uart_rx_buff));
//     uint32_t start_time = ftd_mw_misc_manager_get_time_ms();
//     slave_ack = ftd_mw_slave_check_rx(uart_rx_buff, sizeof(uart_rx_buff));
//     FTD_LOG_DEBUG("slave_ack:%d ", slave_ack);
//     if (SLAVE_CMD_TEST_CURRENT_AT_0DB == slave_ack)
//     {
//         FTD_LOG_DEBUG("ch:%d check test current at 0db ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
//         cmd_ret = true;
//     }
//     else
//     {
//         FTD_LOG_WARN("ch:%d slave test current at 0db unack", p_st_slave_info->slave_uart_channel);
//         cmd_ret = false;
//         return cmd_ret;
//     }
//     CLK_SysTickLongDelay(20000); // wait for the chip to be ready
//     //5. test the operating current of the chip
//     cur_value = ftd_mw_powermon_manager_get_channel_current(p_st_slave_info->slave_uart_channel);
//     FTD_LOG_INFO("Measured current: %.2f mA", cur_value);
//     // Always return true since we're just measuring
//     cur_ret = true;
//     //6. check the rF signal machine receives the average RSSI value of 20 packets.
//     wait_time = 1000;
//     while ((ftd_drv_rf_uart_is_rx_complete() == false) && (wait_time-- > 0))
//     {
//         CLK_SysTickLongDelay(1000);
//     }
//     FTD_LOG_DEBUG("wait_time:%d", wait_time);
//     FTD_LOG_DEBUG_BUFF(rf_rx_buff, sizeof(rf_rx_buff), BLUETOOTH_RECV_RSSI_ACK_LEN);
//     slave_ack = ftd_mw_rf_check_rx(rf_rx_buff, sizeof(rf_rx_buff));
//     if (FTD_RF_EVENT_RECEIVE_RSSI == slave_ack)
//     {
//         RSSI_value = rf_rx_buff[14];
//         FTD_LOG_DEBUG("check signal RSSI ack [0x%02x], RSSI_value:%d", slave_ack, RSSI_value);
//         signal_RSSI_ret = true;
//     }
//     else {
//         FTD_LOG_WARN("ch:%d check signal RSSI unack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
//         FTD_LOG_DEBUG("Received RSSI data when unack:");
//         FTD_LOG_INFO_BUFF(rf_rx_buff, sizeof(rf_rx_buff), BLUETOOTH_RECV_RSSI_ACK_LEN);
//         signal_RSSI_ret = false;
//         ftd_drv_rf_uart_stop();
//         return signal_RSSI_ret;
//     }
//     // Stop RF UART and switch back to panel mode
//     ftd_drv_rf_uart_stop();
//     // Ensure at least 100ms from packet send to function end
//     uint32_t elapsed_time = ftd_mw_misc_manager_get_time_ms() - start_time;
//     if (elapsed_time < SLAVE_SEND_PACKET_DELAY_MS) {
//         uint32_t delay_time = SLAVE_SEND_PACKET_DELAY_MS - elapsed_time;
//         FTD_LOG_TRACE("Adding delay %dms to reach %dms total", delay_time, SLAVE_SEND_PACKET_DELAY_MS);
//         for (uint32_t i = 0; i < delay_time; i++) {
//             CLK_SysTickLongDelay(1000); // 1ms delay
//         }
//     }
//     FTD_LOG_DEBUG("Total time from packet send to end: %dms", ftd_mw_misc_manager_get_time_ms() - start_time);
//     return cmd_ret && cur_ret && signal_RSSI_ret && signal_ret;
// }

//Test the current at 0 dB
bool ftd_mw_slave_manager_test_current_at_0db(SLAVE_INFO* p_st_slave_info)
{
    bool    cmd_ret = false;
    bool    cur_ret = false;
    bool    signal_ret = false;
    bool    signal_RSSI_ret = false;
    uint32_t packet_address = 0x00000000;

    int8_t RSSI_value = 0;

    float cur_value = 0.0f;
    uint16_t wait_time = 0;
    uint8_t send_cmd_buff[2] = { 0x5A, 0x2A }; //uart protocol
    // Use separate buffers for RF and UART to avoid data corruption
    uint8_t rf_rx_buff[32] = { 0 };  // Buffer for RF UART reception
    uint8_t rf_rx_rssi_buff[32] = { 0 };
    uint8_t uart_rx_buff[32] = { 0 }; // Buffer for burn UART reception

    // uint8_t rf_send_buff[] = { 0x4A, 0x59, 0x4D, 0x43, 0x54, 0x00, 0x0A, 0x00, 0x88,
    //                         0x11, 0x22, 0x33, 0x44, 0x03, TX_POWER_0DB, RF_SEND_PACKET_MAX_NUM, FTD_CMD_RECEIVE_PACKET, 0xFF };
    uint8_t rf_send_buff[] = { 0x4A, 0x59, 0x4D, 0x43, 0x54, 0x00, 0x0A, 0x00, 0x88,
                                0xFF, 0xFF, 0xFF, 0xFF, 0x03, TX_POWER_0DB, RF_SEND_PACKET_MAX_NUM, FTD_CMD_RECEIVE_PACKET, 0xFF };
    packet_address = ftd_utils_slave_read_uid_build_addr();
    /* 修改地址低4位为通道号 */
    packet_address &= 0xFFFFFF00;
    packet_address |= (p_st_slave_info->slave_uart_channel & 0xFF);
    FTD_LOG_DEBUG("packet_address:0x%08x", packet_address);

    rf_send_buff[9] = (uint8_t)((packet_address >> 24) & 0xFF);
    rf_send_buff[10] = (uint8_t)((packet_address >> 16) & 0xFF);
    rf_send_buff[11] = (uint8_t)((packet_address >> 8) & 0xFF);
    rf_send_buff[12] = (uint8_t)((packet_address >> 0) & 0xFF);

    rf_send_buff[sizeof(rf_send_buff) - 1] = ftd_mw_rf_manager_get_crc(rf_send_buff, sizeof(rf_send_buff) - 1);

    //1.send rf  receives 0 dB broadcast packet signal
    FTD_LOG_TRACE("tx start");
    memset(rf_rx_buff, 0x00, sizeof(rf_rx_buff));
    FTD_LOG_DEBUG_BUFF(rf_send_buff, sizeof(rf_send_buff), sizeof(rf_send_buff));
    ftd_drv_rf_uart_rx_start(rf_rx_buff, sizeof(rf_send_buff));
    ftd_drv_rf_uart_tx_start(rf_send_buff, sizeof(rf_send_buff));
    while (!ftd_drv_rf_uart_is_tx_complete());


    FTD_LOG_TRACE("rx start");
    //2. check rf 0 dB broadcast packet signal ack
    wait_time = 1000;
    while ((ftd_drv_rf_uart_is_rx_complete() == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }



    FTD_LOG_DEBUG_BUFF(rf_rx_buff, sizeof(rf_rx_buff), sizeof(rf_send_buff));
    FTD_LOG_DEBUG("wait_time:%d", wait_time);
    uint8_t slave_ack = ftd_mw_rf_check_rx(rf_rx_buff, sizeof(rf_send_buff));
    FTD_LOG_DEBUG("slave_ack:%d ", slave_ack);
    if (FTD_RF_EVENT_RECEIVE_PACKET == slave_ack)
    {
        FTD_LOG_DEBUG("check signal ack [0x%02x]", slave_ack);
        signal_ret = true;
    }
    else {
        FTD_LOG_WARN("ch:%d check signal unack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
        FTD_LOG_DEBUG("Received data when unack:");
        FTD_LOG_INFO_BUFF(rf_rx_buff, sizeof(rf_rx_buff), sizeof(rf_send_buff));
        signal_ret = false;
        ftd_drv_rf_uart_stop();
        return signal_ret;
    }

    ftd_drv_rf_uart_rx_start(rf_rx_rssi_buff, 16);

    //3. send test current at 0db command
    FTD_LOG_TRACE("tx start");
    memset(uart_rx_buff, 0x00, sizeof(uart_rx_buff));
    ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, uart_rx_buff, sizeof(send_cmd_buff));
    ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_cmd_buff, sizeof(send_cmd_buff));
    while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
    FTD_LOG_TRACE("rx start");

    //4. check tset current at 0db ack
    wait_time = 1000;
    while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }

    FTD_LOG_DEBUG("rx end, wait_time:%d ", wait_time);

    FTD_LOG_TRACE_BUFF(uart_rx_buff, sizeof(uart_rx_buff), sizeof(uart_rx_buff));
    uint32_t start_time = ftd_mw_misc_manager_get_time_ms();
    slave_ack = ftd_mw_slave_check_rx(uart_rx_buff, sizeof(uart_rx_buff));
    FTD_LOG_DEBUG("slave_ack:%d ", slave_ack);
    if (SLAVE_CMD_TEST_CURRENT_AT_0DB == slave_ack)
    {
        FTD_LOG_DEBUG("ch:%d check test current at 0db ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
        cmd_ret = true;
    }
    else
    {
        FTD_LOG_WARN("ch:%d slave test current at 0db unack", p_st_slave_info->slave_uart_channel);
        cmd_ret = false;
        return cmd_ret;
    }

    CLK_SysTickLongDelay(20000); // wait for the chip to be ready

    //5. test the operating current of the chip
    cur_value = ftd_mw_powermon_manager_get_channel_current(p_st_slave_info->slave_uart_channel);
    FTD_LOG_INFO("Measured current: %.2f mA", cur_value);
    // Always return true since we're just measuring
    cur_ret = true;

    //6. check the rF signal machine receives the average RSSI value of 20 packets.
    wait_time = 1000;
    while ((ftd_drv_rf_uart_is_rx_complete() == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }
    FTD_LOG_DEBUG("wait_time:%d", wait_time);
    FTD_LOG_DEBUG_BUFF(rf_rx_rssi_buff, sizeof(rf_rx_rssi_buff), BLUETOOTH_RECV_RSSI_ACK_LEN);
    slave_ack = ftd_mw_rf_check_rx(rf_rx_rssi_buff, sizeof(rf_rx_rssi_buff));
    if (FTD_RF_EVENT_RECEIVE_RSSI == slave_ack)
    {
        RSSI_value = rf_rx_rssi_buff[14];
        FTD_LOG_INFO("check signal RSSI ack [0x%02x], RSSI_value:%d", slave_ack, RSSI_value);
        signal_RSSI_ret = true;
    }
    else {
        FTD_LOG_WARN("ch:%d check signal RSSI unack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
        FTD_LOG_DEBUG("Received RSSI data when unack:");
        FTD_LOG_INFO_BUFF(rf_rx_rssi_buff, sizeof(rf_rx_rssi_buff), BLUETOOTH_RECV_RSSI_ACK_LEN);
        signal_RSSI_ret = false;
        ftd_drv_rf_uart_stop();
        return signal_RSSI_ret;
    }

    // Stop RF UART and switch back to panel mode
    ftd_drv_rf_uart_stop();

    // Ensure at least 100ms from packet send to function end
    uint32_t elapsed_time = ftd_mw_misc_manager_get_time_ms() - start_time;
    if (elapsed_time < SLAVE_SEND_PACKET_DELAY_MS) {
        uint32_t delay_time = SLAVE_SEND_PACKET_DELAY_MS - elapsed_time;
        FTD_LOG_TRACE("Adding delay %dms to reach %dms total", delay_time, SLAVE_SEND_PACKET_DELAY_MS);
        for (uint32_t i = 0; i < delay_time; i++) {
            CLK_SysTickLongDelay(1000); // 1ms delay
        }
    }
    FTD_LOG_DEBUG("Total time from packet send to end: %dms", ftd_mw_misc_manager_get_time_ms() - start_time);


    return cmd_ret && cur_ret && signal_RSSI_ret && signal_ret;
}


//Test the current at 5 dB
bool ftd_mw_slave_manager_test_current_at_5db(SLAVE_INFO* p_st_slave_info)
{
    bool    cmd_ret = false;
    bool    cur_ret = false;
    bool    signal_ret = false;
    bool    signal_RSSI_ret = false;
    uint32_t packet_address = 0x00000000;

    int8_t RSSI_value = 0;

    float cur_value = 0.0f;
    uint16_t wait_time = 0;
    uint8_t send_cmd_buff[2] = { 0x5A, 0x2B }; //uart protocol
    // Use separate buffers for RF and UART to avoid data corruption
    uint8_t rf_rx_buff[32] = { 0 };   // Buffer for RF UART reception
    uint8_t uart_rx_buff[32] = { 0 }; // Buffer for burn UART reception
    // uint8_t rf_send_buff[] = { 0x4A, 0x59, 0x4D, 0x43, 0x54, 0x00, 0x0A, 0x00, 0x88,
    //                             0x11, 0x22, 0x33, 0x44, 0x03, TX_POWER_5DB, RF_SEND_PACKET_MAX_NUM, FTD_CMD_RECEIVE_PACKET, 0xFF };

    uint8_t rf_send_buff[] = { 0x4A, 0x59, 0x4D, 0x43, 0x54, 0x00, 0x0A, 0x00, 0x88,
                            0xFF, 0xFF, 0xFF, 0xFF, 0x03, TX_POWER_5DB, RF_SEND_PACKET_MAX_NUM, FTD_CMD_RECEIVE_PACKET, 0xFF };
    packet_address = ftd_utils_slave_read_uid_build_addr();
    /* 修改地址低4位为通道号 */
    packet_address &= 0xFFFFFF00;
    packet_address |= (p_st_slave_info->slave_uart_channel & 0xFF);

    rf_send_buff[9] = (uint8_t)((packet_address >> 24) & 0xFF);
    rf_send_buff[10] = (uint8_t)((packet_address >> 16) & 0xFF);
    rf_send_buff[11] = (uint8_t)((packet_address >> 8) & 0xFF);
    rf_send_buff[12] = (uint8_t)((packet_address >> 0) & 0xFF);
    rf_send_buff[sizeof(rf_send_buff) - 1] = ftd_mw_rf_manager_get_crc(rf_send_buff, sizeof(rf_send_buff) - 1);

    //1.send rf  receives 5 dB broadcast packet signal
    FTD_LOG_TRACE("tx start");
    memset(rf_rx_buff, 0x00, sizeof(rf_rx_buff));
    ftd_drv_rf_uart_rx_start(rf_rx_buff, sizeof(rf_send_buff));
    ftd_drv_rf_uart_tx_start(rf_send_buff, sizeof(rf_send_buff));
    while (!ftd_drv_rf_uart_is_tx_complete());

    FTD_LOG_TRACE("rx start");

    //2. check rf 5 dB broadcast packet signal ack
    wait_time = 1000;
    while ((ftd_drv_rf_uart_is_rx_complete() == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }
    FTD_LOG_DEBUG_BUFF(rf_rx_buff, sizeof(rf_rx_buff), sizeof(rf_send_buff));
    FTD_LOG_DEBUG("wait_time:%d", wait_time);
    uint8_t slave_ack = ftd_mw_rf_check_rx(rf_rx_buff, sizeof(rf_send_buff));
    FTD_LOG_DEBUG("slave_ack:%d ", slave_ack);
    if (FTD_RF_EVENT_RECEIVE_PACKET == slave_ack)
    {
        FTD_LOG_DEBUG("check signal ack [0x%02x]", slave_ack);
        signal_ret = true;
    }
    else {
        FTD_LOG_WARN("ch:%d check signal unack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
        FTD_LOG_DEBUG("Received data when unack:");
        FTD_LOG_INFO_BUFF(rf_rx_buff, sizeof(rf_rx_buff), sizeof(rf_send_buff));
        signal_ret = false;
        ftd_drv_rf_uart_stop();
        return signal_ret;
    }

    memset(rf_rx_buff, 0x00, sizeof(rf_rx_buff));
    ftd_drv_rf_uart_rx_start(rf_rx_buff, 16);

    //3. send test current at 5db command
    FTD_LOG_TRACE("tx start");
    memset(uart_rx_buff, 0x00, sizeof(uart_rx_buff));
    ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, uart_rx_buff, sizeof(send_cmd_buff));
    ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_cmd_buff, sizeof(send_cmd_buff));
    while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
    FTD_LOG_TRACE("rx start");

    //4. check tset current at 5db ack
    wait_time = 1000;
    while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }
    FTD_LOG_DEBUG("rx end, wait_time:%d ", wait_time);
    FTD_LOG_TRACE_BUFF(uart_rx_buff, sizeof(uart_rx_buff), sizeof(send_cmd_buff));

    uint32_t start_time = ftd_mw_misc_manager_get_time_ms();
    slave_ack = ftd_mw_slave_check_rx(uart_rx_buff, sizeof(uart_rx_buff));
    FTD_LOG_DEBUG("slave_ack:%d ", slave_ack);
    if (SLAVE_CMD_TEST_CURRENT_AT_5DB == slave_ack)
    {
        FTD_LOG_DEBUG("ch:%d check test current at 5db ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
        cmd_ret = true;
    }
    else
    {
        FTD_LOG_WARN("ch:%d slave test current at 5db unack", p_st_slave_info->slave_uart_channel);
        cmd_ret = false;
        return cmd_ret;
    }

    CLK_SysTickLongDelay(20000); // wait for the chip to be ready

    //5. test the operating current of the chip
    cur_value = ftd_mw_powermon_manager_get_channel_current(p_st_slave_info->slave_uart_channel);
    FTD_LOG_INFO("Measured current: %.2f mA", cur_value);
    // Always return true since we're just measuring
    cur_ret = true;

    //6. check the rF signal machine receives the average RSSI value of 20 packets.
    wait_time = 1000;
    while ((ftd_drv_rf_uart_is_rx_complete() == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }
    FTD_LOG_DEBUG("wait_time:%d", wait_time);
    FTD_LOG_DEBUG_BUFF(rf_rx_buff, sizeof(rf_rx_buff), BLUETOOTH_RECV_RSSI_ACK_LEN);
    slave_ack = ftd_mw_rf_check_rx(rf_rx_buff, sizeof(rf_rx_buff));
    if (FTD_RF_EVENT_RECEIVE_RSSI == slave_ack)
    {
        RSSI_value = rf_rx_buff[14]; //RSSI value is in the 14th byte
        FTD_LOG_DEBUG("check signal RSSI ack [0x%02x], RSSI_value:%d", slave_ack, RSSI_value);
        signal_RSSI_ret = true;
    }
    else {
        FTD_LOG_WARN("ch:%d check signal RSSI unack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
        FTD_LOG_DEBUG("Received RSSI data when unack:");
        FTD_LOG_INFO_BUFF(rf_rx_buff, sizeof(rf_rx_buff), BLUETOOTH_RECV_RSSI_ACK_LEN);
        signal_RSSI_ret = false;
        ftd_drv_rf_uart_stop();
        return signal_RSSI_ret;
    }

    // Stop RF UART and switch back to panel mode
    ftd_drv_rf_uart_stop();

    // Ensure at least 100ms from packet send to function end
    uint32_t elapsed_time = ftd_mw_misc_manager_get_time_ms() - start_time;
    if (elapsed_time < SLAVE_SEND_PACKET_DELAY_MS) {
        uint32_t delay_time = SLAVE_SEND_PACKET_DELAY_MS - elapsed_time;
        FTD_LOG_TRACE("Adding delay %dms to reach %dms total", delay_time, SLAVE_SEND_PACKET_DELAY_MS);
        for (uint32_t i = 0; i < delay_time; i++) {
            CLK_SysTickLongDelay(1000); // 1ms delay
        }
    }
    FTD_LOG_DEBUG("Total time from packet send to end: %dms", ftd_mw_misc_manager_get_time_ms() - start_time);


    return cmd_ret && cur_ret && signal_RSSI_ret && signal_ret;

}



//Test the static operating current
bool ftd_mw_slave_manager_test_current_at_static(SLAVE_INFO* p_st_slave_info)
{
    return ftd_mw_slave_manager_start_calib_test(p_st_slave_info, TEST_TYPE_STATIC_CURRENT);
}

/**
 * @brief Test static current (non-blocking state machine version)
 * @param p_st_slave_info Pointer to slave info
 * @return true if operation completed, false if still in progress
 */
bool ftd_mw_slave_manager_test_current_at_static_sm(SLAVE_INFO* p_st_slave_info)
{
    return ftd_mw_slave_manager_start_calib_test(p_st_slave_info, TEST_TYPE_STATIC_CURRENT);
}

/**
 * @brief Calibrate MADC (non-blocking state machine version)
 * @param p_st_slave_info Pointer to slave info
 * @return true if operation completed, false if still in progress
 */
bool ftd_mw_slave_manager_calibrate_madc_sm(SLAVE_INFO* p_st_slave_info)
{
    return ftd_mw_slave_manager_start_calib_test(p_st_slave_info, CALIB_TYPE_MADC);
}

/**
 * @brief Calibrate GPADC (non-blocking state machine version)
 * @param p_st_slave_info Pointer to slave info
 * @return true if operation completed, false if still in progress
 */
bool ftd_mw_slave_manager_calibrate_gpadc_sm(SLAVE_INFO* p_st_slave_info)
{
    return ftd_mw_slave_manager_start_calib_test(p_st_slave_info, CALIB_TYPE_GPADC);
}

/**
 * @brief Calibrate clock (non-blocking state machine version)
 * @param p_st_slave_info Pointer to slave info
 * @return true if operation completed, false if still in progress
 */
bool ftd_mw_slave_manager_calibrate_clock_sm(SLAVE_INFO* p_st_slave_info)
{
    return ftd_mw_slave_manager_start_calib_test(p_st_slave_info, CALIB_TYPE_CLOCK);
}

/**
 * @brief Test sleep current (non-blocking state machine version)
 * @param p_st_slave_info Pointer to slave info
 * @return true if operation completed, false if still in progress
 */
bool ftd_mw_slave_manager_test_sleep_current_sm(SLAVE_INFO* p_st_slave_info)
{
    return ftd_mw_slave_manager_start_calib_test(p_st_slave_info, TEST_TYPE_SLEEP_CURRENT);
}

/**
 * @brief Check if test state machine has error
 * @param channel Channel number
 * @return true if error, false otherwise
 */
bool ftd_mw_slave_manager_test_sm_is_error(uint8_t channel)
{
    return ftd_mw_slave_manager_calib_test_is_error(channel);
}

// Test the receiving sensitivity
bool ftd_mw_slave_manager_test_receiving_sensitivity(SLAVE_INFO* p_st_slave_info)
{
    bool    cmd_ret = false;
    bool    cur_ret = false;
    bool    RSSI_ret = false;
    uint32_t RSSI_value = 0;
    float cur_value = 0.0f;
    bool signal_ret = false;
    uint32_t packet_address = 0x00000000;
    uint16_t wait_time = 0;
    uint8_t send_cmd_buff[2] = { 0x5A, 0x2D }; //uart protocol
    // Use separate buffers for RF and UART to avoid data corruption
    uint8_t uart_rx_buff[32] = { 0 }; // Buffer for burn UART reception
    uint8_t rf_rx_buff[32] = { 0 };   // Buffer for RF UART reception
    // uint8_t rf_send_buff[] = { 0x4A, 0x59, 0x4D, 0x43, 0x54, 0x00, 0x0A, 0x00, 0x88,
    //                             0x11, 0x22, 0x33, 0x44, 0x03, TX_POWER_5DB, 0x20, FTD_CMD_SEND_PACKET, 0xFF };



    uint8_t rf_send_buff[] = { 0x4A, 0x59, 0x4D, 0x43, 0x54, 0x00, 0x0A, 0x00, 0x88,
                                0xFF, 0xFF, 0xFF, 0xFF, 0x03, TX_POWER_5DB, RF_SEND_PACKET_MAX_NUM, FTD_CMD_SEND_PACKET, 0xFF };
    packet_address = ftd_utils_slave_read_uid_build_addr();
    /* 修改地址低4位为通道号 */
    packet_address &= 0xFFFFFF00;
    packet_address |= (p_st_slave_info->slave_uart_channel & 0xFF);

    rf_send_buff[9] = (uint8_t)((packet_address >> 24) & 0xFF);
    rf_send_buff[10] = (uint8_t)((packet_address >> 16) & 0xFF);
    rf_send_buff[11] = (uint8_t)((packet_address >> 8) & 0xFF);
    rf_send_buff[12] = (uint8_t)((packet_address >> 0) & 0xFF);
    rf_send_buff[sizeof(rf_send_buff) - 1] = ftd_mw_rf_manager_get_crc(rf_send_buff, sizeof(rf_send_buff) - 1);

    //1. send test receiving sensitivity command
    FTD_LOG_TRACE("tx start");
    memset(uart_rx_buff, 0x00, sizeof(uart_rx_buff));
    ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, uart_rx_buff, sizeof(send_cmd_buff));
    ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_cmd_buff, sizeof(send_cmd_buff));
    while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
    FTD_LOG_TRACE("rx start");

    //4. check tset current at 5db ack
    wait_time = 1000;
    while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }
    FTD_LOG_DEBUG("rx end, wait_time:%d ", wait_time);
    FTD_LOG_DEBUG_BUFF(uart_rx_buff, sizeof(uart_rx_buff), sizeof(uart_rx_buff));
    uint8_t slave_ack = ftd_mw_slave_check_rx(uart_rx_buff, sizeof(uart_rx_buff));
    if (SLAVE_CMD_TEST_RECEIVE_SENSITIVITY == slave_ack)
    {
        FTD_LOG_DEBUG("ch:%d check test receiving sensitivity ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
        cmd_ret = true;
    }
    else
    {
        FTD_LOG_WARN("ch:%d slave test receiving sensitivity unack", p_st_slave_info->slave_uart_channel);
        cmd_ret = false;
        return cmd_ret;
    }


    memset(uart_rx_buff, 0x00, sizeof(uart_rx_buff));
    ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, uart_rx_buff, 4);

    //3.send rf  send receiving sensitivity signal
    FTD_LOG_TRACE("tx start");
    memset(rf_rx_buff, 0x00, sizeof(rf_rx_buff));
    ftd_drv_rf_uart_rx_start(rf_rx_buff, sizeof(rf_send_buff));
    ftd_drv_rf_uart_tx_start(rf_send_buff, sizeof(rf_send_buff));

    while (!ftd_drv_rf_uart_is_tx_complete());
    FTD_LOG_TRACE("rx start");

    //4. check rf 5 dB broadcast packet signal ack
    wait_time = 1000;
    while ((ftd_drv_rf_uart_is_rx_complete() == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }
    ftd_drv_rf_uart_stop();
    FTD_LOG_TRACE_BUFF(rf_rx_buff, sizeof(rf_rx_buff), sizeof(rf_send_buff));
    FTD_LOG_DEBUG("wait_time:%d", wait_time);
    slave_ack = ftd_mw_rf_check_rx(rf_rx_buff, sizeof(rf_send_buff));
    FTD_LOG_DEBUG("slave_ack:%d ", slave_ack);
    if (FTD_RF_EVENT_SEND_PACKET == slave_ack)
    {
        FTD_LOG_DEBUG("check signal ack [0x%02x]", slave_ack);
        signal_ret = true;
    }
    else {
        FTD_LOG_WARN("ch:%d check signal unack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
        FTD_LOG_DEBUG("Received data when unack:");
        FTD_LOG_INFO_BUFF(rf_rx_buff, sizeof(rf_rx_buff), sizeof(rf_send_buff));
        signal_ret = false;
        return signal_ret;
    }



    //5. test the operating current of the chip
    cur_value = ftd_mw_powermon_manager_get_channel_current(p_st_slave_info->slave_uart_channel);
    FTD_LOG_INFO("Measured current: %.2f mA", cur_value);
    // Always return true since we're just measuring
    cur_ret = true;

    //6. check the chip receives the average RSSI value of 20 packets.
    wait_time = 1000;
    while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }
    FTD_LOG_DEBUG("rx end, wait_time:%d ", wait_time);
    FTD_LOG_TRACE_BUFF(uart_rx_buff, sizeof(uart_rx_buff), sizeof(uart_rx_buff));
    slave_ack = ftd_mw_slave_check_rx(uart_rx_buff, sizeof(uart_rx_buff));
    if (SLAVE_CMD_TEST_RECEIVE_SENSITIVITY_VALUE == slave_ack)
    {
        uint8_t param_len = uart_rx_buff[2];
        if (param_len == 1)
            RSSI_value = (uint32_t)uart_rx_buff[3];
        else if (param_len == 2)
            RSSI_value = (uint32_t)uart_rx_buff[3] << 8 | (uint32_t)uart_rx_buff[4];
        else if (param_len == 3)
            RSSI_value = (uint32_t)uart_rx_buff[3] << 16 | (uint32_t)uart_rx_buff[4] << 8 | (uint32_t)uart_rx_buff[5];
        else if (param_len == 4)
            RSSI_value = (uint32_t)uart_rx_buff[3] << 24 | (uint32_t)uart_rx_buff[4] << 16 | (uint32_t)uart_rx_buff[5] << 8 | (uint32_t)uart_rx_buff[6];

        FTD_LOG_DEBUG("ch:%d check test receiving sensitivity value ack [0x%02x], parm_len :%d,RSSI_value [0x%04x]",
            p_st_slave_info->slave_uart_channel, slave_ack, param_len, RSSI_value);
        cmd_ret = true;
    }
    else
    {
        FTD_LOG_WARN("ch:%d slave test receiving sensitivity value unack", p_st_slave_info->slave_uart_channel);
        cmd_ret = false;
        return cmd_ret;
    }
    // if (RSSI_value != 0xFF)
    // {
    //     RSSI_ret = true;
    // }
    // else
    // {
    //     RSSI_ret = false;
    // }
    RSSI_ret = true;
    return signal_ret && cmd_ret && cur_ret && RSSI_ret;

}


// //Test the receiving sensitivity
// bool ftd_mw_slave_manager_test_receiving_sensitivity(SLAVE_INFO* p_st_slave_info)
// {
//     bool    cmd_ret = false;
//     bool    cur_ret = false;
//     bool    RSSI_ret = false;
//     uint32_t RSSI_value = 0;
//     float cur_value = 0.0f;
//     bool signal_ret = false;
//     uint32_t packet_address = 0x00000000;
//     uint16_t wait_time = 0;
//     uint8_t send_cmd_buff[2] = { 0x5A, 0x2D }; //uart protocol
//     // uint8_t rf_send_buff[] = { 0x4A, 0x59, 0x4D, 0x43, 0x54, 0x00, 0x0A, 0x00, 0x88,
//     //                             0x11, 0x22, 0x33, 0x44, 0x03, TX_POWER_5DB, 0x20, FTD_CMD_SEND_PACKET, 0xFF };
//     uint8_t rf_send_buff[] = { 0x4A, 0x59, 0x4D, 0x43, 0x54, 0x00, 0x0A, 0x00, 0x88,
//                                 0xFF, 0xFF, 0xFF, 0xFF, 0x03, TX_POWER_5DB, RF_SEND_PACKET_MAX_NUM, FTD_CMD_SEND_PACKET, 0xFF };
//     packet_address = ftd_utils_slave_read_uid_build_addr();
//     /* 修改地址最后一位为通道号 */
//     packet_address &= 0xFFFFFFF0;
//     packet_address |= (p_st_slave_info->slave_uart_channel & 0x0F);
//     rf_send_buff[9] = (uint8_t)((packet_address >> 24) & 0xFF);
//     rf_send_buff[10] = (uint8_t)((packet_address >> 16) & 0xFF);
//     rf_send_buff[11] = (uint8_t)((packet_address >> 8) & 0xFF);
//     rf_send_buff[12] = (uint8_t)((packet_address >> 0) & 0xFF);
//     rf_send_buff[sizeof(rf_send_buff) - 1] = ftd_mw_rf_manager_get_crc(rf_send_buff, sizeof(rf_send_buff) - 1);
//     //1.power on
//     ftd_mw_slave_manager_power_on(p_st_slave_info->slave_uart_channel);
//     //2.send rf  send receiving sensitivity signal
//     FTD_LOG_TRACE("tx start");
//     memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
//     ftd_drv_rf_uart_rx_start(SLAVE_CHN1_FUFF, sizeof(rf_send_buff));
//     ftd_drv_rf_uart_tx_start(rf_send_buff, sizeof(rf_send_buff));
//     while (!ftd_drv_rf_uart_is_tx_complete());
//     FTD_LOG_TRACE("rx start");
//     //3. check rf 5 dB broadcast packet signal ack
//     wait_time = 30;
//     while ((ftd_drv_rf_uart_is_rx_complete() == false) && (wait_time-- > 0))
//     {
//         CLK_SysTickLongDelay(1000);
//     }
//     ftd_drv_rf_uart_stop();
//     FTD_LOG_DEBUG_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(rf_send_buff));
//     FTD_LOG_DEBUG("wait_time:%d", wait_time);
//     uint8_t slave_ack = ftd_mw_rf_check_rx(SLAVE_CHN1_FUFF, sizeof(rf_send_buff));
//     FTD_LOG_DEBUG("slave_ack:%d ", slave_ack);
//     if (FTD_RF_EVENT_SEND_PACKET == slave_ack)
//     {
//         FTD_LOG_DEBUG("check signal ack [0x%02x]", slave_ack);
//         signal_ret = true;
//     }
//     else {
//         FTD_LOG_DEBUG("check signal unack [0x%02x]", slave_ack);
//         signal_ret = false;
//         return signal_ret;
//     }
//     uint16_t start_time = ftd_mw_misc_manager_get_time_ms();
//     FTD_LOG_DEBUG("start_time: %d", start_time);
//     //4. send test receiving sensitivity command
//     FTD_LOG_TRACE("tx start");
//     memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
//     ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, sizeof(send_cmd_buff));
//     ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_cmd_buff, sizeof(send_cmd_buff));
//     while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
//     FTD_LOG_TRACE("rx start");
//         start_time = ftd_mw_misc_manager_get_time_ms();
//         FTD_LOG_DEBUG("start_time: %d", start_time);
//     //5. check test receiving sensitivity ack
//     wait_time = 30;
//     while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
//     {
//         CLK_SysTickLongDelay(1000);
//     }
//     FTD_LOG_DEBUG("rx end, wait_time:%d ", wait_time);
//     FTD_LOG_TRACE_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));
//     slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));
//     if (SLAVE_CMD_TEST_RECEIVE_SENSITIVITY == slave_ack)
//     {
//         FTD_LOG_DEBUG("ch:%d check test receiving sensitivity ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
//         cmd_ret = true;
//     }
//     else
//     {
//         FTD_LOG_DEBUG("ch:%d slave test receiving sensitivity unack", p_st_slave_info->slave_uart_channel);
//         cmd_ret = false;
//         return cmd_ret;
//     }
//     memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
//     ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, 4);
//     //6. test the operating current of the chip
//     cur_value = ftd_mw_powermon_manager_get_channel_current(p_st_slave_info->slave_uart_channel);
//     FTD_LOG_INFO("Measured current: %.2f mA", cur_value);
//     // Always return true since we're just measuring
//     cur_ret = true;
//     //7. check the chip receives the average RSSI value of 20 packets.
//     wait_time = 300;
//     while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
//     {
//         CLK_SysTickLongDelay(1000);
//     }
//     start_time = ftd_mw_misc_manager_get_time_ms();
//     FTD_LOG_DEBUG("start_time: %d", start_time);
//     FTD_LOG_DEBUG("rx end, wait_time:%d ", wait_time);
//     FTD_LOG_TRACE_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));
//     slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));
//     if (SLAVE_CMD_TEST_RECEIVE_SENSITIVITY_VALUE == slave_ack)
//     {
//         uint8_t param_len = SLAVE_CHN1_FUFF[2];
//         if (param_len == 1)
//             RSSI_value = (uint32_t)SLAVE_CHN1_FUFF[3];
//         else if (param_len == 2)
//             RSSI_value = (uint32_t)SLAVE_CHN1_FUFF[3] << 8 | (uint32_t)SLAVE_CHN1_FUFF[4];
//         else if (param_len == 3)
//             RSSI_value = (uint32_t)SLAVE_CHN1_FUFF[3] << 16 | (uint32_t)SLAVE_CHN1_FUFF[4] << 8 | (uint32_t)SLAVE_CHN1_FUFF[5];
//         else if (param_len == 4)
//             RSSI_value = (uint32_t)SLAVE_CHN1_FUFF[3] << 24 | (uint32_t)SLAVE_CHN1_FUFF[4] << 16 | (uint32_t)SLAVE_CHN1_FUFF[5] << 8 | (uint32_t)SLAVE_CHN1_FUFF[6];
//         FTD_LOG_DEBUG("ch:%d check test receiving sensitivity value ack [0x%02x], parm_len :%d,RSSI_value [0x%04x]",
//             p_st_slave_info->slave_uart_channel, slave_ack, param_len, RSSI_value);
//         cmd_ret = true;
//     }
//     else
//     {
//         FTD_LOG_DEBUG("ch:%d slave test receiving sensitivity value unack", p_st_slave_info->slave_uart_channel);
//         cmd_ret = false;
//         return cmd_ret;
//     }
//     if (RSSI_value != 0xFF)
//     {
//         RSSI_ret = true;
//     }
//     else
//     {
//         RSSI_ret = false;
//     }
//     return signal_ret && cmd_ret && cur_ret && RSSI_ret;
// }

//Calibrate the MADC
bool ftd_mw_slave_manager_calibrate_madc(SLAVE_INFO* p_st_slave_info)
{
    bool    cmd_ret = false;
    bool    value_ret = false;
    uint32_t madc_value = 0;

    uint16_t wait_time = 0;
    uint8_t send_cmd_buff[2] = { 0x5A, 0x2E }; //uart protocol


    //1. send test calibrate madc command
    FTD_LOG_TRACE("tx start");
    memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
    ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, sizeof(send_cmd_buff));
    ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_cmd_buff, sizeof(send_cmd_buff));
    while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
    FTD_LOG_TRACE("rx start");

    //2. check calibrate madc ack
    wait_time = 1000;
    while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }

    FTD_LOG_DEBUG("rx end, wait_time:%d ", wait_time);
    FTD_LOG_TRACE_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));
    uint8_t slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));
    if (SLAVE_CMD_CALIBRATE_MADC == slave_ack)
    {
        FTD_LOG_DEBUG("ch:%d check calibrate madc ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
        cmd_ret = true;
    }
    else
    {
        FTD_LOG_WARN("ch:%d slave calibrate madc unack", p_st_slave_info->slave_uart_channel);
        cmd_ret = false;
        return cmd_ret;
    }


    //3. check the chip receives madc value
    memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
    ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, 6);
    wait_time = 3000;
    while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }
    FTD_LOG_DEBUG("rx end, wait_time:%d ", wait_time);
    FTD_LOG_TRACE_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));
    slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));
    if (SLAVE_CMD_CALIBRATE_MADC_VALUE == slave_ack)
    {
        uint8_t param_len = SLAVE_CHN1_FUFF[2];
        if (param_len == 1)
            madc_value = SLAVE_CHN1_FUFF[3];
        else if (param_len == 2)
            madc_value = SLAVE_CHN1_FUFF[3] << 8 | SLAVE_CHN1_FUFF[4];
        else if (param_len == 3)
            madc_value = SLAVE_CHN1_FUFF[3] << 16 | SLAVE_CHN1_FUFF[4] << 8 | SLAVE_CHN1_FUFF[5];
        else if (param_len == 4)
            madc_value = SLAVE_CHN1_FUFF[3] << 24 | SLAVE_CHN1_FUFF[4] << 16 | SLAVE_CHN1_FUFF[5] << 8 | SLAVE_CHN1_FUFF[6];

        FTD_LOG_DEBUG("ch:%d calibrate madc ack [0x%02x],parm_len :%d, value [0x%08x]",
            p_st_slave_info->slave_uart_channel, slave_ack, param_len, madc_value);
        cmd_ret = true;
        CLK_SysTickLongDelay(1000);
    }
    else
    {
        FTD_LOG_WARN("ch:%d slave calibrate madc value unack", p_st_slave_info->slave_uart_channel);
        cmd_ret = false;
        return cmd_ret;
    }

    // if (madc_value != 0)
    value_ret = true;

    return cmd_ret && value_ret;

}


// //Calibrate the MADC
// bool ftd_mw_slave_manager_calibrate_madc(SLAVE_INFO* p_st_slave_info)
// {
//     bool    cmd_ret = false;
//     bool    value_ret = false;
//     uint32_t madc_value = 0;
//     uint16_t wait_time = 0;
//     uint8_t send_cmd_buff[2] = { 0x5A, 0x2E }; //uart protocol
//     uint8_t rx_buff[32] = { 0 };
//     //1. send test calibrate madc command
//     FTD_LOG_TRACE("tx start");
//     memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
//      (p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, sizeof(send_cmd_buff));
//     ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_cmd_buff, sizeof(send_cmd_buff));
//     while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
//     FTD_LOG_TRACE("rx start");
//     //2. check calibrate madc ack
//     wait_time = 1000;
//     while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
//     {
//         CLK_SysTickLongDelay(1000);
//     }
//     memset(rx_buff, 0x00, sizeof(rx_buff));
//     ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, rx_buff, 6);
//     FTD_LOG_DEBUG("rx end, wait_time:%d ", wait_time);
//     FTD_LOG_DEBUG_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), 2);
//     uint8_t slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));
//     if (SLAVE_CMD_CALIBRATE_MADC == slave_ack)
//     {
//         FTD_LOG_DEBUG("ch:%d check calibrate madc ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
//         cmd_ret = true;
//     }
//     else
//     {
//         FTD_LOG_WARN("ch:%d slave calibrate madc unack", p_st_slave_info->slave_uart_channel);
//         cmd_ret = false;
//         return cmd_ret;
//     }
//     //3. check the chip receives madc value
//     wait_time = 10000;
//     while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
//     {
//         CLK_SysTickLongDelay(1000);
//     }
//     FTD_LOG_DEBUG("rx end, wait_time:%d ", wait_time);
//     FTD_LOG_TRACE_BUFF(rx_buff, sizeof(rx_buff), sizeof(rx_buff));
//     slave_ack = ftd_mw_slave_check_rx(rx_buff, sizeof(rx_buff));
//     if (SLAVE_CMD_CALIBRATE_MADC_VALUE == slave_ack)
//     {
//         uint8_t param_len = rx_buff[2];
//         if (param_len == 1)
//             madc_value = rx_buff[3];
//         else if (param_len == 2)
//             madc_value = rx_buff[3] << 8 | rx_buff[4];
//         else if (param_len == 3)
//             madc_value = rx_buff[3] << 16 | rx_buff[4] << 8 | rx_buff[5];
//         else if (param_len == 4)
//             madc_value = rx_buff[3] << 24 | rx_buff[4] << 16 | rx_buff[5] << 8 | rx_buff[6];
//         FTD_LOG_DEBUG("ch:%d calibrate madc ack [0x%02x],parm_len :%d, value [0x%08x]",
//             p_st_slave_info->slave_uart_channel, slave_ack, param_len, madc_value);
//         cmd_ret = true;
//         CLK_SysTickLongDelay(1000);
//     }
//     else
//     {
//         FTD_LOG_WARN("ch:%d slave calibrate madc value unack", p_st_slave_info->slave_uart_channel);
//         cmd_ret = false;
//         return cmd_ret;
//     }
//     // if (madc_value != 0)
//     value_ret = true;
//     return cmd_ret && value_ret;
// }

//Calibrate the GPADC
bool ftd_mw_slave_manager_calibrate_gpadc(SLAVE_INFO* p_st_slave_info)
{
    bool    cmd_ret = false;
    bool    value_ret = false;
    uint32_t gpadc_value = 0;

    uint16_t wait_time = 0;
    uint8_t send_cmd_buff[2] = { 0x5A, 0x2F }; //uart protocol
    uint32_t start_time = ftd_mw_misc_manager_get_time_ms();


    //1. send test calibrate gpadc command
    FTD_LOG_TRACE("tx start");
    memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
    ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, sizeof(send_cmd_buff));
    ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_cmd_buff, sizeof(send_cmd_buff));
    while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
    FTD_LOG_TRACE("rx start");

    //2. check calibrate gpadc ack
    wait_time = 1000;
    while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }

    FTD_LOG_DEBUG("rx end, wait_time:%d ", wait_time);
    FTD_LOG_TRACE_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));
    uint8_t slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));
    if (SLAVE_CMD_CALIBRATE_GPADC == slave_ack)
    {
        FTD_LOG_DEBUG("ch:%d check test calibrate gpadc ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
        cmd_ret = true;
    }
    else
    {
        FTD_LOG_WARN("ch:%d slave test calibrate gpadc unack", p_st_slave_info->slave_uart_channel);
        cmd_ret = false;
        return cmd_ret;
    }


    //7. check the chip receives gpadc value
    memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
    ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, 6);
    wait_time = 10000;
    while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }
    FTD_LOG_DEBUG("rx end, wait_time:%d ", wait_time);

    FTD_LOG_TRACE_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));

    slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));

    if (SLAVE_CMD_CALIBRATE_GPADC_VALUE == slave_ack)
    {
        uint8_t param_len = SLAVE_CHN1_FUFF[2];
        if (param_len == 1)
            gpadc_value = SLAVE_CHN1_FUFF[3];
        else if (param_len == 2)
            gpadc_value = SLAVE_CHN1_FUFF[3] << 8 | SLAVE_CHN1_FUFF[4];
        else if (param_len == 3)
            gpadc_value = SLAVE_CHN1_FUFF[3] << 16 | SLAVE_CHN1_FUFF[4] << 8 | SLAVE_CHN1_FUFF[5];
        else if (param_len == 4)
            gpadc_value = SLAVE_CHN1_FUFF[3] << 24 | SLAVE_CHN1_FUFF[4] << 16 | SLAVE_CHN1_FUFF[5] << 8 | SLAVE_CHN1_FUFF[6];

        FTD_LOG_DEBUG("ch:%d calibrate gpadc ack [0x%02x],param_len :%d, value [0x%08x]",
            p_st_slave_info->slave_uart_channel, slave_ack, param_len, gpadc_value);
        cmd_ret = true;
        CLK_SysTickLongDelay(1000);
    }
    else
    {
        FTD_LOG_WARN("ch:%d slave calibrate gpadc value unack", p_st_slave_info->slave_uart_channel);
        cmd_ret = false;
        return cmd_ret;
    }

    if (gpadc_value != 0)
        value_ret = true;

    // Ensure at least 100ms from function start to end
    uint32_t elapsed_time = ftd_mw_misc_manager_get_time_ms() - start_time;
    if (elapsed_time < SLAVE_SEND_PACKET_DELAY_MS) {
        uint32_t delay_time = SLAVE_SEND_PACKET_DELAY_MS - elapsed_time;
        FTD_LOG_TRACE("Adding delay %dms to reach %dms total", delay_time, SLAVE_SEND_PACKET_DELAY_MS);
        for (uint32_t i = 0; i < delay_time; i++) {
            CLK_SysTickLongDelay(1000); // 1ms delay
        }
    }
    FTD_LOG_DEBUG("Total time from function start to end: %dms", ftd_mw_misc_manager_get_time_ms() - start_time);

    return cmd_ret && value_ret;

}

//Calibrate the CLOCK
bool ftd_mw_slave_manager_calibrate_clock(SLAVE_INFO* p_st_slave_info)
{
    bool    cmd_ret = false;
    bool    value_ret = false;
    uint32_t clock_value = 0;

    uint16_t wait_time = 0;
    uint8_t send_cmd_buff[2] = { 0x5A, 0x30 }; //uart protocol
    uint32_t start_time = ftd_mw_misc_manager_get_time_ms();

    //1. send test calibrate clock command
    FTD_LOG_TRACE("tx start");
    memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
    ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, sizeof(send_cmd_buff));
    ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_cmd_buff, sizeof(send_cmd_buff));
    while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
    FTD_LOG_TRACE("rx start");

    //2. check calibrate clock ack
    wait_time = 1000;
    while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }

    FTD_LOG_DEBUG("rx end, wait_time:%d ", wait_time);

    FTD_LOG_INFO("CH%d: Calibrate clock ACK data: [0x%02x, 0x%02x]", p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF[0], SLAVE_CHN1_FUFF[1]);

    uint8_t slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));

    if (SLAVE_CMD_CALIBRATE_CLOCK == slave_ack)
    {
        FTD_LOG_DEBUG("ch:%d check test calibrate clock ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
        cmd_ret = true;
    }
    else
    {
        FTD_LOG_WARN("ch:%d slave test calibrate clock unack", p_st_slave_info->slave_uart_channel);
        cmd_ret = false;
        return cmd_ret;
    }


    //7. check the chip calibrate clock value
    memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
    ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, 8);
    wait_time = 1000;
    while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }
    FTD_LOG_DEBUG("rx end, wait_time:%d ", wait_time);

    //FTD_LOG_DEBUG_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));

    slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));

    if (SLAVE_CMD_CALIBRATE_CLOCK_VALUE == slave_ack)
    {
        uint8_t param_len = SLAVE_CHN1_FUFF[2];

        if (param_len <= 0)
        {
            value_ret = false;
        }
        else {
            value_ret = SLAVE_CHN1_FUFF[3];
            clock_value = SLAVE_CHN1_FUFF[4] << 24 | SLAVE_CHN1_FUFF[5] << 16 | SLAVE_CHN1_FUFF[6] << 8 | SLAVE_CHN1_FUFF[7];
        }
        FTD_LOG_DEBUG("ch:%d calibrate clock ack [0x%02x], param_len %d,value_ret %s ,clock_value [0x%08x]",
            p_st_slave_info->slave_uart_channel, param_len, slave_ack, value_ret ? "true" : "false", clock_value);
        cmd_ret = true;
        CLK_SysTickLongDelay(1000);
    }
    else
    {
        FTD_LOG_WARN("ch:%d slave calibrate clock value unack", p_st_slave_info->slave_uart_channel);
        cmd_ret = false;
        return cmd_ret;
    }

    // Ensure at least 100ms from function start to end
    uint32_t elapsed_time = ftd_mw_misc_manager_get_time_ms() - start_time;
    if (elapsed_time < SLAVE_SEND_PACKET_DELAY_MS) {
        uint32_t delay_time = SLAVE_SEND_PACKET_DELAY_MS - elapsed_time;
        FTD_LOG_TRACE("Adding delay %dms to reach %dms total", delay_time, SLAVE_SEND_PACKET_DELAY_MS);
        for (uint32_t i = 0; i < delay_time; i++) {
            CLK_SysTickLongDelay(1000); // 1ms delay
        }
    }
    FTD_LOG_DEBUG("Total time from function start to end: %dms", ftd_mw_misc_manager_get_time_ms() - start_time);

    return cmd_ret && value_ret;

}

// Test sleep current
bool ftd_mw_slave_manager_test_sleep_current(SLAVE_INFO* p_st_slave_info)
{
    bool    cmd_ret = false;
    float cur_value = 0.0f;
    bool cur_ret = false;
    uint16_t wait_time = 0;
    uint8_t send_cmd_buff[2] = { 0x5A, 0x11 }; //uart protocol

    //1. send test sleep current command
    FTD_LOG_TRACE("tx start");
    memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
    ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, sizeof(send_cmd_buff));
    ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, send_cmd_buff, sizeof(send_cmd_buff));
    while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
    FTD_LOG_TRACE("rx start");

    //2. check test sleep current ack
    wait_time = 1000;
    while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
    {
        CLK_SysTickLongDelay(1000);
    }

    FTD_LOG_DEBUG("rx end, wait_time:%d ", wait_time);

    FTD_LOG_TRACE_BUFF(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF), sizeof(SLAVE_CHN1_FUFF));

    uint8_t slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));

    if (SLAVE_CMD_TEST_SLEEP_CURRENT == slave_ack)
    {
        FTD_LOG_DEBUG("ch:%d check test sleep current ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
        cmd_ret = true;
    }
    else
    {
        FTD_LOG_WARN("ch:%d slave test sleep current unack", p_st_slave_info->slave_uart_channel);
        cmd_ret = false;
        return cmd_ret;
    }
    //wait chip to sleep
    for (int i = 0; i < 100; i++)
    {
        CLK_SysTickLongDelay(1000);
    }


    //6. test the operating current of the chip
    cur_value = ftd_mw_powermon_manager_get_channel_current(p_st_slave_info->slave_uart_channel);
    FTD_LOG_INFO("Measured current: %.2f mA", cur_value);
    // Always return true since we're just measuring
    cur_ret = true;

    return cmd_ret;

}

bool ftd_mw_slave_manager_restart_test(SLAVE_INFO* p_st_slave_info)
{
    ftd_mw_slave_manager_power_off(p_st_slave_info->slave_uart_channel);
    for (int i = 0; i < 300; i++)
    {
        CLK_SysTickLongDelay(1000);
    }

    ftd_mw_slave_manager_power_on(p_st_slave_info->slave_uart_channel);
    for (int i = 0; i < 300; i++)
    {
        CLK_SysTickLongDelay(1000);
    }

    return true;
}



///////////////////////////////////////////// debug end
void ftd_mw_slave_manager_set_default_slave_info(SLAVE_INFO* p_st_slave_info)
{
    p_st_slave_info->uart_baudrate = 115200;
    p_st_slave_info->en_slave_cmd_state = SLAVE_CMD_ENTER_BOOT_LOADER;
}

void ftd_mw_slave_manager_set_bin_info(uint32_t bin_addr, uint32_t bin_size, uint16_t bin_crc16)
{

}

/**
 * @brief Unified state machine context for programming and CRC check
 * @note One channel cannot do program and CRC check at the same time,
 *       so they share the same context to save memory.
 */
typedef struct
{
    // Common fields
    SM_STATE     state;                                 // Current state
    SM_OPERATION operation;                             // Current operation type
    uint8_t      channel;                               // UART channel number
    uint32_t     fw_size;                               // Total firmware size
    uint32_t     fw_read_offset;                        // Current read offset
    uint32_t     slv_flash_addr;                        // Slave flash address
    uint16_t     uart_cmd_len;                          // UART command length
    uint32_t     rx_start_tick;                         // RX start timestamp
    uint16_t     retry_count;                           // Retry counter
    bool         error_flag;                            // Error flag
    bool         success_flag;                          // Success flag (boot/crc)

    // Chip identification fields (for dynamic address mapping)
    uint16_t     chip_code;                             // Chip code (e.g., 0x2202)
    CHIP_FAMILY  chip_family;                           // Chip family (FL100/FL100B)
    uint8_t      fw_num;                                // Firmware number (0-2)

    // Boot trigger specific fields
    uint8_t      boot_trigger_cnt;                      // Outer loop counter (boot only)
    uint8_t      tx_cmd_cnt;                            // Inner loop counter (boot only)
    uint8_t      rx_wait_cnt;                           // RX wait counter (boot only)
    uint8_t* boot_trigger_cmd;                      // Boot trigger command pointer (boot only)
    uint8_t      boot_trigger_cmd_len;                  // Boot trigger command length (boot only)

    // Program specific fields
    uint8_t      read_partition;                        // Flash partition to read from
    uint8_t      fw_buff[SLAVE_FLASH_PROG_SIZE];        // Firmware data buffer (program only)

    // CRC check specific fields
    uint32_t     read_len;                              // Current read length (CRC only)
    uint16_t     calc_crc;                              // Calculated CRC (CRC only)
    uint16_t     expect_crc;                            // Expected CRC from sys_info (CRC only)
    bool         crc_pass;                              // CRC check result (CRC only)

    // Calibration/Test specific fields
    CALIB_TEST_TYPE calib_type;                         // Calibration/test type
    uint8_t         cmd_code;                           // Command code
    uint8_t         expect_ack;                         // Expected ACK
    uint8_t         rx_ack_len;                         // Receive ACK length
    uint8_t         rx_value_len;                       // Receive value length
    uint32_t        min_delay_ms;                       // Minimum delay (ms)
    float           cur_value;                          // Current measurement value
    uint32_t        calib_value;                        // Calibration value
    bool            value_valid;                        // Value valid flag
    uint32_t        calib_start_time;                   // Calibration start time for delay calculation

    // Buffers (use max size to support both operations)
    uint8_t      uart_tx_buff[SLAVE_FLASH_PROG_CMD_SIZE_MAX]; // UART TX buffer
    uint8_t      uart_rx_buff[BURN_UART_DATA_MAX];            // UART RX buffer
    uint8_t      uart_rx_value_buff[BURN_UART_DATA_MAX];      // UART RX buffer for value responses
} SM_CONTEXT;

// Keep old type names for compatibility
typedef SM_CONTEXT PROG_CONTEXT;
typedef SM_CONTEXT CRC_CONTEXT;
typedef SM_CONTEXT BOOT_SM_CONTEXT;
typedef SM_CONTEXT RAM_PROG_CONTEXT;
typedef SM_CONTEXT JUMP_SM_CONTEXT;

// Static context for each channel (shared between program and CRC)
static SM_CONTEXT g_sm_ctx[BURN_UART_CHANNELS] = { 0 };

// Compatibility macros
#define g_prog_ctx g_sm_ctx
#define g_crc_ctx  g_sm_ctx
#define g_boot_ctx g_sm_ctx
#define g_ram_prog_ctx g_sm_ctx
#define g_jump_ctx g_sm_ctx


/******************************************************************************
 * Calibration/Test State Machine Implementation
 ******************************************************************************/

 /**
  * @brief Initialize calibration/test context for a channel
  * @param p_ctx Pointer to context
  * @param p_slave_info Pointer to slave info
  * @param calib_type Calibration/test type
  */
static void ftd_mw_slave_manager_calib_test_ctx_init(SM_CONTEXT* p_ctx,
    SLAVE_INFO* p_slave_info,
    CALIB_TEST_TYPE calib_type)
{
    memset(p_ctx, 0, sizeof(SM_CONTEXT));
    p_ctx->state = SM_STATE_IDLE;
    p_ctx->operation = SM_OP_CALIB_TEST;
    p_ctx->channel = p_slave_info->slave_uart_channel;
    p_ctx->calib_type = calib_type;
    p_ctx->error_flag = false;
    p_ctx->value_valid = false;
    p_ctx->calib_value = 0;
    p_ctx->cur_value = 0.0f;

    // Configure parameters based on calibration type
    switch (calib_type)
    {
        case CALIB_TYPE_GPADC:
            p_ctx->cmd_code = 0x2F;
            p_ctx->expect_ack = SLAVE_CMD_CALIBRATE_GPADC;
            p_ctx->rx_ack_len = 2; // ACK length
            p_ctx->rx_value_len = 6; // Value response length
            p_ctx->min_delay_ms = SLAVE_SEND_PACKET_DELAY_MS; // 100ms
            break;

        case CALIB_TYPE_CLOCK:
            p_ctx->cmd_code = 0x30;
            p_ctx->expect_ack = SLAVE_CMD_CALIBRATE_CLOCK;
            p_ctx->rx_ack_len = 2; // ACK length
            p_ctx->rx_value_len = 8; // Value response length
            p_ctx->min_delay_ms = SLAVE_SEND_PACKET_DELAY_MS; // 100ms
            break;

        case CALIB_TYPE_MADC:
            p_ctx->cmd_code = 0x2E;
            p_ctx->expect_ack = SLAVE_CMD_CALIBRATE_MADC;
            p_ctx->rx_ack_len = 2; // ACK length
            p_ctx->rx_value_len = 6; // Value response length
            p_ctx->min_delay_ms = 0; // No delay required
            break;

        case TEST_TYPE_STATIC_CURRENT:
            p_ctx->cmd_code = 0x2C;
            p_ctx->expect_ack = SLAVE_CMD_TEST_CURRENT_AT_STATIC;
            p_ctx->rx_ack_len = 2; // Only ACK, current measured by INA236
            p_ctx->rx_value_len = 0; // No value response
            p_ctx->min_delay_ms = 10; // 10ms delay before measurement
            break;

        case TEST_TYPE_SLEEP_CURRENT:
            p_ctx->cmd_code = 0x11;
            p_ctx->expect_ack = SLAVE_CMD_TEST_SLEEP_CURRENT;
            p_ctx->rx_ack_len = 2; // Only ACK, current measured by INA236
            p_ctx->rx_value_len = 0; // No value response
            p_ctx->min_delay_ms = 100; // Wait for chip to sleep
            break;

        default:
            FTD_LOG_ERROR("CH%d: Invalid calibration type: %d", p_ctx->channel, calib_type);
            p_ctx->error_flag = true;
            break;
    }

    FTD_LOG_TRACE("CH%d: Calib/Test SM initialized, type=%d, cmd=0x%02X",
        p_ctx->channel, calib_type, p_ctx->cmd_code);
}

/**
 * @brief Process one state machine step for calibration/test operation
 * @param p_ctx Pointer to context
 * @return true if operation completed (success or error), false if still in progress
 */
static bool ftd_mw_slave_manager_calib_test_state_machine(SM_CONTEXT* p_ctx)
{
    uint32_t elapsed;

    switch (p_ctx->state)
    {
        case SM_STATE_IDLE:
        {
            // Check if initialization failed
            if (p_ctx->error_flag)
            {
                p_ctx->state = SM_STATE_ERROR;
                return true;
            }

            // Prepare command buffer
            p_ctx->uart_tx_buff[0] = 0x5A;
            p_ctx->uart_tx_buff[1] = p_ctx->cmd_code;

            p_ctx->state = SM_STATE_TX_STARTING;
            FTD_LOG_TRACE("CH%d: Prepared calib/test command [0x%02X 0x%02X]",
                p_ctx->channel, p_ctx->uart_tx_buff[0], p_ctx->uart_tx_buff[1]);
            break;
        }

        case SM_STATE_TX_STARTING:
        {
            // Clear RX buffer and start UART DMA
            memset(p_ctx->uart_rx_buff, 0x00, p_ctx->rx_ack_len);
            ftd_drv_burn_uart_rx_start(p_ctx->channel, p_ctx->uart_rx_buff, p_ctx->rx_ack_len);
            ftd_drv_burn_uart_tx_start(p_ctx->channel, p_ctx->uart_tx_buff, 2);

            p_ctx->state = SM_STATE_WAIT_TX;
            FTD_LOG_TRACE("CH%d: TX/RX started", p_ctx->channel);
            break;
        }

        case SM_STATE_WAIT_TX:
        {
            // Check if TX DMA complete
            if (ftd_drv_burn_channel_is_tx_complete(p_ctx->channel))
            {
                p_ctx->state = SM_STATE_WAIT_RX;
                p_ctx->rx_start_tick = ftd_mw_misc_manager_get_time_ms();
                FTD_LOG_TRACE("CH%d: TX done, waiting RX", p_ctx->channel);
            }
            break;
        }

        case SM_STATE_WAIT_RX:
        {
            // Check if RX DMA complete
            if (ftd_drv_burn_channel_is_rx_complete(p_ctx->channel))
            {
                ftd_drv_burn_uart_rx_stop(p_ctx->channel);

                // This is always an ACK response, go to check ACK
                p_ctx->state = SM_STATE_CHECK_ACK;
                FTD_LOG_TRACE("CH%d: ACK RX done, data=[%02X %02X]",
                    p_ctx->channel, p_ctx->uart_rx_buff[0], p_ctx->uart_rx_buff[1]);
            }
            else
            {
                // Check timeout for ACK response
                elapsed = ftd_mw_misc_manager_get_time_ms() - p_ctx->rx_start_tick;
                if (elapsed > 100)
                {
                    ftd_drv_burn_uart_rx_stop(p_ctx->channel);
                    FTD_LOG_WARN("CH%d: ACK RX timeout after %dms", p_ctx->channel, elapsed);
                    p_ctx->state = SM_STATE_ERROR;
                    p_ctx->error_flag = true;
                }
            }
            break;
        }

        case SM_STATE_CHECK_ACK:
        {
            // Check ACK response
            uint8_t ack = ftd_mw_slave_check_rx(p_ctx->uart_rx_buff, p_ctx->rx_ack_len);

            if (ack == p_ctx->expect_ack)
            {
                FTD_LOG_DEBUG("CH%d: Calib/test ACK OK [0x%02X]", p_ctx->channel, ack);

                // For current tests, measure current and complete
                if (p_ctx->calib_type == TEST_TYPE_SLEEP_CURRENT ||
                    p_ctx->calib_type == TEST_TYPE_STATIC_CURRENT)
                {
                    // Wait for specified delay before measuring
                    p_ctx->calib_start_time = ftd_mw_misc_manager_get_time_ms();
                    p_ctx->state = SM_STATE_MEASURE_CURRENT; // Use dedicated state for current measurement
                }
                // For calibration commands, receive calibration value
                else if (p_ctx->rx_value_len > 0)
                {
                    // Start RX for calibration value using dedicated buffer
                    memset(p_ctx->uart_rx_value_buff, 0x00, BURN_UART_DATA_MAX);
                    ftd_drv_burn_uart_rx_start(p_ctx->channel, p_ctx->uart_rx_value_buff, p_ctx->rx_value_len);
                    p_ctx->rx_start_tick = ftd_mw_misc_manager_get_time_ms();
                    p_ctx->state = SM_STATE_WAIT_VALUE; // Use dedicated state for value reception
                }
                else
                {
                    // No value expected, complete directly
                    p_ctx->state = SM_STATE_COMPLETE;
                }
            }
            else
            {
                FTD_LOG_ERROR("CH%d: Calib/test ACK failed, expected=0x%02X, got=0x%02X",
                    p_ctx->channel, p_ctx->expect_ack, ack);
                p_ctx->state = SM_STATE_ERROR;
                p_ctx->error_flag = true;
            }
            break;
        }

        case SM_STATE_MEASURE_CURRENT: // Dedicated state for current measurement
        {
            // For current tests, measure after delay
            elapsed = ftd_mw_misc_manager_get_time_ms() - p_ctx->calib_start_time;
            if (elapsed >= p_ctx->min_delay_ms)
            {
                // Measure current using INA236
                p_ctx->cur_value = ftd_mw_powermon_manager_get_channel_current(p_ctx->channel);
                FTD_LOG_INFO("CH%d: Measured current: %.2f mA", p_ctx->channel, p_ctx->cur_value);
                p_ctx->value_valid = true;

                // Current test always succeeds (just measurement)
                p_ctx->state = SM_STATE_COMPLETE;
                return true;
            }
            break;
        }

        case SM_STATE_WAIT_VALUE: // Dedicated state for calibration value reception
        {
            if (ftd_drv_burn_channel_is_rx_complete(p_ctx->channel))
            {
                ftd_drv_burn_uart_rx_stop(p_ctx->channel);
                p_ctx->state = SM_STATE_PARSE_VALUE;
                FTD_LOG_TRACE("CH%d: Value RX done, parsing value", p_ctx->channel);
            }
            else
            {
                // Check timeout for calibration value reception
                elapsed = ftd_mw_misc_manager_get_time_ms() - p_ctx->rx_start_tick;
                uint16_t timeout = (p_ctx->calib_type == CALIB_TYPE_GPADC) ? 10000 : 3000;

                if (elapsed > timeout)
                {
                    ftd_drv_burn_uart_rx_stop(p_ctx->channel);
                    FTD_LOG_WARN("CH%d: Calib value RX timeout after %dms", p_ctx->channel, elapsed);
                    p_ctx->state = SM_STATE_ERROR;
                    p_ctx->error_flag = true;
                }
            }
            break;
        }

        case SM_STATE_PARSE_VALUE: // Dedicated state for parsing calibration values
        {
            // Parse calibration value from response
            uint8_t param_len = p_ctx->uart_rx_value_buff[2];
            if (param_len == 1)
                p_ctx->calib_value = p_ctx->uart_rx_value_buff[3];
            else if (param_len == 2)
                p_ctx->calib_value = p_ctx->uart_rx_value_buff[3] << 8 | p_ctx->uart_rx_value_buff[4];
            else if (param_len == 3)
                p_ctx->calib_value = p_ctx->uart_rx_value_buff[3] << 16 | p_ctx->uart_rx_value_buff[4] << 8 | p_ctx->uart_rx_value_buff[5];
            else if (param_len == 4)
                p_ctx->calib_value = p_ctx->uart_rx_value_buff[3] << 24 | p_ctx->uart_rx_value_buff[4] << 16 | p_ctx->uart_rx_value_buff[5] << 8 | p_ctx->uart_rx_value_buff[6];

            FTD_LOG_INFO("CH%d: Calib value [0x%08X], param_len=%d",
                p_ctx->channel, p_ctx->calib_value, param_len);

            // Validate calibration value
            p_ctx->value_valid = (p_ctx->calib_value != 0);

            // Apply minimum delay if required
            if (p_ctx->min_delay_ms > 0)
            {
                p_ctx->calib_start_time = ftd_mw_misc_manager_get_time_ms();
                p_ctx->state = SM_STATE_WAIT_POWER_STABLE; // Reuse for delay
            }
            else
            {
                p_ctx->state = SM_STATE_COMPLETE;
                return true;
            }
            break;
        }

        case SM_STATE_WAIT_POWER_STABLE: // Reused for minimum delay
        {
            elapsed = ftd_mw_misc_manager_get_time_ms() - p_ctx->calib_start_time;
            if (elapsed >= p_ctx->min_delay_ms)
            {
                FTD_LOG_DEBUG("Total time from function start to end: %dms",
                    ftd_mw_misc_manager_get_time_ms() - p_ctx->calib_start_time + p_ctx->min_delay_ms);
                p_ctx->state = SM_STATE_COMPLETE;
                return true;
            }
            break;
        }

        case SM_STATE_COMPLETE:
            return true; // Complete successfully

        case SM_STATE_ERROR:
            return true; // Complete with error

        default:
            p_ctx->state = SM_STATE_ERROR;
            p_ctx->error_flag = true;
            return true;
    }

    return false; // Still in progress
}

/**
 * @brief Start calibration/test operation (non-blocking)
 * @param p_st_slave_info Pointer to slave info
 * @param calib_type Calibration/test type
 * @return true if operation completed, false if still in progress
 */
static bool ftd_mw_slave_manager_start_calib_test(SLAVE_INFO* p_st_slave_info,
    CALIB_TEST_TYPE calib_type)
{
    uint8_t channel = p_st_slave_info->slave_uart_channel;

    // Validate channel
    if (channel >= BURN_UART_CHANNELS)
    {
        FTD_LOG_ERROR("Invalid channel: %d", channel);
        return false;
    }

    SM_CONTEXT* p_ctx = &g_sm_ctx[channel];

    // Initialize context on first call
    if (p_ctx->state == SM_STATE_IDLE || p_ctx->calib_type != calib_type)
    {
        ftd_mw_slave_manager_calib_test_ctx_init(p_ctx, p_st_slave_info, calib_type);
    }

    // Run state machine
    bool completed = ftd_mw_slave_manager_calib_test_state_machine(p_ctx);

    if (completed)
    {
        bool success = (p_ctx->state == SM_STATE_COMPLETE && !p_ctx->error_flag);
        FTD_LOG_TRACE("CH%d: Calib/Test SM %s", channel, success ? "SUCCESS" : "FAILED");

        // Reset state for next use
        p_ctx->state = SM_STATE_IDLE;

        return success;
    }

    return false; // Still in progress
}

/**
 * @brief Check if calibration/test state machine has error
 * @param channel Channel number
 * @return true if error, false otherwise
 */
bool ftd_mw_slave_manager_calib_test_is_error(uint8_t channel)
{
    if (channel >= BURN_UART_CHANNELS)
        return false;

    return (g_sm_ctx[channel].state == SM_STATE_ERROR || g_sm_ctx[channel].error_flag);
}


/******************************************************************************
 * Bootloader Trigger State Machine Implementation
 ******************************************************************************/

 // Bootloader trigger commands for different chip families
 // A series (FL100): 0x55, 0x96 pattern
static uint8_t g_boot_trigger_cmd_fl100am[] = { 0x55, 0x96 };
// B series (FL100B): 0x40, 0x40 pattern
static uint8_t g_boot_trigger_cmd_fl100bm[] = { 0x40, 0x40, 0x40, 0x40, 0x40, 0x40 };
// C series (PY32): 0x55, 0x96 pattern
static uint8_t g_boot_trigger_cmd_py32[] = { 0x55, 0x96 };
// D series (CW32): 0x55, 0x96 pattern
static uint8_t g_boot_trigger_cmd_cw32[] = { 0x55, 0x96 };

/**
 * @brief Initialize bootloader trigger context for a channel
 * @param p_ctx Pointer to boot context
 * @param p_slave_info Pointer to slave info
 * @param fw_num Firmware number (0-2) for chip_id lookup
 */
static void ftd_mw_slave_manager_boot_ctx_init(BOOT_SM_CONTEXT* p_ctx, SLAVE_INFO* p_slave_info, uint8_t fw_num)
{
    // Get chip flash map by chip_code
    SYS_INFO st_sys_info;
    ftd_mw_sys_info_manager_get(&st_sys_info);
    uint16_t chip_code = (uint16_t)st_sys_info.st_fw_info[fw_num].chip_id;
    const CHIP_FLASH_ADDR_MAP* p_chip_map = ftd_mw_slave_get_chip_flash_map(chip_code);

    memset(p_ctx, 0, sizeof(BOOT_SM_CONTEXT));
    p_ctx->state = BOOT_SM_STATE_IDLE;
    p_ctx->operation = SM_OP_BOOT_TRIGGER;
    p_ctx->channel = p_slave_info->slave_uart_channel;
    p_ctx->boot_trigger_cnt = 0;
    p_ctx->tx_cmd_cnt = 0;
    p_ctx->rx_wait_cnt = 0;
    p_ctx->error_flag = false;
    p_ctx->success_flag = false;

    // Store chip identification for dynamic command selection
    p_ctx->chip_code = chip_code;
    p_ctx->chip_family = p_chip_map->chip_family;
    p_ctx->fw_num = fw_num;

    // Select boot trigger command based on chip family
    if (p_chip_map->chip_family == CHIP_FAMILY_FL100B)
    {
        p_ctx->boot_trigger_cmd = g_boot_trigger_cmd_fl100bm;
        p_ctx->boot_trigger_cmd_len = sizeof(g_boot_trigger_cmd_fl100bm);
        //ftd_drv_burn_uart_single_deinit(p_ctx->channel);
        ftd_drv_burn_uart_single_init(p_ctx->channel, TRANSFER_BAUDRATE);
        FTD_LOG_DEBUG("CH%d: Using FL100B boot trigger (0x40), Chip: %s", p_ctx->channel, p_chip_map->chip_name);
    }
    else if (p_chip_map->chip_family == CHIP_FAMILY_FL100)
    {
        p_ctx->boot_trigger_cmd = g_boot_trigger_cmd_fl100am;
        p_ctx->boot_trigger_cmd_len = sizeof(g_boot_trigger_cmd_fl100am);
        FTD_LOG_DEBUG("CH%d: Using FL100 boot trigger (0x55,0x96), Chip: %s", p_ctx->channel, p_chip_map->chip_name);
    }
    else if (p_chip_map->chip_family == CHIP_FAMILY_PY32)
    {
        p_ctx->boot_trigger_cmd = g_boot_trigger_cmd_py32;
        p_ctx->boot_trigger_cmd_len = sizeof(g_boot_trigger_cmd_py32);
        FTD_LOG_DEBUG("CH%d: Using PY32 boot trigger (0x55,0x96), Chip: %s", p_ctx->channel, p_chip_map->chip_name);
    }
    else if (p_chip_map->chip_family == CHIP_FAMILY_CW32)
    {
        p_ctx->boot_trigger_cmd = g_boot_trigger_cmd_cw32;
        p_ctx->boot_trigger_cmd_len = sizeof(g_boot_trigger_cmd_cw32);
        FTD_LOG_DEBUG("CH%d: Using CW32 boot trigger (0x55,0x96), Chip: %s", p_ctx->channel, p_chip_map->chip_name);
    }

    FTD_LOG_INFO("CH%d: Boot SM context initialized, chip_code=0x%04X, family=%d",
        p_ctx->channel, p_ctx->chip_code, p_ctx->chip_family);
}

/**
 * @brief Bootloader trigger state machine core function (non-blocking)
 * @param p_ctx Pointer to boot context
 * @return true if completed (success or error), false if still in progress
 */
static bool ftd_mw_slave_manager_boot_state_machine(BOOT_SM_CONTEXT* p_ctx)
{
    uint32_t elapsed;
    uint8_t slave_ack;

    switch (p_ctx->state)
    {
        case BOOT_SM_STATE_IDLE:
            // Start - transition to power off first
            p_ctx->state = BOOT_SM_STATE_POWER_OFF;
            FTD_LOG_TRACE("CH%d: Boot SM start, trigger_cnt=%d", p_ctx->channel, p_ctx->boot_trigger_cnt);
            break;

        case BOOT_SM_STATE_POWER_OFF:
            // Start UART DMA receive first before power off
            memset(p_ctx->uart_rx_buff, 0x00, sizeof(p_ctx->uart_rx_buff));
            ftd_drv_burn_uart_rx_start(p_ctx->channel, p_ctx->uart_rx_buff, 11);
            FTD_LOG_TRACE("CH%d: RX started before power off", p_ctx->channel);

            // Power off slave first to ensure clean boot
            ftd_mw_slave_manager_power_off(p_ctx->channel);
            FTD_LOG_TRACE("CH%d: Power off", p_ctx->channel);

            // Start cyclic send task for bootloader trigger commands
            ftd_mw_misc_manager_start_cyclic_task(p_ctx->channel,
                ftd_drv_burn_uart_tx_start,
                p_ctx->boot_trigger_cmd,
                p_ctx->boot_trigger_cmd_len,
                10); // 10ms period

            // Start wait timer for power off stable
            p_ctx->rx_start_tick = ftd_mw_misc_manager_get_time_ms();
            p_ctx->state = BOOT_SM_STATE_WAIT_POWER_OFF;
            break;

        case BOOT_SM_STATE_WAIT_POWER_OFF:
            // Wait 100ms for power off stable (non-blocking)
            elapsed = ftd_mw_misc_manager_get_time_ms() - p_ctx->rx_start_tick;
            if (elapsed >= 300)
            {
                FTD_LOG_TRACE("CH%d: Power off stable after %dms", p_ctx->channel, elapsed);
                p_ctx->state = BOOT_SM_STATE_POWER_ON;
            }
            // Still waiting, return false to allow other channels to run
            break;

        case BOOT_SM_STATE_POWER_ON:
            // Power on slave
            ftd_mw_slave_manager_power_on(p_ctx->channel);
            FTD_LOG_TRACE("CH%d: Power on", p_ctx->channel);

            // Start wait timer for power stable
            p_ctx->rx_start_tick = ftd_mw_misc_manager_get_time_ms();
            p_ctx->state = BOOT_SM_STATE_WAIT_POWER_STABLE;
            break;

        case BOOT_SM_STATE_WAIT_POWER_STABLE:
            // Wait 50ms for power stable (non-blocking)
            elapsed = ftd_mw_misc_manager_get_time_ms() - p_ctx->rx_start_tick;
            if (elapsed >= 47)
            {
                FTD_LOG_TRACE("CH%d: Power stable after %dms", p_ctx->channel, elapsed);

                p_ctx->rx_wait_cnt = 150;  // Max 150ms wait
                p_ctx->rx_start_tick = ftd_mw_misc_manager_get_time_ms();
                p_ctx->state = BOOT_SM_STATE_WAIT_RX;
                FTD_LOG_TRACE("CH%d: Entering wait RX state, cyclic trigger ongoing", p_ctx->channel);
            }
            // Still waiting, return false to allow other channels to run
            break;

        case BOOT_SM_STATE_WAIT_RX:
            // Check if RX complete (non-blocking)
            if (ftd_drv_burn_channel_is_rx_complete(p_ctx->channel))
            {
                // Stop cyclic bootloader trigger task
                ftd_mw_misc_manager_stop_cyclic_task(p_ctx->channel);

                p_ctx->state = BOOT_SM_STATE_CHECK_ACK;
                FTD_LOG_TRACE("CH%d: RX complete", p_ctx->channel);
            }
            else
            {
                // Check timeout (1ms per wait count)
                elapsed = ftd_mw_misc_manager_get_time_ms() - p_ctx->rx_start_tick;
                if (elapsed >= p_ctx->rx_wait_cnt)
                {
                    // Stop cyclic bootloader trigger task
                    ftd_mw_misc_manager_stop_cyclic_task(p_ctx->channel);

                    // Timeout, check ACK anyway
                    p_ctx->state = BOOT_SM_STATE_CHECK_ACK;
                    FTD_LOG_TRACE("CH%d: RX timeout after %dms", p_ctx->channel, elapsed);
                }
            }
            break;

        case BOOT_SM_STATE_CHECK_ACK:
            // Check ACK response
            // FTD_LOG_DEBUG_BUFF(p_ctx->uart_rx_buff, sizeof(p_ctx->uart_rx_buff), 11);
            slave_ack = ftd_mw_slave_check_rx(p_ctx->uart_rx_buff, sizeof(p_ctx->uart_rx_buff));

            if (SLAVE_CMD_ENTER_BOOT_LOADER == slave_ack)
            {
                // Success!
                FTD_LOG_DEBUG("CH%d: Bootloader ACK [0x%02x] cnt:%d",
                    p_ctx->channel, slave_ack, p_ctx->tx_cmd_cnt);
                p_ctx->success_flag = true;
                p_ctx->state = BOOT_SM_STATE_COMPLETE;
                return true;
            }
            else
            {
                // ACK check failed - increment retry counter and power cycle
                p_ctx->boot_trigger_cnt++;

                if (p_ctx->boot_trigger_cnt < SLAVE_BOOT_TRIGGER_CNT)
                {
                    // Retry with power cycle - stop cyclic task and go to power off
                    ftd_mw_misc_manager_stop_cyclic_task(p_ctx->channel);

                    FTD_LOG_WARN("CH%d: ACK check failed, retry %d/%d - power cycling",
                        p_ctx->channel, p_ctx->boot_trigger_cnt, SLAVE_BOOT_TRIGGER_CNT);
                    p_ctx->tx_cmd_cnt = 0;
                    p_ctx->state = BOOT_SM_STATE_POWER_OFF;  // Power off before retry
                }
                else
                {
                    // Stop cyclic bootloader trigger task
                    ftd_mw_misc_manager_stop_cyclic_task(p_ctx->channel);

                    // All retries exhausted
                    FTD_LOG_ERROR("CH%d: Boot trigger failed after %d retries",
                        p_ctx->channel, SLAVE_BOOT_TRIGGER_CNT);
                    p_ctx->error_flag = true;
                    p_ctx->state = BOOT_SM_STATE_ERROR;
                    return true;
                }
            }
            break;

        case BOOT_SM_STATE_COMPLETE:
            return true;

        case BOOT_SM_STATE_ERROR:
            return true;

        default:
            p_ctx->state = BOOT_SM_STATE_ERROR;
            p_ctx->error_flag = true;
            return true;
    }

    return false;  // Still in progress
}

/**
 * @brief Bootloader trigger with state machine (non-blocking, for concurrent multi-channel)
 * @param p_st_slave_info Pointer to slave info
 * @param p_burn_status Pointer to burn status (for chip_id lookup)
 * @return true if completed successfully, false if still in progress or error
 *
 * @note This function uses state machine to allow concurrent multi-channel boot trigger.
 *       Return value meaning:
 *       - true: Boot trigger completed SUCCESSFULLY (slave entered bootloader)
 *       - false: Still in progress OR error occurred
 *
 *       To distinguish between "in progress" and "error", use:
 *       - ftd_mw_slave_manager_start_ld_sm_is_error(channel)
 *       - ftd_mw_slave_manager_start_ld_sm_get_state(channel)
 */
bool ftd_mw_slave_manager_start_ld_sm(SLAVE_INFO* p_st_slave_info, CHANNEL_BURN_STATUS* p_burn_status)
{
    uint8_t channel = p_st_slave_info->slave_uart_channel;
    uint8_t fw_num = p_burn_status ? p_burn_status->fw_num : 0;

    // Validate channel
    if (channel >= BURN_UART_CHANNELS)
    {
        FTD_LOG_ERROR("Boot SM: Invalid channel %d", channel);
        return false;
    }

    BOOT_SM_CONTEXT* p_ctx = &g_boot_ctx[channel];

    // Initialize on first call (state == IDLE and not yet initialized)
    if (p_ctx->state == BOOT_SM_STATE_IDLE && !p_ctx->success_flag && !p_ctx->error_flag)
    {
        ftd_mw_slave_manager_boot_ctx_init(p_ctx, p_st_slave_info, fw_num);
    }

    // Run state machine
    bool completed = ftd_mw_slave_manager_boot_state_machine(p_ctx);

    if (completed)
    {
        bool success = (p_ctx->state == BOOT_SM_STATE_COMPLETE && p_ctx->success_flag);
        FTD_LOG_TRACE("CH%d: Boot SM %s", channel, success ? "SUCCESS" : "FAILED");

        // Keep state for application to check
        // Reset will be done by ftd_mw_slave_manager_start_ld_sm_reset()

        return success;
    }

    return false;  // Still in progress
}

/**
 * @brief Check if bootloader SM is in error state
 * @param channel UART channel number (0-3)
 * @return true if error, false otherwise
 */
bool ftd_mw_slave_manager_start_ld_sm_is_error(uint8_t channel)
{
    if (channel >= BURN_UART_CHANNELS)
        return false;

    return g_boot_ctx[channel].error_flag;
}

/**
 * @brief Check if bootloader SM is complete (success or error)
 * @param channel UART channel number (0-3)
 * @return true if complete, false if still in progress
 */
bool ftd_mw_slave_manager_start_ld_sm_is_complete(uint8_t channel)
{
    if (channel >= BURN_UART_CHANNELS)
        return false;

    BOOT_SM_STATE state = g_boot_ctx[channel].state;
    return (state == BOOT_SM_STATE_COMPLETE || state == BOOT_SM_STATE_ERROR);
}

/**
 * @brief Get bootloader SM current state
 * @param channel UART channel number (0-3)
 * @return Current state
 */
BOOT_SM_STATE ftd_mw_slave_manager_start_ld_sm_get_state(uint8_t channel)
{
    if (channel >= BURN_UART_CHANNELS)
        return BOOT_SM_STATE_ERROR;

    return g_boot_ctx[channel].state;
}

/**
 * @brief Reset bootloader SM for next use
 * @param channel UART channel number (0-3)
 */
void ftd_mw_slave_manager_start_ld_sm_reset(uint8_t channel)
{
    if (channel >= BURN_UART_CHANNELS)
        return;

    // Stop any ongoing UART RX DMA transfer
    ftd_drv_burn_uart_rx_stop(channel);

    memset(&g_boot_ctx[channel], 0, sizeof(BOOT_SM_CONTEXT));
    g_boot_ctx[channel].state = BOOT_SM_STATE_IDLE;

    FTD_LOG_TRACE("CH%d: Boot SM reset", channel);
}


/**
 * @brief Initialize program context for a channel
 * @param p_ctx Pointer to program context
 * @param p_slave_info Pointer to slave info
 * @param p_burn_status Pointer to burn status
 */
static void ftd_mw_slave_manager_program_ctx_init(PROG_CONTEXT* p_ctx,
    SLAVE_INFO* p_slave_info,
    CHANNEL_BURN_STATUS* p_burn_status)
{
    // Get chip flash map by chip_code
    SYS_INFO sys_info;
    ftd_mw_sys_info_manager_get(&sys_info);
    uint16_t chip_code = (uint16_t)sys_info.st_fw_info[p_burn_status->fw_num].chip_id;
    const CHIP_FLASH_ADDR_MAP* p_chip_map = ftd_mw_slave_get_chip_flash_map(chip_code);

    memset(p_ctx, 0, sizeof(PROG_CONTEXT));
    p_ctx->state = PROG_STATE_IDLE;
    p_ctx->operation = SM_OP_PROGRAM;
    p_ctx->channel = p_slave_info->slave_uart_channel;
    p_ctx->fw_size = p_slave_info->flash_bin_size;
    p_ctx->fw_read_offset = 0;

    // Use dynamic address from chip flash map instead of hardcoded macro
    p_ctx->slv_flash_addr = p_chip_map->fw_bin_a_addr;

    // Store chip identification
    p_ctx->chip_code = chip_code;
    p_ctx->chip_family = p_chip_map->chip_family;
    p_ctx->fw_num = p_burn_status->fw_num;

    p_ctx->read_partition = PARTITION_FW1_BIN + p_burn_status->fw_num * (PARTITION_FW2_BIN - PARTITION_FW1_BIN);
    p_ctx->error_flag = false;
    p_ctx->retry_count = 0;

    // Initialize UART TX buffer header (fixed part)
    p_ctx->uart_tx_buff[0] = 0x4a;
    p_ctx->uart_tx_buff[1] = 0x59;
    p_ctx->uart_tx_buff[2] = 0x4d;
    p_ctx->uart_tx_buff[3] = 0x43;
    p_ctx->uart_tx_buff[4] = 0x54;

    // Clear UART RX buffer to avoid residual data
    memset(p_ctx->uart_rx_buff, 0x00, sizeof(p_ctx->uart_rx_buff));

    // Add a small delay to ensure slave is ready after entering bootloader
    // This prevents the first packet from being sent too early
    CLK_SysTickLongDelay(5000); // 5ms delay

    FTD_LOG_INFO("CH%d: Program SM context initialized, Chip: %s (0x%04X), FW Addr: 0x%08X",
        p_ctx->channel, p_chip_map->chip_name, p_ctx->chip_code, p_ctx->slv_flash_addr);
}

/**
 * @brief Process one state machine step for program operation
 * @param p_ctx Pointer to program context
 * @return true if operation completed (success or error), false if still in progress
 */
static bool ftd_mw_slave_manager_program_state_machine(PROG_CONTEXT* p_ctx)
{
    switch (p_ctx->state)
    {
        case PROG_STATE_IDLE:
            // Start state, transition to read flash
            p_ctx->state = PROG_STATE_READ_FLASH;
            FTD_LOG_TRACE("CH%d: Start programming, fw_size=0x%08x", p_ctx->channel, p_ctx->fw_size);
            break;

        case PROG_STATE_READ_FLASH:
        {
            // Read firmware data from flash
            if (ftd_drv_flash_operation((PARTITION_NAME)p_ctx->read_partition, OPERATION_READ,
                p_ctx->fw_read_offset, SLAVE_FLASH_PROG_SIZE, p_ctx->fw_buff) == 0)
            {
                FTD_LOG_TRACE("CH%d: Flash read OK, offset=0x%08x", p_ctx->channel, p_ctx->fw_read_offset);
                p_ctx->state = PROG_STATE_PREPARE_CMD;
            }
            else
            {
                FTD_LOG_ERROR("CH%d: Flash read failed at offset=0x%08x", p_ctx->channel, p_ctx->fw_read_offset);
                p_ctx->state = PROG_STATE_ERROR;
                p_ctx->error_flag = true;
            }
            break;
        }

        case PROG_STATE_PREPARE_CMD:
        {
            // Prepare UART command packet
            uint8_t* uart_buff = p_ctx->uart_tx_buff;

            // Log if this is a retry
            if (p_ctx->retry_count > 0)
            {
                FTD_LOG_TRACE("CH%d: Preparing retry packet #%d, offset=0x%08x, addr=0x%08x",
                    p_ctx->channel, p_ctx->retry_count, p_ctx->fw_read_offset, p_ctx->slv_flash_addr);
            }

            // Extended command
            uart_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET] = 0x00;
            uart_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET + 1] = 0x51;

            // Slave flash address
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET] = (p_ctx->slv_flash_addr >> 24) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 1] = (p_ctx->slv_flash_addr >> 16) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 2] = (p_ctx->slv_flash_addr >> 8) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 3] = p_ctx->slv_flash_addr & 0xFF;

            // Data length (fixed 0x00 for now)
            uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET] = 0x00;
            uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 1] = 0x00;
            uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 2] = 0x00;

            // Check if last packet
            if (p_ctx->fw_read_offset + SLAVE_FLASH_PROG_SIZE > p_ctx->fw_size)
            {
                // Last page
                uint32_t data_len = p_ctx->fw_size % SLAVE_FLASH_PROG_SIZE;
                uint16_t parm_len = EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_OP_LENGTH_SIZE + data_len;

                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET] = (parm_len >> 8) & 0xFF;
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] = parm_len & 0xFF;

                memcpy(&uart_buff[SLAVE_FLASH_PROG_DATA_OFFSET], p_ctx->fw_buff, data_len);

                p_ctx->uart_cmd_len = SLAVE_FLASH_PROG_DATA_OFFSET + data_len + CHECKSUM_SIZE;
            }
            else
            {
                // Normal page
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET] = (SLAVE_FLASH_PROG_PRAM_LEN_MAX >> 8) & 0xFF;
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] = SLAVE_FLASH_PROG_PRAM_LEN_MAX & 0xFF;

                memcpy(&uart_buff[SLAVE_FLASH_PROG_DATA_OFFSET], p_ctx->fw_buff, SLAVE_FLASH_PROG_SIZE);

                p_ctx->uart_cmd_len = SLAVE_FLASH_PROG_CMD_SIZE_MAX;
            }

            // Add CRC
            uart_buff[p_ctx->uart_cmd_len - CHECKSUM_SIZE] =
                ftd_mw_slave_manager_get_crc(uart_buff, (p_ctx->uart_cmd_len - CHECKSUM_SIZE));

            p_ctx->state = PROG_STATE_TX_STARTING;
            FTD_LOG_TRACE("CH%d: CMD prepared, len=%d, addr=0x%08x",
                p_ctx->channel, p_ctx->uart_cmd_len, p_ctx->slv_flash_addr);
            break;
        }

        case PROG_STATE_TX_STARTING:
        {
            // Clear RX buffer and ensure FIFO is clean before starting transfer
            memset(p_ctx->uart_rx_buff, 0x00, SLAVE_FLASH_PROG_ACK_LEN);

            // Double-clear FIFO for first packet to ensure no residual data
            if (p_ctx->fw_read_offset == 0 || p_ctx->retry_count > 0)
            {
                // Extra FIFO clear for first packet or retry
                ftd_drv_burn_uart_rx_stop(p_ctx->channel);
                CLK_SysTickLongDelay(1000); // 1ms
                FTD_LOG_TRACE("CH%d: Extra FIFO clear (first=%d, retry=%d)",
                    p_ctx->channel, (p_ctx->fw_read_offset == 0), p_ctx->retry_count);
            }
            // Start UART TX and RX DMA
            ftd_drv_burn_uart_rx_start(p_ctx->channel, p_ctx->uart_rx_buff, SLAVE_FLASH_PROG_ACK_LEN);
            ftd_drv_burn_uart_tx_start(p_ctx->channel, p_ctx->uart_tx_buff, p_ctx->uart_cmd_len);

            p_ctx->state = PROG_STATE_WAIT_TX;
            FTD_LOG_TRACE("CH%d: TX/RX started", p_ctx->channel);
            break;
        }

        case PROG_STATE_WAIT_TX:
        {
            // Check if TX DMA complete
            if (ftd_drv_burn_channel_is_tx_complete(p_ctx->channel))
            {
                p_ctx->state = PROG_STATE_WAIT_RX;
                p_ctx->rx_start_tick = ftd_mw_misc_manager_get_time_ms();
                FTD_LOG_TRACE("CH%d: TX done, waiting RX", p_ctx->channel);
            }
            // No blocking here, return false to allow other channels to run
            break;
        }

        case PROG_STATE_WAIT_RX:
        {
            // Check if RX DMA complete
            if (ftd_drv_burn_channel_is_rx_complete(p_ctx->channel))
            {
                ftd_drv_burn_uart_rx_stop(p_ctx->channel);
                p_ctx->state = PROG_STATE_CHECK_ACK;

                // Log received data for debugging (first 4 bytes)
                FTD_LOG_TRACE("CH%d: RX done, data=[%02X %02X %02X %02X]",
                    p_ctx->channel,
                    p_ctx->uart_rx_buff[0], p_ctx->uart_rx_buff[1],
                    p_ctx->uart_rx_buff[2], p_ctx->uart_rx_buff[3]);
            }
            else
            {
                // Check timeout (100ms)
                uint32_t elapsed = ftd_mw_misc_manager_get_time_ms() - p_ctx->rx_start_tick;
                if (elapsed > 100)
                {
                    ftd_drv_burn_uart_rx_stop(p_ctx->channel);
                    FTD_LOG_WARN("CH%d: RX timeout after %dms", p_ctx->channel, elapsed);
                    p_ctx->state = PROG_STATE_ERROR;
                    p_ctx->error_flag = true;
                }
            }
            break;
        }

        case PROG_STATE_CHECK_ACK:
        {
            // Check ACK response
            uint8_t ack = ftd_mw_slave_check_rx(p_ctx->uart_rx_buff, SLAVE_FLASH_PROG_ACK_LEN);
            if (SLAVE_CMD_WRITE_FLASH_DATA == ack)
            {
                FTD_LOG_TRACE("CH%d: ACK OK [0x%02x]", p_ctx->channel, ack);

                // Reset retry counter on success
                p_ctx->retry_count = 0;

                // Move to next packet
                p_ctx->fw_read_offset += SLAVE_FLASH_PROG_SIZE;
                p_ctx->slv_flash_addr += SLAVE_FLASH_PROG_SIZE;

                // Check if all packets sent
                if (p_ctx->fw_read_offset >= p_ctx->fw_size)
                {
                    p_ctx->state = PROG_STATE_COMPLETE;
                    FTD_LOG_TRACE("CH%d: Programming complete! Total size=0x%08x", p_ctx->channel, p_ctx->fw_size);
                    return true; // Complete
                }
                else
                {
                    // Continue to next packet
                    p_ctx->state = PROG_STATE_READ_FLASH;
                }
            }
            else
            {
                // ACK failed - check if we should retry
                if (p_ctx->retry_count < 3)  // Allow up to 3 retries
                {
                    p_ctx->retry_count++;
                    FTD_LOG_WARN("CH%d: ACK failed (retry %d/3), expected=0x%02x, got=0x%02x, retrying...",
                        p_ctx->channel, p_ctx->retry_count, SLAVE_CMD_WRITE_FLASH_DATA, ack);

                    // Add a small delay before retry
                    CLK_SysTickLongDelay(2000); // 2ms

                    // Retry from PREPARE_CMD (don't re-read flash, just resend)
                    p_ctx->state = PROG_STATE_PREPARE_CMD;
                }
                else
                {
                    // Max retries exceeded
                    FTD_LOG_ERROR("CH%d: ACK failed after %d retries, expected=0x%02x, got=0x%02x",
                        p_ctx->channel, p_ctx->retry_count, SLAVE_CMD_WRITE_FLASH_DATA, ack);
                    p_ctx->state = PROG_STATE_ERROR;
                    p_ctx->error_flag = true;
                }
            }
            break;
        }

        case PROG_STATE_COMPLETE:
            return true; // Complete

        case PROG_STATE_ERROR:
            return true; // Complete with error

        default:
            p_ctx->state = PROG_STATE_ERROR;
            p_ctx->error_flag = true;
            return true;
    }

    return false; // Still in progress
}

/**
 * @brief Program firmware with state machine (non-blocking, can run multiple channels concurrently)
 * @param p_st_slave_info Pointer to slave info
 * @param st_channel_burn_status Pointer to channel burn status
 * @return true if programming completed successfully, false if still in progress or error
 *
 * @note This function uses state machine to allow concurrent multi-channel programming.
 *       Return value meaning:
 *       - true: Programming completed SUCCESSFULLY
 *       - false: Still in progress OR error occurred
 *
 *       To distinguish between "in progress" and "error", use:
 *       - ftd_mw_slave_manager_program_sm_is_error(channel)
 *       - ftd_mw_slave_manager_program_sm_get_state(channel)
 *
 *       IMPORTANT: State is reset to IDLE after completion, so check error flag before it returns.
 */
bool ftd_mw_slave_manager_program_sm(SLAVE_INFO* p_st_slave_info, CHANNEL_BURN_STATUS* st_channel_burn_status)
{
    uint8_t channel = p_st_slave_info->slave_uart_channel;

    // Validate channel
    if (channel >= BURN_UART_CHANNELS)
    {
        FTD_LOG_ERROR("Invalid channel: %d", channel);
        return false;
    }

    PROG_CONTEXT* p_ctx = &g_prog_ctx[channel];

    // Initialize context on first call (state == IDLE or uninitialized)
    if (p_ctx->state == PROG_STATE_IDLE || p_ctx->fw_size == 0)
    {
        ftd_mw_slave_manager_program_ctx_init(p_ctx, p_st_slave_info, st_channel_burn_status);
        FTD_LOG_TRACE("CH%d: Program SM initialized, fw_size=0x%08x", channel, p_ctx->fw_size);
    }

    // Run state machine
    bool completed = ftd_mw_slave_manager_program_state_machine(p_ctx);

    if (completed)
    {
        bool success = (p_ctx->state == PROG_STATE_COMPLETE && !p_ctx->error_flag);
        FTD_LOG_TRACE("CH%d: Program SM %s", channel, success ? "SUCCESS" : "FAILED");

        // Keep error flag for application to check
        // But reset state for next use
        if (!success)
        {
            p_ctx->error_flag = true;  // Ensure error flag is set
        }

        p_ctx->state = PROG_STATE_IDLE;
        p_ctx->fw_size = 0;

        return success;
    }

    return false; // Still in progress, call again
}

/**
 * @brief Check if programming state machine is in error state
 * @param channel UART channel number (0-3)
 * @return true if in error state, false otherwise
 */
bool ftd_mw_slave_manager_program_sm_is_error(uint8_t channel)
{
    if (channel >= BURN_UART_CHANNELS)
        return false;

    return (g_prog_ctx[channel].state == PROG_STATE_ERROR || g_prog_ctx[channel].error_flag);
}

/**
 * @brief Get programming state machine current state
 * @param channel UART channel number (0-3)
 * @return Current state
 */
PROG_STATE ftd_mw_slave_manager_program_sm_get_state(uint8_t channel)
{
    if (channel >= BURN_UART_CHANNELS)
        return PROG_STATE_ERROR;

    return g_prog_ctx[channel].state;
}

/******************************************************************************
 * RAM Program State Machine Implementation
 ******************************************************************************/
 /**
 * @brief Initialize RAM program context for a channel
 * @param p_ctx Pointer to SM context
 * @param p_slave_info Pointer to slave info
 * @param p_burn_status Pointer to burn status
 */
static void ftd_mw_slave_manager_ram_prog_ctx_init(SM_CONTEXT* p_ctx,
    SLAVE_INFO* p_slave_info,
    CHANNEL_BURN_STATUS* p_burn_status)
{
    // Get chip flash map by chip_code
    SYS_INFO sys_info;
    ftd_mw_sys_info_manager_get(&sys_info);
    uint16_t chip_code = (uint16_t)sys_info.st_fw_info[p_burn_status->fw_num].chip_id;
    const CHIP_FLASH_ADDR_MAP* p_chip_map = ftd_mw_slave_get_chip_flash_map(chip_code);

    memset(p_ctx, 0, sizeof(SM_CONTEXT));
    p_ctx->state = RAM_SM_STATE_IDLE;
    p_ctx->operation = SM_OP_RAM_PROGRAM;
    p_ctx->channel = p_slave_info->slave_uart_channel;
    p_ctx->fw_size = FT_TEST_SLAVE_BIN_SIZE;
    p_ctx->fw_read_offset = 0;

    // Use FT test RAM address (store in slv_flash_addr field)
    p_ctx->slv_flash_addr = FT_TEST_SLAVE_BIN_ADDR;

    // Store chip identification
    p_ctx->chip_code = chip_code;
    p_ctx->fw_num = p_burn_status->fw_num;

    p_ctx->read_partition = PARTITION_FW1_BIN + p_burn_status->fw_num * (PARTITION_FW2_BIN - PARTITION_FW1_BIN);
    p_ctx->error_flag = false;
    p_ctx->retry_count = 0;

    // Initialize UART TX buffer header (fixed part)
    p_ctx->uart_tx_buff[0] = 0x4a;
    p_ctx->uart_tx_buff[1] = 0x59;
    p_ctx->uart_tx_buff[2] = 0x4d;
    p_ctx->uart_tx_buff[3] = 0x43;
    p_ctx->uart_tx_buff[4] = 0x54;

    // Clear UART RX buffer to avoid residual data
    memset(p_ctx->uart_rx_buff, 0x00, sizeof(p_ctx->uart_rx_buff));

    // Add a small delay to ensure slave is ready
    CLK_SysTickLongDelay(5000); // 5ms delay

    FTD_LOG_INFO("CH%d: RAM Program SM context initialized, Chip: %s (0x%04X), RAM Addr: 0x%08X",
        p_ctx->channel, p_chip_map->chip_name, p_ctx->chip_code, p_ctx->slv_flash_addr);
}

/**
 * @brief Process one state machine step for RAM program operation
 * @param p_ctx Pointer to SM context
 * @return true if operation completed (success or error), false if still in progress
 */
static bool ftd_mw_slave_manager_ram_prog_state_machine(SM_CONTEXT* p_ctx)
{
    switch (p_ctx->state)
    {
        case RAM_SM_STATE_IDLE:
            // Start state, transition to read flash
            p_ctx->state = RAM_SM_STATE_READ_FLASH;
            FTD_LOG_TRACE("CH%d: Start RAM programming, fw_size=0x%08x", p_ctx->channel, p_ctx->fw_size);
            break;

        case RAM_SM_STATE_READ_FLASH:
        {
            // Read firmware data from flash
            if (ftd_drv_fmc_read_data(FT_TEST_SLAVE_ADDR, p_ctx->fw_read_offset, p_ctx->fw_buff, SLAVE_FLASH_PROG_SIZE) == 0)
            {
                FTD_LOG_TRACE("CH%d: Flash read OK, offset=0x%08x", p_ctx->channel, p_ctx->fw_read_offset);
                p_ctx->state = RAM_SM_STATE_PREPARE_CMD;
            }
            else
            {
                FTD_LOG_ERROR("CH%d: Flash read failed at offset=0x%08x", p_ctx->channel, p_ctx->fw_read_offset);
                p_ctx->state = RAM_SM_STATE_ERROR;
                p_ctx->error_flag = true;
            }
            break;
        }

        case RAM_SM_STATE_PREPARE_CMD:
        {
            // Prepare UART command packet for RAM write (0x52 command)
            uint8_t* uart_buff = p_ctx->uart_tx_buff;

            // Log if this is a retry
            if (p_ctx->retry_count > 0)
            {
                FTD_LOG_TRACE("CH%d: Preparing RAM retry packet #%d, offset=0x%08x, addr=0x%08x",
                    p_ctx->channel, p_ctx->retry_count, p_ctx->fw_read_offset, p_ctx->slv_flash_addr);
            }

            // Extended command for RAM write
            uart_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET] = 0x00;
            uart_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET + 1] = 0x52;

            // Slave RAM address
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET] = (p_ctx->slv_flash_addr >> 24) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 1] = (p_ctx->slv_flash_addr >> 16) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 2] = (p_ctx->slv_flash_addr >> 8) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 3] = p_ctx->slv_flash_addr & 0xFF;

            // Check if last packet
            if (p_ctx->fw_read_offset + SLAVE_FLASH_PROG_SIZE > p_ctx->fw_size)
            {
                // Last page
                uint32_t data_len = p_ctx->fw_size % SLAVE_FLASH_PROG_SIZE;
                uint16_t parm_len = EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_OP_LENGTH_SIZE - 3 + data_len;

                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET] = (parm_len >> 8) & 0xFF;
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] = parm_len & 0xFF;

                memcpy(&uart_buff[SLAVE_FLASH_PROG_DATA_OFFSET - 3], p_ctx->fw_buff, data_len);

                p_ctx->uart_cmd_len = SLAVE_FLASH_PROG_DATA_OFFSET - 3 + data_len + CHECKSUM_SIZE;
            }
            else
            {
                // Normal page
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET] = ((SLAVE_FLASH_PROG_PRAM_LEN_MAX - 3) >> 8) & 0xFF;
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] = (SLAVE_FLASH_PROG_PRAM_LEN_MAX - 3) & 0xFF;

                memcpy(&uart_buff[SLAVE_FLASH_PROG_DATA_OFFSET - 3], p_ctx->fw_buff, SLAVE_FLASH_PROG_SIZE);

                p_ctx->uart_cmd_len = SLAVE_FLASH_PROG_CMD_SIZE_MAX - 3;
            }

            // Add CRC
            uart_buff[p_ctx->uart_cmd_len - CHECKSUM_SIZE] =
                ftd_mw_slave_manager_get_crc(uart_buff, (p_ctx->uart_cmd_len - CHECKSUM_SIZE));

            p_ctx->state = RAM_SM_STATE_TX_STARTING;
            FTD_LOG_TRACE("CH%d: RAM CMD prepared, len=%d, addr=0x%08x",
                p_ctx->channel, p_ctx->uart_cmd_len, p_ctx->slv_flash_addr);
            break;
        }

        case RAM_SM_STATE_TX_STARTING:
        {
            // Clear RX buffer and ensure FIFO is clean before starting transfer
            memset(p_ctx->uart_rx_buff, 0x00, SLAVE_FLASH_PROG_ACK_LEN);

            // Double-clear FIFO for first packet to ensure no residual data
            if (p_ctx->fw_read_offset == 0 || p_ctx->retry_count > 0)
            {
                // Extra FIFO clear for first packet or retry
                ftd_drv_burn_uart_rx_stop(p_ctx->channel);
                CLK_SysTickLongDelay(1000); // 1ms
                FTD_LOG_TRACE("CH%d: Extra FIFO clear (first=%d, retry=%d)",
                    p_ctx->channel, (p_ctx->fw_read_offset == 0), p_ctx->retry_count);
            }

            // Start UART TX and RX DMA
            ftd_drv_burn_uart_rx_start(p_ctx->channel, p_ctx->uart_rx_buff, SLAVE_FLASH_PROG_ACK_LEN);
            ftd_drv_burn_uart_tx_start(p_ctx->channel, p_ctx->uart_tx_buff, p_ctx->uart_cmd_len);

            p_ctx->state = RAM_SM_STATE_WAIT_TX;
            FTD_LOG_TRACE("CH%d: RAM TX/RX started", p_ctx->channel);
            break;
        }

        case RAM_SM_STATE_WAIT_TX:
        {
            // Check if TX DMA complete
            if (ftd_drv_burn_channel_is_tx_complete(p_ctx->channel))
            {
                p_ctx->state = RAM_SM_STATE_WAIT_RX;
                p_ctx->rx_start_tick = ftd_mw_misc_manager_get_time_ms();
                FTD_LOG_TRACE("CH%d: RAM TX done, waiting RX", p_ctx->channel);
            }
            // No blocking here, return false to allow other channels to run
            break;
        }

        case RAM_SM_STATE_WAIT_RX:
        {
            // Check if RX DMA complete
            if (ftd_drv_burn_channel_is_rx_complete(p_ctx->channel))
            {
                ftd_drv_burn_uart_rx_stop(p_ctx->channel);
                p_ctx->state = RAM_SM_STATE_CHECK_ACK;

                // Log received data for debugging (first 4 bytes)
                FTD_LOG_TRACE("CH%d: RAM RX done, data=[%02X %02X %02X %02X]",
                    p_ctx->channel,
                    p_ctx->uart_rx_buff[0], p_ctx->uart_rx_buff[1],
                    p_ctx->uart_rx_buff[2], p_ctx->uart_rx_buff[3]);
            }
            else
            {
                // Check timeout (100ms)
                uint32_t elapsed = ftd_mw_misc_manager_get_time_ms() - p_ctx->rx_start_tick;
                if (elapsed > 100)
                {
                    ftd_drv_burn_uart_rx_stop(p_ctx->channel);
                    FTD_LOG_WARN("CH%d: RAM RX timeout after %dms", p_ctx->channel, elapsed);
                    p_ctx->state = RAM_SM_STATE_ERROR;
                    p_ctx->error_flag = true;
                }
            }
            break;
        }

        case RAM_SM_STATE_CHECK_ACK:
        {
            // Check ACK response for RAM write (should be 0x52)
            uint8_t ack = ftd_mw_slave_check_rx(p_ctx->uart_rx_buff, SLAVE_FLASH_PROG_ACK_LEN);
            if (SLAVE_CMD_WRITE_RAM_DATA == ack)
            {
                FTD_LOG_TRACE("CH%d: RAM ACK OK [0x%02x]", p_ctx->channel, ack);

                // Reset retry counter on success
                p_ctx->retry_count = 0;

                // Move to next packet
                p_ctx->fw_read_offset += SLAVE_FLASH_PROG_SIZE;
                p_ctx->slv_flash_addr += SLAVE_FLASH_PROG_SIZE;

                // Check if all packets sent
                if (p_ctx->fw_read_offset >= p_ctx->fw_size)
                {
                    p_ctx->state = RAM_SM_STATE_COMPLETE;
                    FTD_LOG_TRACE("CH%d: RAM programming complete! Total size=0x%08x", p_ctx->channel, p_ctx->fw_size);
                    return true; // Complete
                }
                else
                {
                    // Continue to next packet
                    p_ctx->state = RAM_SM_STATE_READ_FLASH;
                }
            }
            else
            {
                // ACK failed - check if we should retry
                if (p_ctx->retry_count < 3)  // Allow up to 3 retries
                {
                    p_ctx->retry_count++;
                    FTD_LOG_WARN("CH%d: RAM ACK failed (retry %d/3), expected=0x%02x, got=0x%02x, retrying...",
                        p_ctx->channel, p_ctx->retry_count, SLAVE_CMD_WRITE_RAM_DATA, ack);

                    // Add a small delay before retry
                    CLK_SysTickLongDelay(2000); // 2ms

                    // Retry from PREPARE_CMD (don't re-read flash, just resend)
                    p_ctx->state = RAM_SM_STATE_PREPARE_CMD;
                }
                else
                {
                    // Max retries exceeded
                    FTD_LOG_ERROR("CH%d: RAM ACK failed after %d retries, expected=0x%02x, got=0x%02x",
                        p_ctx->channel, p_ctx->retry_count, SLAVE_CMD_WRITE_RAM_DATA, ack);
                    p_ctx->state = RAM_SM_STATE_ERROR;
                    p_ctx->error_flag = true;
                }
            }
            break;
        }

        case RAM_SM_STATE_COMPLETE:
            return true; // Complete

        case RAM_SM_STATE_ERROR:
            return true; // Complete with error

        default:
            p_ctx->state = RAM_SM_STATE_ERROR;
            p_ctx->error_flag = true;
            return true;
    }

    return false; // Still in progress
}

/**
 * @brief RAM program with state machine (non-blocking, can run multiple channels concurrently)
 * @param p_st_slave_info Pointer to slave info
 * @param st_channel_burn_status Pointer to channel burn status
 * @return true if programming completed successfully, false if still in progress or error
 */
bool ftd_mw_slave_ft_test_manager_program_test_sm(SLAVE_INFO* p_st_slave_info, CHANNEL_BURN_STATUS* st_channel_burn_status)
{
    uint8_t channel = p_st_slave_info->slave_uart_channel;

    // Validate channel
    if (channel >= BURN_UART_CHANNELS)
    {
        FTD_LOG_ERROR("Invalid channel: %d", channel);
        return false;
    }

    RAM_PROG_CONTEXT* p_ctx = &g_ram_prog_ctx[channel];

    // Initialize context on first call (state == IDLE or uninitialized)
    if (p_ctx->state == RAM_SM_STATE_IDLE || p_ctx->fw_size == 0)
    {
        ftd_mw_slave_manager_ram_prog_ctx_init(p_ctx, p_st_slave_info, st_channel_burn_status);
        FTD_LOG_TRACE("CH%d: RAM Program SM initialized, fw_size=0x%08x", channel, p_ctx->fw_size);
    }

    // Run state machine
    bool completed = ftd_mw_slave_manager_ram_prog_state_machine(p_ctx);

    if (completed)
    {
        bool success = (p_ctx->state == RAM_SM_STATE_COMPLETE && !p_ctx->error_flag);
        FTD_LOG_TRACE("CH%d: RAM Program SM %s", channel, success ? "SUCCESS" : "FAILED");

        // Keep error flag for application to check
        // But reset state for next use
        if (!success)
        {
            p_ctx->error_flag = true;  // Ensure error flag is set
        }

        p_ctx->state = RAM_SM_STATE_IDLE;
        p_ctx->fw_size = 0;

        return success;
    }

    return false; // Still in progress, call again
}

// Keep error flag for application to check

/**
 * @brief Check if RAM programming state machine is in error state
 * @param channel UART channel number (0-3)
 * @return true if in error state, false otherwise
 */
bool ftd_mw_slave_ft_test_manager_program_test_sm_is_error(uint8_t channel)
{
    if (channel >= BURN_UART_CHANNELS)
        return false;

    return (g_ram_prog_ctx[channel].state == RAM_SM_STATE_ERROR || g_ram_prog_ctx[channel].error_flag);
}

/**
 * @brief Get RAM programming state machine current state
 * @param channel UART channel number (0-3)
 * @return Current state
 */
RAM_SM_STATE ftd_mw_slave_ft_test_manager_program_test_sm_get_state(uint8_t channel)
{
    if (channel >= BURN_UART_CHANNELS)
        return RAM_SM_STATE_ERROR;

    return g_ram_prog_ctx[channel].state;
}

/******************************************************************************
 * Jump State Machine Implementation
 ******************************************************************************/

 /**
  * @brief Initialize Jump context for a channel
  * @param p_ctx Pointer to Jump context
  * @param p_slave_info Pointer to slave info
  * @param jump_addr Jump address
  */
static void ftd_mw_slave_manager_jump_ctx_init(JUMP_SM_CONTEXT* p_ctx, SLAVE_INFO* p_slave_info, uint32_t jump_addr)
{
    memset(p_ctx, 0, sizeof(JUMP_SM_CONTEXT));
    p_ctx->state = JUMP_SM_STATE_IDLE;
    p_ctx->operation = SM_OP_NONE;
    p_ctx->channel = p_slave_info->slave_uart_channel;
    p_ctx->slv_flash_addr = jump_addr;
    p_ctx->error_flag = false;
    p_ctx->retry_count = 0;

    // Clear UART RX buffer to avoid residual data
    memset(p_ctx->uart_rx_buff, 0x00, sizeof(p_ctx->uart_rx_buff));

    FTD_LOG_INFO("CH%d: Jump SM context initialized, Jump Addr: 0x%08X",
        p_ctx->channel, p_ctx->slv_flash_addr);
}

/**
 * @brief Process one state machine step for Jump operation
 * @param p_ctx Pointer to Jump context
 * @return true if operation completed (success or error), false if still in progress
 */
static bool ftd_mw_slave_manager_jump_state_machine(JUMP_SM_CONTEXT* p_ctx)
{
    switch (p_ctx->state)
    {
        case JUMP_SM_STATE_IDLE:
            // Start state, transition to prepare command
            p_ctx->state = JUMP_SM_STATE_PREPARE_CMD;
            FTD_LOG_TRACE("CH%d: Start Jump state machine, jump_addr=0x%08x", p_ctx->channel, p_ctx->slv_flash_addr);
            break;

        case JUMP_SM_STATE_PREPARE_CMD:
        {
            // Prepare UART command packet for program jump (0x53 command)
            uint8_t* uart_buff = p_ctx->uart_tx_buff;

            // JYMC header
            uart_buff[0] = 0x4a;
            uart_buff[1] = 0x59;
            uart_buff[2] = 0x4d;
            uart_buff[3] = 0x43;
            uart_buff[4] = 0x54;

            // Parameter length: 6 (ext_cmd 2 + addr 4)
            uart_buff[5] = 0x00;
            uart_buff[6] = 0x06;

            // Extended command for program jump
            uart_buff[7] = 0x00;
            uart_buff[8] = 0x53;

            // Jump address
            uart_buff[9] = (p_ctx->slv_flash_addr >> 24) & 0xFF;
            uart_buff[10] = (p_ctx->slv_flash_addr >> 16) & 0xFF;
            uart_buff[11] = (p_ctx->slv_flash_addr >> 8) & 0xFF;
            uart_buff[12] = p_ctx->slv_flash_addr & 0xFF;

            // CRC (XOR of all previous bytes)
            p_ctx->uart_cmd_len = 14; // 13 bytes + 1 CRC
            uart_buff[13] = ftd_mw_slave_manager_get_crc(uart_buff, 13);

            p_ctx->state = JUMP_SM_STATE_TX_STARTING;
            FTD_LOG_TRACE("CH%d: Jump CMD prepared, len=%d, addr=0x%08x",
                p_ctx->channel, p_ctx->uart_cmd_len, p_ctx->slv_flash_addr);
            break;
        }

        case JUMP_SM_STATE_TX_STARTING:
        {
            // Clear RX buffer before starting transfer
            memset(p_ctx->uart_rx_buff, 0x00, SLAVE_FLASH_PROG_ACK_LEN);

            // Start UART TX and RX DMA
            ftd_drv_burn_uart_rx_start(p_ctx->channel, p_ctx->uart_rx_buff, SLAVE_FLASH_PROG_ACK_LEN);
            ftd_drv_burn_uart_tx_start(p_ctx->channel, p_ctx->uart_tx_buff, p_ctx->uart_cmd_len);

            p_ctx->state = JUMP_SM_STATE_WAIT_TX;
            FTD_LOG_TRACE("CH%d: Jump TX/RX started", p_ctx->channel);
            break;
        }

        case JUMP_SM_STATE_WAIT_TX:
        {
            // Check if TX DMA complete
            if (ftd_drv_burn_channel_is_tx_complete(p_ctx->channel))
            {
                p_ctx->state = JUMP_SM_STATE_WAIT_RX;
                p_ctx->rx_start_tick = ftd_mw_misc_manager_get_time_ms();
                FTD_LOG_TRACE("CH%d: Jump TX done, waiting RX", p_ctx->channel);
            }
            break;
        }

        case JUMP_SM_STATE_WAIT_RX:
        {
            // Check if RX DMA complete
            if (ftd_drv_burn_channel_is_rx_complete(p_ctx->channel))
            {
                ftd_drv_burn_uart_rx_stop(p_ctx->channel);
                p_ctx->state = JUMP_SM_STATE_CHECK_ACK;

                FTD_LOG_TRACE("CH%d: Jump RX done, data=[%02X %02X %02X %02X]",
                    p_ctx->channel,
                    p_ctx->uart_rx_buff[0], p_ctx->uart_rx_buff[1],
                    p_ctx->uart_rx_buff[2], p_ctx->uart_rx_buff[3]);
            }
            else
            {
                // Check timeout (30ms)
                uint32_t elapsed = ftd_mw_misc_manager_get_time_ms() - p_ctx->rx_start_tick;
                if (elapsed > 30)
                {
                    ftd_drv_burn_uart_rx_stop(p_ctx->channel);
                    FTD_LOG_WARN("CH%d: Jump RX timeout after %dms", p_ctx->channel, elapsed);
                    p_ctx->state = JUMP_SM_STATE_ERROR;
                    p_ctx->error_flag = true;
                }
            }
            break;
        }

        case JUMP_SM_STATE_CHECK_ACK:
        {
            // Check ACK response for program jump (should be 0x53)
            uint8_t ack = ftd_mw_slave_check_rx(p_ctx->uart_rx_buff, SLAVE_FLASH_PROG_ACK_LEN);
            if (SLAVE_CMD_SET_PROGRAM_JUMP == ack)
            {
                FTD_LOG_TRACE("CH%d: Jump ACK OK [0x%02x], waiting for calibration", p_ctx->channel, ack);
                // Move to wait calibration state
                p_ctx->state = JUMP_SM_STATE_WAIT_CALIBRATION;
                p_ctx->rx_start_tick = ftd_mw_misc_manager_get_time_ms();

                // Start RX for calibration ACK (2 bytes)
                memset(p_ctx->uart_rx_buff, 0x00, 2);
                ftd_drv_burn_uart_rx_start(p_ctx->channel, p_ctx->uart_rx_buff, 2);
            }
            else
            {
                FTD_LOG_ERROR("CH%d: Jump ACK failed, expected=0x%02x, got=0x%02x",
                    p_ctx->channel, SLAVE_CMD_SET_PROGRAM_JUMP, ack);
                p_ctx->state = JUMP_SM_STATE_ERROR;
                p_ctx->error_flag = true;
            }
            break;
        }

        case JUMP_SM_STATE_WAIT_CALIBRATION:
        {
            // Check if RX DMA complete for calibration ACK
            if (ftd_drv_burn_channel_is_rx_complete(p_ctx->channel))
            {
                ftd_drv_burn_uart_rx_stop(p_ctx->channel);

                uint8_t calib_ack = ftd_mw_slave_check_rx(p_ctx->uart_rx_buff, 2);
                if (SLAVE_CMD_WAIT_CALIBRATION == calib_ack)
                {
                    FTD_LOG_TRACE("CH%d: Calibration ACK OK [0x%02x]", p_ctx->channel, calib_ack);
                    p_ctx->state = JUMP_SM_STATE_COMPLETE;
                    return true; // Complete successfully
                }
                else
                {
                    FTD_LOG_ERROR("CH%d: Calibration ACK failed, expected=0x%02x, got=0x%02x",
                        p_ctx->channel, SLAVE_CMD_WAIT_CALIBRATION, calib_ack);
                    p_ctx->state = JUMP_SM_STATE_ERROR;
                    p_ctx->error_flag = true;
                }
            }
            else
            {
                // Check timeout (5000ms = 5s)
                uint32_t elapsed = ftd_mw_misc_manager_get_time_ms() - p_ctx->rx_start_tick;
                if (elapsed > 10000)
                {
                    ftd_drv_burn_uart_rx_stop(p_ctx->channel);
                    FTD_LOG_WARN("CH%d: Calibration wait timeout after %dms", p_ctx->channel, elapsed);
                    p_ctx->state = JUMP_SM_STATE_ERROR;
                    p_ctx->error_flag = true;
                }
            }
            break;
        }

        case JUMP_SM_STATE_COMPLETE:
            return true; // Complete

        case JUMP_SM_STATE_ERROR:
            return true; // Complete with error

        default:
            p_ctx->state = JUMP_SM_STATE_ERROR;
            p_ctx->error_flag = true;
            return true;
    }

    return false; // Still in progress
}

/**
 * @brief Program jump with state machine (non-blocking)
 * @param p_st_slave_info Pointer to slave info
 * @param jump_addr Jump address
 * @return true if jump completed successfully, false if still in progress or error
 */
bool ftd_mw_slave_manager_program_jump_sm(SLAVE_INFO* p_st_slave_info, uint32_t jump_addr)
{
    uint8_t channel = p_st_slave_info->slave_uart_channel;

    // Validate channel
    if (channel >= BURN_UART_CHANNELS)
    {
        FTD_LOG_ERROR("Invalid channel: %d", channel);
        return false;
    }

    JUMP_SM_CONTEXT* p_ctx = &g_jump_ctx[channel];

    // Initialize context on first call (state == IDLE or uninitialized)
    if (p_ctx->state == JUMP_SM_STATE_IDLE || p_ctx->slv_flash_addr != jump_addr)
    {
        ftd_mw_slave_manager_jump_ctx_init(p_ctx, p_st_slave_info, jump_addr);
        FTD_LOG_TRACE("CH%d: Jump SM initialized, jump_addr=0x%08x", channel, jump_addr);
    }

    // Run state machine
    bool completed = ftd_mw_slave_manager_jump_state_machine(p_ctx);

    if (completed)
    {
        bool success = (p_ctx->state == JUMP_SM_STATE_COMPLETE && !p_ctx->error_flag);
        FTD_LOG_TRACE("CH%d: Jump SM %s", channel, success ? "SUCCESS" : "FAILED");

        // Reset state for next use
        p_ctx->state = JUMP_SM_STATE_IDLE;

        return success;
    }

    return false; // Still in progress, call again
}

/**
 * @brief Check if Jump state machine is in error state
 * @param channel UART channel number (0-3)
 * @return true if in error state, false otherwise
 */
bool ftd_mw_slave_manager_program_jump_sm_is_error(uint8_t channel)
{
    if (channel >= BURN_UART_CHANNELS)
        return false;

    return (g_jump_ctx[channel].state == JUMP_SM_STATE_ERROR || g_jump_ctx[channel].error_flag);
}

/**
 * @brief Get Jump state machine current state
 * @param channel UART channel number (0-3)
 * @return Current state
 */
JUMP_SM_STATE ftd_mw_slave_manager_program_jump_sm_get_state(uint8_t channel)
{
    if (channel >= BURN_UART_CHANNELS)
        return JUMP_SM_STATE_ERROR;

    return g_jump_ctx[channel].state;
}


/******************************************************************************
 * CRC Check State Machine Implementation
 ******************************************************************************/

 /**
*@brief Initialize CRC check context for a channel
*@param p_ctx Pointer to CRC context
*@param p_slave_info Pointer to slave info
*@param p_burn_status Pointer to burn status(for chip_id lookup)
*/
static void ftd_mw_slave_manager_crc_ctx_init(CRC_CONTEXT* p_ctx, SLAVE_INFO* p_slave_info, CHANNEL_BURN_STATUS* p_burn_status)
{
    SYS_INFO st_sys_info;
    ftd_mw_sys_info_manager_get(&st_sys_info);

    uint8_t fw_num = p_burn_status ? p_burn_status->fw_num : 0;

    // Get chip flash map by chip_code
    uint16_t chip_code = (uint16_t)st_sys_info.st_fw_info[fw_num].chip_id;
    const CHIP_FLASH_ADDR_MAP* p_chip_map = ftd_mw_slave_get_chip_flash_map(chip_code);

    memset(p_ctx, 0, sizeof(CRC_CONTEXT));
    p_ctx->state = CRC_STATE_IDLE;
    p_ctx->operation = SM_OP_CRC_CHECK;
    p_ctx->channel = p_slave_info->slave_uart_channel;
    p_ctx->fw_size = p_slave_info->flash_bin_size;
    p_ctx->fw_read_offset = 0;

    // Use dynamic address from chip flash map instead of hardcoded macro
    p_ctx->slv_flash_addr = p_chip_map->fw_bin_a_addr;

    // Store chip identification
    p_ctx->chip_code = chip_code;
    p_ctx->chip_family = p_chip_map->chip_family;
    p_ctx->fw_num = fw_num;

    p_ctx->calc_crc = 0xFFFF;  // CRC init value
    p_ctx->expect_crc = st_sys_info.st_fw_info[fw_num].st_bin_info.bin_crc16;
    p_ctx->error_flag = false;
    p_ctx->crc_pass = false;
    p_ctx->retry_count = 0;

    // Initialize UART TX buffer header (fixed part)
    p_ctx->uart_tx_buff[0] = 0x4a;
    p_ctx->uart_tx_buff[1] = 0x59;
    p_ctx->uart_tx_buff[2] = 0x4d;
    p_ctx->uart_tx_buff[3] = 0x43;
    p_ctx->uart_tx_buff[4] = 0x54;

    // Clear RX buffer
    memset(p_ctx->uart_rx_buff, 0x00, sizeof(p_ctx->uart_rx_buff));

    FTD_LOG_INFO("CH%d: CRC SM context initialized, Chip: %s (0x%04X), FW Addr: 0x%08X, expect_crc=0x%04x",
        p_ctx->channel, p_chip_map->chip_name, p_ctx->chip_code, p_ctx->slv_flash_addr, p_ctx->expect_crc);
}

/**
 * @brief CRC check state machine core function (non-blocking)
 * @param p_ctx Pointer to CRC context
 * @return true if completed (success or error), false if still in progress
 */
static bool ftd_mw_slave_manager_crc_state_machine(CRC_CONTEXT* p_ctx)
{
    static int cut = 0;
    switch (p_ctx->state)
    {
        case CRC_STATE_IDLE:
            // Start - transition to prepare command
            p_ctx->state = CRC_STATE_PREPARE_CMD;
            FTD_LOG_TRACE("CH%d: Start CRC check, fw_size=0x%08x", p_ctx->channel, p_ctx->fw_size);
            break;

        case CRC_STATE_PREPARE_CMD:
        {
            // Prepare read command
            uint8_t* uart_buff = p_ctx->uart_tx_buff;

            // Calculate read length for this packet
            if (p_ctx->fw_read_offset + SLAVE_FLASH_READ_SIZE > p_ctx->fw_size)
            {
                p_ctx->read_len = p_ctx->fw_size % SLAVE_FLASH_READ_SIZE;
                if (p_ctx->read_len == 0) p_ctx->read_len = SLAVE_FLASH_READ_SIZE;
            }
            else
            {
                p_ctx->read_len = SLAVE_FLASH_READ_SIZE;
            }

            // Parameter length
            uint16_t parm_len = EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_READ_LENGTH_SIZE;
            uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET] = (parm_len >> 8) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] = parm_len & 0xFF;

            // Extended command
            uart_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET] = (SLAVE_EXT_CMD_READ_FLASH >> 8) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET + 1] = SLAVE_EXT_CMD_READ_FLASH & 0xFF;

            // Slave flash address
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET] = (p_ctx->slv_flash_addr >> 24) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 1] = (p_ctx->slv_flash_addr >> 16) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 2] = (p_ctx->slv_flash_addr >> 8) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 3] = p_ctx->slv_flash_addr & 0xFF;

            // Read length (4 bytes)
            uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET] = (p_ctx->read_len >> 24) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 1] = (p_ctx->read_len >> 16) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 2] = (p_ctx->read_len >> 8) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 3] = p_ctx->read_len & 0xFF;

            p_ctx->uart_cmd_len = SLAVE_FLASH_READ_DATA_OFFSET + CHECKSUM_SIZE;

            // Add CRC
            uart_buff[p_ctx->uart_cmd_len - CHECKSUM_SIZE] =
                ftd_mw_slave_manager_get_crc(uart_buff, (p_ctx->uart_cmd_len - CHECKSUM_SIZE));

            p_ctx->state = CRC_STATE_TX_STARTING;
            FTD_LOG_TRACE("CH%d: CRC cmd prepared, addr=0x%08x, len=%d",
                p_ctx->channel, p_ctx->slv_flash_addr, p_ctx->read_len);
            break;
        }

        case CRC_STATE_TX_STARTING:
        {
            // Clear RX buffer
            memset(p_ctx->uart_rx_buff, 0x00, sizeof(p_ctx->uart_rx_buff));

            // Start RX first, then TX
            uint16_t rx_len = SLAVE_FLASH_READ_CMD_SIZE_MAX + p_ctx->read_len;
            ftd_drv_burn_uart_rx_start(p_ctx->channel, p_ctx->uart_rx_buff, rx_len);
            ftd_drv_burn_uart_tx_start(p_ctx->channel, p_ctx->uart_tx_buff, p_ctx->uart_cmd_len);

            p_ctx->state = CRC_STATE_WAIT_TX;
            FTD_LOG_TRACE("CH%d: CRC TX/RX started", p_ctx->channel);
            break;
        }

        case CRC_STATE_WAIT_TX:
        {
            // Check TX complete (non-blocking)
            if (ftd_drv_burn_channel_is_tx_complete(p_ctx->channel))
            {
                p_ctx->state = CRC_STATE_WAIT_RX;
                p_ctx->rx_start_tick = ftd_mw_misc_manager_get_time_ms();
                FTD_LOG_TRACE("CH%d: CRC TX done, waiting RX", p_ctx->channel);
            }
            break;
        }

        case CRC_STATE_WAIT_RX:
        {
            // Check RX complete (non-blocking)
            if (ftd_drv_burn_channel_is_rx_complete(p_ctx->channel))
            {
                ftd_drv_burn_uart_rx_stop(p_ctx->channel);
                p_ctx->state = CRC_STATE_CHECK_ACK;
                FTD_LOG_TRACE("CH%d: CRC RX done", p_ctx->channel);
            }
            else
            {
                // Timeout check (200ms for read operation)
                uint32_t elapsed = ftd_mw_misc_manager_get_time_ms() - p_ctx->rx_start_tick;
                if (elapsed > 800)
                {
                    ftd_drv_burn_uart_rx_stop(p_ctx->channel);

                    if (p_ctx->retry_count < 3)
                    {
                        p_ctx->retry_count++;
                        FTD_LOG_WARN("CH%d: CRC RX timeout (retry %d/3)", p_ctx->channel, p_ctx->retry_count);
                        p_ctx->state = CRC_STATE_PREPARE_CMD;
                    }
                    else
                    {
                        FTD_LOG_ERROR("CH%d: CRC RX timeout after %d retries", p_ctx->channel, p_ctx->retry_count);
                        p_ctx->state = CRC_STATE_ERROR;
                        p_ctx->error_flag = true;
                    }
                }
            }
            break;
        }

        case CRC_STATE_CHECK_ACK:
        {
            uint16_t rx_len = SLAVE_FLASH_READ_CMD_SIZE_MAX + p_ctx->read_len;
            uint8_t ack = ftd_mw_slave_check_rx(p_ctx->uart_rx_buff, rx_len);

            if (SLAVE_CMD_READ_CODE_CRC == ack)
            {
                // ACK OK - update CRC
                p_ctx->retry_count = 0;

                // Calculate CRC from received data
                // Data starts at uart_cmd_len - CHECKSUM_SIZE position
                // uart_cmd_len = SLAVE_FLASH_READ_DATA_OFFSET + CHECKSUM_SIZE
                // So data offset = SLAVE_FLASH_READ_DATA_OFFSET
                uint16_t data_offset = p_ctx->uart_cmd_len - CHECKSUM_SIZE;
                p_ctx->calc_crc = crc16_ccitt_update(p_ctx->calc_crc,
                    &p_ctx->uart_rx_buff[data_offset], p_ctx->read_len);
                FTD_LOG_TRACE("CH%d: CRC ACK OK, offset=0x%08x, calc_crc=0x%04x",
                    p_ctx->channel, p_ctx->fw_read_offset, p_ctx->calc_crc);

                // Move to next block
                p_ctx->fw_read_offset += p_ctx->read_len;
                p_ctx->slv_flash_addr += p_ctx->read_len;

                // Check if all data read
                if (p_ctx->fw_read_offset >= p_ctx->fw_size)
                {
                    p_ctx->state = CRC_STATE_VERIFY_CRC;
                }
                else
                {
                    p_ctx->state = CRC_STATE_PREPARE_CMD;
                }
            }
            else
            {
                // ACK failed
                if (p_ctx->retry_count < 3)
                {
                    p_ctx->retry_count++;
                    FTD_LOG_WARN("CH%d: CRC ACK failed (retry %d/3), expected=0x%02x, got=0x%02x",
                        p_ctx->channel, p_ctx->retry_count, SLAVE_CMD_READ_CODE_CRC, ack);
                    CLK_SysTickLongDelay(2000);
                    p_ctx->state = CRC_STATE_PREPARE_CMD;
                }
                else
                {
                    FTD_LOG_ERROR("CH%d: CRC ACK failed after %d retries", p_ctx->channel, p_ctx->retry_count);
                    p_ctx->state = CRC_STATE_ERROR;
                    p_ctx->error_flag = true;
                }
            }
            break;
        }

        case CRC_STATE_VERIFY_CRC:
        {
            // Final CRC comparison
            if (p_ctx->calc_crc == p_ctx->expect_crc)
            {
                p_ctx->crc_pass = true;
                FTD_LOG_INFO("CH%d: CRC CHECK PASS, calc=0x%04x, expect=0x%04x",
                    p_ctx->channel, p_ctx->calc_crc, p_ctx->expect_crc);
            }
            else
            {
                p_ctx->crc_pass = false;
                FTD_LOG_ERROR("CH%d: CRC CHECK FAIL, calc=0x%04x, expect=0x%04x",
                    p_ctx->channel, p_ctx->calc_crc, p_ctx->expect_crc);
            }
            p_ctx->state = CRC_STATE_COMPLETE;
            return true;
        }

        case CRC_STATE_COMPLETE:
            return true;

        case CRC_STATE_ERROR:
            return true;

        default:
            p_ctx->state = CRC_STATE_ERROR;
            p_ctx->error_flag = true;
            return true;
    }

    return false;  // Still in progress
}

/**
 * @brief CRC check with state machine (non-blocking, for concurrent multi-channel)
 * @param p_st_slave_info Pointer to slave info
 * @param p_burn_status Pointer to burn status (for chip_id lookup)
 * @return true if completed successfully, false if still in progress or error
 */
bool ftd_mw_slave_manager_read_crc_sm(SLAVE_INFO* p_st_slave_info, CHANNEL_BURN_STATUS* p_burn_status)
{
    uint8_t channel = p_st_slave_info->slave_uart_channel;

    if (channel >= BURN_UART_CHANNELS)
    {
        FTD_LOG_ERROR("CRC SM: Invalid channel %d", channel);
        return false;
    }

    CRC_CONTEXT* p_ctx = &g_crc_ctx[channel];

    // Initialize on first call
    if (p_ctx->state == CRC_STATE_IDLE || p_ctx->fw_size == 0)
    {
        ftd_mw_slave_manager_crc_ctx_init(p_ctx, p_st_slave_info, p_burn_status);
    }

    // Run state machine
    bool completed = ftd_mw_slave_manager_crc_state_machine(p_ctx);

    if (completed)
    {
        bool success = (p_ctx->state == CRC_STATE_COMPLETE && p_ctx->crc_pass && !p_ctx->error_flag);
        FTD_LOG_INFO("CH%d: CRC SM %s", channel, success ? "PASS" : "FAIL");

        // Keep result for application to check
        if (!success)
        {
            p_ctx->error_flag = true;
        }

        // Reset state for next use
        p_ctx->state = CRC_STATE_IDLE;
        p_ctx->fw_size = 0;

        return success;
    }

    return false;  // Still in progress
}

/**
 * @brief Check if CRC state machine is in error state
 * @param channel UART channel number (0-3)
 * @return true if error, false otherwise
 */
bool ftd_mw_slave_manager_read_crc_sm_is_error(uint8_t channel)
{
    if (channel >= BURN_UART_CHANNELS)
        return false;

    return g_crc_ctx[channel].error_flag;
}

/**
 * @brief Get CRC check result
 * @param channel UART channel number (0-3)
 * @return true if CRC passed, false if failed or not complete
 */
bool ftd_mw_slave_manager_read_crc_sm_get_result(uint8_t channel)
{
    if (channel >= BURN_UART_CHANNELS)
        return false;

    return g_crc_ctx[channel].crc_pass;
}

