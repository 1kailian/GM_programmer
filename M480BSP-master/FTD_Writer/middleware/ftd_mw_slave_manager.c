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
#include "ftd_drv_gpio.h"
#include "ftd_drv_burn_uart.h"
#include "ftd_drv_flash_access.h"
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
    while (index > 11)
    {
        // check header: 0x4A 0x59 0x4D 0x43 
        if ((rx_buffer[0] == 0x4A) && (rx_buffer[1] == 0x59) && (rx_buffer[2] == 0x4D) && (rx_buffer[3] == 0x43))
        {
            ret = 0xFF;

            // in bootloader mode?
            if ((rx_buffer[4] == 0xD4) && (rx_buffer[5] == 0x00) && (rx_buffer[6] == 0x03) && (rx_buffer[7] == 0x00)
                && (rx_buffer[8] == 0x47) && (rx_buffer[9] == 0x01) && (rx_buffer[10] == 0x8C)
                )
            {
                FTD_LOG_TRACE("SLAVE TRIGGER IN BOOTLOADER");
                FTD_LOG_TRACE_BUFF(rx_buffer, sizeof(SLAVE_CHN1_FUFF), 11);
                ret = SLAVE_CMD_ENTER_BOOT_LOADER;
            }
            #if 0
            // reset flash??
            if ((rx_buffer[4] == 0xD4) && (rx_buffer[5] == 0x00) && (rx_buffer[6] == 0x03) && (rx_buffer[7] == 0x00)
                && (rx_buffer[8] == 0x47) && (rx_buffer[9] == 0x01) && (rx_buffer[10] == 0x8C)
                )
            {
                FTD_LOG_TRACE("SLAVE TRIGGER IN BOOTLOADER");
                FTD_LOG_TRACE_BUFF(rx_buffer, sizeof(SLAVE_CHN1_FUFF), 11);
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
                    FTD_LOG_TRACE_BUFF(rx_buffer, sizeof(SLAVE_CHN1_FUFF), SLAVE_FLASH_PROG_ACK_LEN);
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
                    FTD_LOG_TRACE_BUFF(rx_buffer, sizeof(SLAVE_CHN1_FUFF), 14);
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
    uint8_t boot_loader_trigeer_cmd[2] = { 0x55, 0x96 }; //uart protocol
    uint8_t wait_time = 0;

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
            ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, boot_loader_trigeer_cmd, sizeof(boot_loader_trigeer_cmd));

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

bool ftd_mw_slave_manager_modify_baud_rate(SLAVE_INFO* p_st_slave_info)
{
    bool    ret = false;
    uint8_t wait_time = 0;
    uint8_t send_cmd_buff[14] =
    { 0x4a, 0x59, 0x4d, 0x43,
        0x54,
        0x00, 0x06,
        0x00, 0x4E,
        0x00, 0x07,
        0x08, 0x00,
        0xFF }; //uart protocol

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
    }
    else
    {
        FTD_LOG_DEBUG("ch:%d slave modify baud rate unack", p_st_slave_info->slave_uart_channel);
        ret = false;
    }


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
        0x00, 0x70,
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

bool ftd_mw_slave_manager_erase_test(SLAVE_INFO* p_st_slave_info)
{
    bool    ret = false;
    uint32_t fw_size;
    uint8_t wait_time;
    uint8_t boot_loader_trigeer_cmd[18] =
    { 0x4a, 0x59, 0x4d, 0x43,
        0x54,
        0x00, 0x0a,
        0x00, 0x4f,
        0x08, 0x02, 0x10, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x87 }; //uart protocol


    // set slave flash start addr
    if (0x00 == burn_test_addr)
    {
        boot_loader_trigeer_cmd[9]  = (SLAVE_FLASH_BIN_ADDR >> 24) & 0xFF;
        boot_loader_trigeer_cmd[10] = (SLAVE_FLASH_BIN_ADDR >> 16) & 0xFF;
        boot_loader_trigeer_cmd[11] = (SLAVE_FLASH_BIN_ADDR >> 8) & 0xFF;
        boot_loader_trigeer_cmd[12] = SLAVE_FLASH_BIN_ADDR & 0xFF;
    }
    else
    {
        boot_loader_trigeer_cmd[9]  = (SLAVE_FLASH_START_ADDR >> 24) & 0xFF;
        boot_loader_trigeer_cmd[10] = (SLAVE_FLASH_START_ADDR >> 16) & 0xFF;
        boot_loader_trigeer_cmd[11] = (SLAVE_FLASH_START_ADDR >> 8) & 0xFF;
        boot_loader_trigeer_cmd[12] = SLAVE_FLASH_START_ADDR & 0xFF;
    }

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
        FTD_LOG_DEBUG("ch:%d slave erase unack", p_st_slave_info->slave_uart_channel);
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

    fw_size = p_st_slave_info->flash_bin_size;
    slv_addr = SLAVE_FLASH_CHIP_0_MAP_0_FW_BIN_A_ADDR;

    FTD_LOG_TRACE("fw_size:0x%08x last_pkt_valid_data_len:0x%08x", fw_size, (fw_size % SLAVE_FLASH_PROG_SIZE));

    read_partition = PARTITION_FW1_BIN + st_channel_burn_status->fw_num * (PARTITION_FW2_BIN - PARTITION_FW1_BIN);

    for (fw_read_length = 0; fw_read_length < fw_size; fw_read_length += SLAVE_FLASH_PROG_SIZE)
    {
        FTD_LOG_TRACE("read addr:0x%08x len:0x%08x slv_addr:0x%08x ", PARTITION_FW1_BIN, fw_read_length, slv_addr);
        if (ftd_drv_flash_operation((PARTITION_NAME)read_partition, OPERATION_READ, fw_read_length, SLAVE_FLASH_PROG_SIZE, fw_buff) == 0)
        {
            // send uart pkt
            // ext cmd
            uart_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET]      = 0x00;
            uart_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET + 1]  = 0x51;
            // slave flash addr
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET]         = (slv_addr >> 24) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 1]     = (slv_addr >> 16) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 2]     = (slv_addr >> 8) & 0xFF;
            uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 3]     = slv_addr & 0xFF;
            // data length (fixed 0x00 for alain)
            uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET]          = 0x00;
            uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 1]      = 0x00;
            uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 2]      = 0x00;

            if (fw_read_length + SLAVE_FLASH_PROG_SIZE > fw_size)
            {
                // last page
                //FTD_LOG_TRACE_BUFF(fw_buff, sizeof(fw_buff), (fw_size % SLAVE_FLASH_PROG_SIZE));

                uint32_t data_len = fw_size % SLAVE_FLASH_PROG_SIZE;
                uint16_t parm_len = EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_OP_LENGTH_SIZE + data_len; // 4 + 4 + 2 + len

                // calc pram len
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET]     = (parm_len >> 8) & 0xFF;
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] = parm_len & 0xFF;

                // copy bin
                memcpy(&uart_buff[SLAVE_FLASH_PROG_DATA_OFFSET], fw_buff, data_len);

                uart_cmd_ttl_len = SLAVE_FLASH_PROG_DATA_OFFSET + data_len + CHECKSUM_SIZE;
            }
            else
            {
                // calc pram len
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET]     = (SLAVE_FLASH_PROG_PRAM_LEN_MAX >> 8) & 0xFF;
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
    fw_size = p_st_slave_info->flash_bin_size;
    FTD_LOG_TRACE("fw_size:0x%08x ", fw_size);

    slv_addr = SLAVE_FLASH_CHIP_0_MAP_0_FW_BIN_A_ADDR;

    FTD_LOG_DEBUG("fw_size:0x%08x last_page_valid_data_len:0x%08x", fw_size, (fw_size % SLAVE_FLASH_READ_SIZE));

    // package read cmd buffer
    // calc pram len
    uint16_t parm_len = EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_READ_LENGTH_SIZE; // 4 + 4 + 2
    uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET]     = (parm_len >> 8) & 0xFF;
    uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] = parm_len & 0xFF;
    // ext cmd
    uart_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET]      = (SLAVE_EXT_CMD_READ_FLASH >> 8) & 0xFF;//0x00;
    uart_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET + 1]  = SLAVE_EXT_CMD_READ_FLASH & 0xFF;//0x50;

    //FTD_LOG_DEBUG("read buffer size:%d single read len:%d rx_len:%d", sizeof(READ_BUFF), SLAVE_FLASH_READ_SIZE, 
    //    (SLAVE_FLASH_READ_CMD_SIZE_MAX + read_len));

    for (fw_read_length = 0; fw_read_length < fw_size; fw_read_length += SLAVE_FLASH_READ_SIZE)
    {
        FTD_LOG_TRACE(" read_slv_addr:0x%08x ", slv_addr);

        // slave flash addr
        uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET]     = (slv_addr >> 24) & 0xFF;
        uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 1] = (slv_addr >> 16) & 0xFF;
        uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 2] = (slv_addr >> 8) & 0xFF;
        uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 3] =  slv_addr & 0xFF;

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
        uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET]      = (read_len >> 24) & 0xFF;
        uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 1]  = (read_len >> 16) & 0xFF;
        uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 2]  = (read_len >> 8) & 0xFF;
        uart_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 3]  =  read_len & 0xFF;

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
        0x00, 0x68,
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

    // set erase addr
    send_buff[9]  = (SLAVE_FLASH_CHIP_0_MAP_0_AUTHOR_ADDR >> 24) & 0xFF;
    send_buff[10] = (SLAVE_FLASH_CHIP_0_MAP_0_AUTHOR_ADDR >> 16) & 0xFF;
    send_buff[11] = (SLAVE_FLASH_CHIP_0_MAP_0_AUTHOR_ADDR >> 8) & 0xFF;
    send_buff[12] =  SLAVE_FLASH_CHIP_0_MAP_0_AUTHOR_ADDR & 0xFF;

    // set slave flash erase size
    erase_size = SLAVE_FLASH_CHIP_0_MAP_0_CONFIG_B_ADDR - SLAVE_FLASH_CHIP_0_MAP_0_AUTHOR_ADDR + 0x1000;
    send_buff[13] = (erase_size >> 24) & 0xFF;
    send_buff[14] = (erase_size >> 16) & 0xFF;
    send_buff[15] = (erase_size >> 8) & 0xFF;
    send_buff[16] =  erase_size & 0xFF;
    FTD_LOG_DEBUG("erase_size = %d, 0x%x, 0x%x, 0x%x, 0x%x", erase_size, send_buff[13], send_buff[14], send_buff[15], send_buff[16]);

    if (0 == erase_size)
    {
        FTD_LOG_INFO("send slave erase cmd fail, erase_size = %x", erase_size);
        return ret;
    }

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
        && send_buff[9]  == SLAVE_CHN1_FUFF[9]
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

    // 1. write authorization info to slave
    uint8_t author_prog_flag = false;
    uint8_t author_info[ROLLCODE_LEN + EASYMESH_PID_LEN + TRIPLE_DATA_LEN + AUTHOR_INFO_CRC_LEN] = { 0 };
    data_size = ROLLCODE_LEN + EASYMESH_PID_LEN + TRIPLE_DATA_LEN + AUTHOR_INFO_CRC_LEN;

    slv_addr = SLAVE_FLASH_CHIP_0_MAP_0_AUTHOR_ADDR;

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
        author_info[4] = (rollcode >> 8)  & 0xFF;
        author_info[5] =  rollcode & 0xFF;

        FTD_LOG_DEBUG("rollcode:0x%0x ", rollcode);
        sys_info.st_fw_info[st_channel_burn_status->fw_num].st_roll_code_info.remain_counts--;
        author_prog_flag = true;
    }

    uint8_t invalid_pid[EASYMESH_PID_LEN] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    if (0 != memcmp(invalid_pid, sys_info.st_fw_info[st_channel_burn_status->fw_num].st_pid_info.easy_mesh_pid, EASYMESH_PID_LEN))
    {
        memcpy(&author_info[ROLLCODE_LEN], sys_info.st_fw_info[st_channel_burn_status->fw_num].st_pid_info.easy_mesh_pid, EASYMESH_PID_LEN);

        FTD_LOG_TRACE_BUFF(sys_info.st_fw_info[st_channel_burn_status->fw_num].st_pid_info.easy_mesh_pid, EASYMESH_PID_LEN, EASYMESH_PID_LEN);
        author_prog_flag = true;
    }

    if (0 != sys_info.st_fw_info[st_channel_burn_status->fw_num].st_triple_info.remain_counts)
    {
        read_partition = PARTITION_FW1_TRIPLE + st_channel_burn_status->fw_num * (PARTITION_FW2_TRIPLE - PARTITION_FW1_TRIPLE);
        read_length    = 0;
        uint16_t idx   = 0;

        idx = sys_info.st_fw_info[st_channel_burn_status->fw_num].st_triple_info.remain_counts - 1;
        read_length = idx * TRIPLE_DATA_LEN;

        if (ftd_drv_flash_operation((PARTITION_NAME)read_partition, OPERATION_READ, read_length, TRIPLE_DATA_LEN, data_buff) == 0)
        {
            memcpy(&author_info[ROLLCODE_LEN + EASYMESH_PID_LEN], data_buff, TRIPLE_DATA_LEN);

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
        author_info[data_size - 1] =  info_crc16 & 0xFF;
        FTD_LOG_TRACE_BUFF(&author_info[data_size - 2], data_size, AUTHOR_INFO_CRC_LEN);

        // save remain cnt to sys_info
        ftd_mw_sys_info_manager_set(&sys_info);

        // prepare uart send data
        send_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET]  = 0x00;
        send_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET + 1] = 0x51;
        // slave flash addr
        send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET]     = (slv_addr >> 24) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 1] = (slv_addr >> 16) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 2] = (slv_addr >> 8) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 3] =  slv_addr & 0xFF;
        // data length (fixed 0x00 for alain)
        send_buff[SLAVE_FLASH_PROG_LEN_OFFSET]      = 0x00;
        send_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 1]  = 0x00;
        send_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 2]  = 0x00;

        send_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET]     = (parm_len >> 8) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] =  parm_len & 0xFF;

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

    // 2. write client info to slave




    // 3. write config bin to slave
    data_size = sys_info.st_fw_info[st_channel_burn_status->fw_num].st_config_info.size;
    slv_addr  = SLAVE_FLASH_CHIP_0_MAP_0_CONFIG_A_ADDR;

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
            send_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET]  = 0x00;
            send_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET + 1] = 0x51;
            // slave flash addr
            send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET]     = (slv_addr >> 24) & 0xFF;
            send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 1] = (slv_addr >> 16) & 0xFF;
            send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 2] = (slv_addr >> 8) & 0xFF;
            send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 3] =  slv_addr & 0xFF;
            // data length (fixed 0x00 for alain)
            send_buff[SLAVE_FLASH_PROG_LEN_OFFSET]      = 0x00;
            send_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 1]  = 0x00;
            send_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 2]  = 0x00;

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
                && send_buff[9]  == SLAVE_CHN1_FUFF[9]
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

    // 1. read authorization info from slave flash
    //data_size = ROLLCODE_LEN + EASYMESH_PID_LEN + TRIPLE_DATA_LEN + AUTHOR_INFO_CRC_LEN;
    //slv_addr  = SLAVE_FLASH_CHIP_0_MAP_0_AUTHOR_ADDR;



    // 2. read client info from slave




    // 3. read write config bin from slave
    data_size = sys_info.st_fw_info[st_channel_burn_status->fw_num].st_config_info.size;
    slv_addr  = SLAVE_FLASH_CHIP_0_MAP_0_CONFIG_A_ADDR;

    FTD_LOG_DEBUG("data_size:0x%08x last_page_valid_data_len:0x%08x", data_size, (data_size % SLAVE_FLASH_READ_SIZE));

    // package read cmd buffer
    // calc pram len
    uint16_t parm_len = EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_READ_LENGTH_SIZE; // 4 + 4 + 2
    send_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET]     = (parm_len >> 8) & 0xFF;
    send_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] =  parm_len & 0xFF;
    // ext cmd
    send_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET]      = (SLAVE_EXT_CMD_READ_FLASH >> 8) & 0xFF;//0x00;
    send_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET + 1]  =  SLAVE_EXT_CMD_READ_FLASH & 0xFF;//0x50;

    for (read_ttl_len = 0; read_ttl_len < data_size; read_ttl_len += SLAVE_FLASH_READ_SIZE)
    {
        FTD_LOG_TRACE(" read_slv_addr:0x%08x ", slv_addr);

        // slave flash addr
        send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET]     = (slv_addr >> 24) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 1] = (slv_addr >> 16) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 2] = (slv_addr >> 8) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_ADDR_OFFSET + 3] =  slv_addr & 0xFF;

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
        send_buff[SLAVE_FLASH_PROG_LEN_OFFSET]     = (read_len >> 24) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 1] = (read_len >> 16) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 2] = (read_len >> 8) & 0xFF;
        send_buff[SLAVE_FLASH_PROG_LEN_OFFSET + 3] =  read_len & 0xFF;

        send_buff_len = SLAVE_FLASH_READ_DATA_OFFSET + CHECKSUM_SIZE;

        // add crc 
        send_buff[send_buff_len - CHECKSUM_SIZE] = ftd_mw_slave_manager_get_crc(send_buff, (send_buff_len - CHECKSUM_SIZE));

        FTD_LOG_DEBUG_BUFF(send_buff, sizeof(send_buff), send_buff_len);

        // send uart pkt to slave
        memset(READ_BUFF, 0x00, sizeof(READ_BUFF));
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

        FTD_LOG_TRACE_BUFF(READ_BUFF, sizeof(READ_BUFF), (SLAVE_FLASH_READ_CMD_SIZE_MAX + read_len));
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

    FTD_LOG_INFO("READ SLAVE FLASH DONE");

    return ret;
}

