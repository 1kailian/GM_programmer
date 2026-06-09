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
/* Define functions                                                                                          */
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
                FTD_LOG_DEBUG("SLAVE TRIGGER IN BOOTLOADER");
                FTD_LOG_DEBUG_BUFF(rx_buffer, sizeof(SLAVE_CHN1_FUFF), 11);
                ret = SLAVE_CMD_ENTER_BOOT_LOADER;
            }
			#if 0
            // reset flash??
            if ((rx_buffer[4] == 0xD4) && (rx_buffer[5] == 0x00) && (rx_buffer[6] == 0x03) && (rx_buffer[7] == 0x00)
                && (rx_buffer[8] == 0x47) && (rx_buffer[9] == 0x01) && (rx_buffer[10] == 0x8C)
                )
            {
                FTD_LOG_DEBUG("SLAVE TRIGGER IN BOOTLOADER");
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
                    FTD_LOG_DEBUG("FLASH STATUS OK");
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
    uint8_t boot_loader_trigeer_cmd[6] = { 0x55, 0x96,0x55, 0x96,0x55, 0x96 }; //uart protocol
    uint8_t wait_time = 0;

    for (uint8_t i = 0; i < SLAVE_BOOT_TRIGGER_CNT; i++)
    {
        // 1.power off slave
        ftd_mw_slave_manager_power_off(p_st_slave_info->slave_uart_channel);
        FTD_LOG_TRACE("power off end");

        // delay abt 1.2s to waitting target power off
        //for (uint16_t i = 0; i < 50; i++)
        CLK_SysTickLongDelay(8000000);

        // 2.power on slave, send bootloader trigger cmd five times(boot_loader_trigeer_cmd)
        ftd_mw_slave_manager_power_on(p_st_slave_info->slave_uart_channel);
        FTD_LOG_TRACE("power on end");
        CLK_SysTickLongDelay(40000);

        FTD_LOG_TRACE("tx start");
        memset(SLAVE_CHN1_FUFF, 0x00, sizeof(SLAVE_CHN1_FUFF));
        ftd_drv_burn_uart_rx_start(p_st_slave_info->slave_uart_channel, SLAVE_CHN1_FUFF, 11);
        ftd_drv_burn_uart_tx_start(p_st_slave_info->slave_uart_channel, boot_loader_trigeer_cmd, sizeof(boot_loader_trigeer_cmd));
        //ftd_drv_burn_channel_start(boot_loader_trigeer_cmd, sizeof(boot_loader_trigeer_cmd), SLAVE_CHN1_FUFF, 11);
        while (!ftd_drv_burn_channel_is_tx_complete(p_st_slave_info->slave_uart_channel));
        FTD_LOG_TRACE("rx start");

        wait_time = 30;
        while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time-- > 0))
        {
            CLK_SysTickLongDelay(1000);
        }

        FTD_LOG_TRACE("rx end, wait_time:%d ", wait_time);

        uint8_t slave_ack = ftd_mw_slave_check_rx(SLAVE_CHN1_FUFF, sizeof(SLAVE_CHN1_FUFF));

        if (SLAVE_CMD_ENTER_BOOT_LOADER == slave_ack)
        {
            FTD_LOG_DEBUG("ch:%d check bootloader ack [0x%02x]", p_st_slave_info->slave_uart_channel, slave_ack);
            ret = true;
            break;
        }
        else
        {
            FTD_LOG_DEBUG("ch:%d slave check bootloader unack", p_st_slave_info->slave_uart_channel);
            ret = false;
        }
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
    wait_time = 30;
    while ((ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false) && (wait_time > 0))
    {
        CLK_SysTickLongDelay(200000);
        wait_time--;
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

bool ftd_mw_slave_manager_program_test(SLAVE_INFO* p_st_slave_info)
{
    bool     ret = false;
    uint16_t fw_read_length;
    uint32_t slv_addr;
    uint32_t fw_size;
    uint8_t  fw_buff[SLAVE_FLASH_PROG_SIZE];
    static uint8_t uart_buff[SLAVE_FLASH_PROG_CMD_SIZE_MAX] = { 0x4a, 0x59, 0x4d, 0x43, 0x54 };
    uint16_t uart_cmd_ttl_len = 0;
    uint16_t wait_time;

    fw_size = p_st_slave_info->flash_bin_size;

    if (0x00 == burn_test_addr)
        slv_addr = SLAVE_FLASH_BIN_ADDR;
    else
        slv_addr = SLAVE_FLASH_START_ADDR;

    FTD_LOG_TRACE("fw_size:0x%08x last_pkt_valid_data_len:0x%08x", fw_size, (fw_size % SLAVE_FLASH_PROG_SIZE));

    for (fw_read_length = 0; fw_read_length < fw_size; fw_read_length += SLAVE_FLASH_PROG_SIZE)
    {
        FTD_LOG_TRACE("read addr:0x%08x len:0x%08x slv_addr:0x%08x ", PARTITION_FW1_BIN, fw_read_length, slv_addr);
        if (ftd_drv_flash_operation(PARTITION_FW1_BIN, OPERATION_READ, fw_read_length, SLAVE_FLASH_PROG_SIZE, fw_buff) == 0)
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
                //FTD_LOG_TRACE_BUFF(fw_buff, sizeof(fw_buff), SLAVE_FLASH_PROG_SIZE);

                // calc pram len
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET]     = (SLAVE_FLASH_PROG_PRAM_LEN_MAX >> 8) & 0xFF;
                uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] = SLAVE_FLASH_PROG_PRAM_LEN_MAX & 0xFF;

                // copy bin
                memcpy(&uart_buff[SLAVE_FLASH_PROG_DATA_OFFSET], fw_buff, SLAVE_FLASH_PROG_SIZE);

                uart_cmd_ttl_len = SLAVE_FLASH_PROG_CMD_SIZE_MAX;
            }

            // add crc 
            uart_buff[uart_cmd_ttl_len - CHECKSUM_SIZE] = ftd_mw_slave_manager_get_crc(uart_buff, (uart_cmd_ttl_len - CHECKSUM_SIZE));

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

            wait_time = 1500;
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
    uint16_t fw_read_length;
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

    if (0x00 == burn_test_addr)
        slv_addr = SLAVE_FLASH_BIN_ADDR;
    else
        slv_addr = SLAVE_FLASH_START_ADDR;

    FTD_LOG_TRACE("fw_size:0x%08x last_page_valid_data_len:0x%08x", fw_size, (fw_size % SLAVE_FLASH_READ_SIZE));

    // package read cmd buffer
    // calc pram len
    uint16_t parm_len = EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_READ_LENGTH_SIZE; // 4 + 4 + 2
    uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET]     = (parm_len >> 8) & 0xFF;
    uart_buff[SLAVE_FLASH_PROG_PRAM_LEN_OFFSET + 1] = parm_len & 0xFF;
    // ext cmd
    uart_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET]      = (SLAVE_EXT_CMD_READ_FLASH >> 8) & 0xFF;//0x00;
    uart_buff[SLAVE_FLASH_PROG_EXT_CMD_OFFSET + 1]  = SLAVE_EXT_CMD_READ_FLASH & 0xFF;//0x50;

    for (fw_read_length = 0; fw_read_length < fw_size; fw_read_length += SLAVE_FLASH_READ_SIZE)
    {
        FTD_LOG_TRACE(" read_slv_addr:0x%08x ", slv_addr);

        // slave flash addr
        uart_buff[SLAVE_FLASH_PROG_ADDR_OFFSET]     = (slv_addr >> 24) & 0xFF;
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

        wait_time = 6000;
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

    FTD_LOG_INFO("READ SLAVE FLASH DONE");

    return ret;
}

///////////////////////////////////////////// debug end

bool ftd_mw_slave_manager_check_ld(SLAVE_INFO *p_st_slave_info)
{
    uint8_t slave_boot_loader_ack[11] = { 0x4A, 0x59, 0x4D, 0x43, 0xD4, 0x00, 0x03, 0x00, 0x47, 0x01, 0x8c }; //uart protocol

    if (ftd_drv_burn_channel_is_rx_complete(p_st_slave_info->slave_uart_channel) == false)
        return false;

    if (memcmp(p_st_slave_info->rx_buffer, slave_boot_loader_ack, sizeof(slave_boot_loader_ack)) == 0)
        return true;
    else
        return false;
}

void ftd_mw_slave_manager_set_default_slave_info(SLAVE_INFO* p_st_slave_info)
{
    p_st_slave_info->uart_baudrate = 115200;
    p_st_slave_info->en_slave_cmd_state = SLAVE_CMD_ENTER_BOOT_LOADER;
}

void ftd_mw_slave_manager_set_bin_info(uint32_t bin_addr, uint32_t bin_size, uint16_t bin_crc16)
{

}