bool ftd_mw_slave_manager_sort(SLAVE_INFO* p_st_slave_info, CHANNEL_BURN_STATUS* st_channel_burn_status)
{
    // 1.power off slave
    ftd_mw_slave_manager_power_off(p_st_slave_info->slave_uart_channel);

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

    // Program specific fields
    uint8_t      read_partition;                        // Flash partition to read from
    uint8_t      fw_buff[SLAVE_FLASH_PROG_SIZE];        // Firmware data buffer (program only)

    // CRC check specific fields
    uint32_t     read_len;                              // Current read length (CRC only)
    uint16_t     calc_crc;                              // Calculated CRC (CRC only)
    uint16_t     expect_crc;                            // Expected CRC from sys_info (CRC only)
    bool         crc_pass;                              // CRC check result (CRC only)

    // Buffers (use max size to support both operations)
    uint8_t      uart_tx_buff[SLAVE_FLASH_PROG_CMD_SIZE_MAX]; // UART TX buffer
    uint8_t      uart_rx_buff[BURN_UART_DATA_MAX];            // UART RX buffer
} SM_CONTEXT;

// Keep old type names for compatibility
typedef SM_CONTEXT PROG_CONTEXT;
typedef SM_CONTEXT CRC_CONTEXT;

// Static context for each channel (shared between program and CRC)
static SM_CONTEXT g_sm_ctx[BURN_UART_CHANNELS] = { 0 };

// Compatibility macros
#define g_prog_ctx g_sm_ctx
#define g_crc_ctx  g_sm_ctx

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
    memset(p_ctx, 0, sizeof(PROG_CONTEXT));
    p_ctx->state = PROG_STATE_IDLE;
    p_ctx->operation = SM_OP_PROGRAM;
    p_ctx->channel = p_slave_info->slave_uart_channel;
    p_ctx->fw_size = p_slave_info->flash_bin_size;
    p_ctx->fw_read_offset = 0;
    p_ctx->slv_flash_addr = SLAVE_FLASH_CHIP_0_MAP_0_FW_BIN_A_ADDR;
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

    FTD_LOG_TRACE("CH%d: Context initialized, added 5ms startup delay", p_ctx->channel);
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
 * CRC Check State Machine Implementation
 ******************************************************************************/

 /**
  * @brief Initialize CRC check context for a channel
  */
static void ftd_mw_slave_manager_crc_ctx_init(CRC_CONTEXT* p_ctx, SLAVE_INFO* p_slave_info)
{
    SYS_INFO st_sys_info;
    ftd_mw_sys_info_manager_get(&st_sys_info);

    memset(p_ctx, 0, sizeof(CRC_CONTEXT));
    p_ctx->state = CRC_STATE_IDLE;
    p_ctx->operation = SM_OP_CRC_CHECK;
    p_ctx->channel = p_slave_info->slave_uart_channel;
    p_ctx->fw_size = p_slave_info->flash_bin_size;
    p_ctx->fw_read_offset = 0;
    p_ctx->slv_flash_addr = SLAVE_FLASH_CHIP_0_MAP_0_FW_BIN_A_ADDR;
    p_ctx->calc_crc = 0xFFFF;  // CRC init value
    p_ctx->expect_crc = st_sys_info.st_fw_info[0].st_bin_info.bin_crc16;
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

    FTD_LOG_TRACE("CH%d: CRC context initialized, fw_size=0x%08x, expect_crc=0x%04x",
        p_ctx->channel, p_ctx->fw_size, p_ctx->expect_crc);
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
 * @return true if completed successfully, false if still in progress or error
 */
bool ftd_mw_slave_manager_read_crc_sm(SLAVE_INFO* p_st_slave_info)
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
        ftd_mw_slave_manager_crc_ctx_init(p_ctx, p_st_slave_info);
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
