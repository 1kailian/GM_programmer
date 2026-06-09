/****************************************************************************
@FILENAME:  ftd_app_burn.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.20
@COPYRIGHT: FTD co.ltd
****************************************************************************/
#include "ftd_app_burn.h"
#include "ftd_app_state_machine.h"
#include "ftd_mw_sys_info_manager.h"
#include "ftd_mw_slave_manager.h"
#include "ftd_mw_display_manager.h"
#include "ftd_mw_log_manager.h"
#include "ftd_mw_burn_signal_manager.h"
#include "ftd_mw_powermon_manager.h"
#include "ftd_data_model.h"
#include "ftd_drv_burn_uart.h"
#include "NuMicro.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "stdint.h"

#define BURN_LOG_WARN   FTD_LOG_WARN
#define BURN_LOG_INFO   FTD_LOG_INFO
#define BURN_LOG_DBUG   FTD_LOG_DEBUG

#define BOOTLOADER_RETRY

static CHANNEL_BURN_STATUS st_channel_burn_status[BURN_CHANNEL];
static SLAVE_INFO st_slave_info_g[BURN_CHANNEL];
static uint8_t g_burn_mode = BURN_AUTO;
static uint8_t g_burn_wind_up = 0x00;
SYS_INFO sys_info;

extern uint8_t burn_test_addr;

void ftd_app_burn_init(void)
{
    uint8_t i;
    //SYS_INFO sys_info;

    BURN_LOG_INFO("ftd_app_burn_init\n");

    ftd_mw_sys_info_manager_get(&sys_info);
    ftd_mw_sys_info_manager_fw_info_dump(&sys_info.st_fw_info[st_channel_burn_status[i].fw_num]);

    g_burn_wind_up = 0x00;

    // Check if total remain count is sufficient for burn operation
    if (0 == sys_info.st_fw_info[st_channel_burn_status[0].fw_num].st_bin_info.remain_counts)
    {
        BURN_LOG_WARN("Total burn count is 0, cannot start burn operation!");
        // Set all channels to DONE state
        for (i = 0; i < BURN_CHANNEL; i++)
        {
            st_channel_burn_status[i].cur_burnning_status = BURN_DONE;
        }
        return;
    }
    else
    {
        BURN_LOG_INFO("Total remain count: %d", sys_info.st_fw_info[st_channel_burn_status[0].fw_num].st_bin_info.remain_counts);
    }

    // config all channel burn info
    for (i = 0; i < BURN_CHANNEL; i++)
    {
        st_slave_info_g[i].slave_uart_channel = i;
        ftd_mw_slave_manager_set_default_slave_info(&st_slave_info_g[i]);

        if (st_channel_burn_status[i].fw_num >= SUPPORT_FW_NUM)
        {
            BURN_LOG_WARN("burn fw num %d error, reset to 0", st_channel_burn_status[i].fw_num);
            st_channel_burn_status[i].fw_num = 0;
        }

        st_channel_burn_status[i].channel_num = i;
        st_channel_burn_status[i].cur_burnning_status = BURN_RUNNING;
        st_channel_burn_status[i].burn_total_count = 0x00;
        st_channel_burn_status[i].burn_success_count = 0x00;
        st_channel_burn_status[i].burn_error_code = 0x00;
        st_channel_burn_status[i].burn_process_rate = 0x00;
        st_channel_burn_status[i].burn_process_state = SLAVE_CMD_ENTER_BOOT_LOADER;

        st_slave_info_g[i].flash_bin_size = sys_info.st_fw_info[0].st_bin_info.size;
        //BURN_LOG_INFO("slave info [%d] bin_size = 0x%08x", i, st_slave_info_g[i].flash_bin_size)

        ftd_mw_display_manager_sync_channel_burn_status(&st_channel_burn_status[i]);
    }


}


void ftd_app_burn_process(void)
{
    bool b_update_display = false;

    //    uint8_t i = 0;

    for (uint8_t i = 0; i < BURN_CHANNEL; i++)
    {
        //BURN_LOG_INFO("___________CHN:%d en_slave_cmd_state:0x%02x", st_slave_info_g[i].slave_uart_channel, st_slave_info_g[i].en_slave_cmd_state);

        switch (st_slave_info_g[i].en_slave_cmd_state)
        {
            case SLAVE_CMD_ENTER_BOOT_LOADER:
            {
                if (BURN_AUTO == g_burn_mode)
                {
                    // check if start signal is received???
                    if (ftd_mw_burn_signal_manager_get_ch_state(st_slave_info_g[i].slave_uart_channel))
                    {
                        // Check if total remain count is sufficient
                        if (0 == sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_bin_info.remain_counts)
                        {
                            BURN_LOG_WARN("CHN:%d Total burn count is 0, cannot start burn!", i);
                            st_channel_burn_status[i].cur_burnning_status = BURN_DONE;
                            st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_DONE;
                            ftd_mw_burn_signal_manager_ch_set_signal((BURN_CH)st_slave_info_g[i].slave_uart_channel, STATE_NG);
                            b_update_display = false;
                            break;
                        }

                        ftd_mw_burn_signal_manager_ch_set_signal((BURN_CH)st_slave_info_g[i].slave_uart_channel, STATE_BUSY);
                        GPIO_DisableInt(PC, 11);
                        BURN_LOG_DBUG("RECV SALVE START SIGNAL CHN:%d ", i);
                    }
                    else
                    {
                        if (0x00 != g_burn_wind_up)
                        {
                            st_channel_burn_status[i].cur_burnning_status = BURN_DONE;
                            st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_DONE;
                        }

                        b_update_display = false;
                        break;
                    }
                }
                else
                {
                    // Manual mode: check total count before starting
                    if (0 == sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_bin_info.remain_counts)
                    {
                        BURN_LOG_WARN("CHN:%d Total burn count is 0, cannot start burn!", i);
                        st_channel_burn_status[i].cur_burnning_status = BURN_DONE;
                        st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_DONE;
                        b_update_display = false;
                        break;
                    }
                }
                if (true == ftd_mw_slave_manager_start_ld_test(&st_slave_info_g[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_CHECK_CURRENT_VOLTAGE;
                    st_channel_burn_status[i].burn_process_rate = 5;
                    st_channel_burn_status[i].burn_process_state = st_slave_info_g[i].en_slave_cmd_state;
                }
                else
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_ENTER_BOOT_LOADER;
                }

                break;
            }
            case SLAVE_CMD_CHECK_CURRENT_VOLTAGE:
            {
                int ret = ftd_mw_powermon_manager_get_burn_status(st_slave_info_g[i].slave_uart_channel);
                FTD_LOG_INFO("burn  status:%s", ret == 0 ? "true" : "false");
                if (ret == 0)
                {
                    // change to state sort send pass signal directly
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_MODIFY_BAUD_RATE;
                    st_channel_burn_status[i].burn_process_rate = 6;
                }
                else
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_CHECK_CURRENT_VOLTAGE;
                }
                break;
            }
            case SLAVE_CMD_MODIFY_BAUD_RATE:
            {
                if (true == ftd_mw_slave_manager_modify_baud_rate(&st_slave_info_g[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_SET_FLASH_STATUS_QUIT_WP;
                    st_channel_burn_status[i].burn_process_rate = 7;
                    ftd_drv_burn_uart_single_deinit(st_slave_info_g[i].slave_uart_channel);
                    ftd_drv_burn_uart_single_init(st_slave_info_g[i].slave_uart_channel, TRANSFER_BAUDRATE);
                }
                else
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_MODIFY_BAUD_RATE;
                }
            }
            case SLAVE_CMD_SET_FLASH_STATUS_QUIT_WP:
            {

                if (true == ftd_mw_slave_manager_disable_wp(&st_slave_info_g[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_ERASE_FLASH_SECTOR;
                    st_channel_burn_status[i].burn_process_rate = 8;

                }
                else
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_SET_FLASH_STATUS_QUIT_WP;
                }

                break;
            }
            case SLAVE_CMD_ERASE_FLASH_SECTOR:
            {
                if (true == ftd_mw_slave_manager_erase_test(&st_slave_info_g[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_WRITE_FLASH_DATA;
                    st_channel_burn_status[i].burn_process_rate = 10;
                }
                else
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_ERASE_FLASH_SECTOR;
                }
                b_update_display = true;
                break;
            }
            case SLAVE_CMD_WRITE_FLASH_DATA:
            {
                // Call state machine (non-blocking)
                // Return value: true = SUCCESS, false = in progress or error
                bool result = ftd_mw_slave_manager_program_sm(&st_slave_info_g[i], &st_channel_burn_status[i]);

                // Check if completed
                if (result)
                {
                    // result == true means SUCCESS
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_SET_FLASH_STATUS_REG;
                    st_channel_burn_status[i].burn_process_rate = 30;
                    BURN_LOG_INFO("CH%d: Programming completed successfully", i);
                    b_update_display = true;
                }
                else if (ftd_mw_slave_manager_program_sm_is_error(i))
                {
                    // Error occurred
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_WRITE_FLASH_DATA;
                    BURN_LOG_INFO("CH%d: Programming failed", i);
                    b_update_display = true;
                }
                // else: still in progress, keep current state

                break;
            }
            case SLAVE_CMD_SET_FLASH_STATUS_REG:
            {
                if (true == ftd_mw_slave_manager_enable_wp(&st_slave_info_g[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_READ_CODE_CRC;
                    st_channel_burn_status[i].burn_process_state = SLAVE_CMD_READ_CODE_CRC;
                    st_channel_burn_status[i].burn_process_rate = 32;
                }
                else
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_process_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_SET_FLASH_STATUS_REG;
                }

                break;
            }
            case SLAVE_CMD_READ_CODE_CRC:
            {
                // Call CRC state machine (non-blocking)
                // Return value: true = SUCCESS, false = in progress or error
                bool result = ftd_mw_slave_manager_read_crc_sm(&st_slave_info_g[i]);

                // Check if completed
                if (result)
                {
                    // CRC check passed
                    if (0x10 < sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_config_info.size
                        || 0 != sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_triple_info.remain_counts
                        || 0 != sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_roll_code_info.remain_counts)
                    {
                        st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_SET_FLASH_STATUS_QUIT_WP_PARM;
                        st_channel_burn_status[i].burn_process_state = st_slave_info_g[i].en_slave_cmd_state;
                        st_channel_burn_status[i].burn_process_rate = 50;
                    }
                    else
                    {
                        st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                        st_channel_burn_status[i].burn_process_state = st_slave_info_g[i].en_slave_cmd_state;
                        st_channel_burn_status[i].burn_process_rate = 100;
                    }
                    b_update_display = true;
                    BURN_LOG_INFO("CH%d: CRC check passed", i);
                }
                else if (ftd_mw_slave_manager_read_crc_sm_is_error(i))
                {
                    // CRC check failed
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_process_state = st_slave_info_g[i].en_slave_cmd_state;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_READ_CODE_CRC;

                    sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_bin_info.reserved++;

                    ftd_mw_sys_info_manager_set(&sys_info);
                    ftd_mw_sys_info_manager_save_nv();
                    BURN_LOG_INFO("CH%d: CRC check failed", i);
                    b_update_display = true;
                }
                // else: still in progress, keep current state

                break;
            }
            // write other data to slave
            case SLAVE_CMD_SET_FLASH_STATUS_QUIT_WP_PARM:
            {
                if (true == ftd_mw_slave_manager_disable_wp_parm(&st_slave_info_g[i], &st_channel_burn_status[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_ERASE_FLASH_SECTOR_PARM;
                    st_channel_burn_status[i].burn_process_rate = 52;
                }
                else
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_SET_FLASH_STATUS_QUIT_WP_PARM;
                }
                b_update_display = true;
                break;
            }
            case SLAVE_CMD_ERASE_FLASH_SECTOR_PARM:
            {
                if (true == ftd_mw_slave_manager_erase_parm(&st_slave_info_g[i], &st_channel_burn_status[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_WRITE_FLASH_DATA_PARM;
                    st_channel_burn_status[i].burn_process_rate = 55;
                }
                else
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_ERASE_FLASH_SECTOR_PARM;
                }
                break;
            }
            case SLAVE_CMD_WRITE_FLASH_DATA_PARM:
            {
                if (true == ftd_mw_slave_manager_program_parm(&st_slave_info_g[i], &st_channel_burn_status[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_SET_FLASH_STATUS_REG_PARM;
                    st_channel_burn_status[i].burn_process_rate = 70;
                }
                else
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_WRITE_FLASH_DATA_PARM;
                }

                break;
            }
            case SLAVE_CMD_SET_FLASH_STATUS_REG_PARM:
            {
                if (true == ftd_mw_slave_manager_enable_wp(&st_slave_info_g[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_READ_CODE_CRC_PARM;
                    st_channel_burn_status[i].burn_process_rate = 80;
                }
                else
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_SET_FLASH_STATUS_REG_PARM;
                }
                b_update_display = true;
                break;
            }
            case SLAVE_CMD_READ_CODE_CRC_PARM:
            {
                if (true == ftd_mw_slave_manager_read_crc_parm(&st_slave_info_g[i], &st_channel_burn_status[i]))
                {
                    st_channel_burn_status[i].burn_process_rate = 100;
                }
                else
                {
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_READ_CODE_CRC;
                }

                st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;

                break;
            }
            case SLAVE_CMD_STATE_SORT:
            {
                ftd_mw_slave_manager_sort(&st_slave_info_g[i], &st_channel_burn_status[i]);
                ftd_drv_burn_uart_single_deinit(st_slave_info_g[i].slave_uart_channel);
                ftd_drv_burn_uart_single_init(st_slave_info_g[i].slave_uart_channel, DEFAULT_BAUDRATE);
                if (BURN_MANNUL == g_burn_mode)
                {
                    // Manual mode: decrease count and set channel to DONE
                    st_channel_burn_status[i].cur_burnning_status = BURN_DONE;
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_DONE;
                    st_channel_burn_status[i].burn_process_state = st_slave_info_g[i].en_slave_cmd_state;
                    sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_bin_info.remain_counts--;
                    st_channel_burn_status[i].burn_total_count++;

                    // Record burn result
                    if (SLAVE_CMD_NONE == st_channel_burn_status[i].burn_error_code)
                    {
                        st_channel_burn_status[i].burn_success_count++;
                    }
                    else
                    {
                    }
                }
                else
                {
                    // Auto mode: check total remain count
                    if (0 == sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_bin_info.remain_counts)
                    {
                        // Total count exhausted, stop burning
                        st_channel_burn_status[i].cur_burnning_status = BURN_DONE;
                        st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_DONE;
                        BURN_LOG_WARN("Total burn count exhausted!");
                    }
                    else
                    {
                        // Decrease total count
                        sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_bin_info.remain_counts--;
                        st_channel_burn_status[i].burn_total_count++;

                        if (SLAVE_CMD_NONE == st_channel_burn_status[i].burn_error_code)
                        {
                            // generate PASS signal to automatic fixture
                            ftd_mw_burn_signal_manager_ch_set_signal((BURN_CH)st_slave_info_g[i].slave_uart_channel, STATE_PASS);
                            st_channel_burn_status[i].burn_success_count++;
                            BURN_LOG_DBUG("SEND SALVE PASS SIGNAL CHN:%d ", i);
                        }
                        else
                        {
                            // generate NG signal to automatic fixture
                            ftd_mw_burn_signal_manager_ch_set_signal((BURN_CH)st_slave_info_g[i].slave_uart_channel, STATE_NG);
                            BURN_LOG_DBUG("SEND SALVE NG SIGNAL CHN:%d ", i);
                            // add debug log to print error code
                        }
                        GPIO_EnableInt(PC, 11, GPIO_INT_FALLING); // enable GPIO interrupt

                        // wind up burn engine
                        if (0x00 != g_burn_wind_up)
                        {
                            st_channel_burn_status[i].cur_burnning_status = BURN_DONE;
                            st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_DONE;
                        }
                        // prepare next burn task
                        else
                        {
                            st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_ENTER_BOOT_LOADER;
                            st_channel_burn_status[i].burn_process_state = st_slave_info_g[i].en_slave_cmd_state;
                            st_channel_burn_status[i].burn_error_code = SLAVE_CMD_NONE;
                            st_channel_burn_status[i].burn_process_rate = 0;
                        }

                    }
                }
                b_update_display = true;
                break;
            }
            case SLAVE_CMD_DONE:
            {
                //BURN_LOG_DBUG("SLAVE_CMD_DONE CHN:%d", i);
                break;
            }
            default:
                b_update_display = false;
                break;
        }

        // sync st_channel_burn_status to panel
        if (b_update_display)
            ftd_mw_display_manager_sync_channel_burn_status(&st_channel_burn_status[i]);

    }
}

void ftd_app_burn_deinit(void)
{
    BURN_LOG_INFO("ftd_app_burn_deinit \n");

    // Print burn statistics for each channel
    for (uint8_t i = 0; i < BURN_CHANNEL; i++)
    {
        BURN_LOG_INFO("CHN:%d Statistics - Total:%d, Success:%d, Failed:%d",
            i,
            st_channel_burn_status[i].burn_total_count,
            st_channel_burn_status[i].burn_success_count,
            st_channel_burn_status[i].burn_total_count - st_channel_burn_status[i].burn_success_count);
    }

    // save fw info in nv 
    // save sys_info to flash
    ftd_mw_sys_info_manager_set(&sys_info);
    ftd_mw_sys_info_manager_save_nv();
}

void ftd_app_burn_fw_select(uint8_t fw_num)
{
    uint8_t i;

    if (fw_num >= SUPPORT_FW_NUM)
    {
        return;
    }

    for (i = 0; i < BURN_CHANNEL; i++)
    {
        st_channel_burn_status[i].fw_num = fw_num;
    }
}

void ftd_app_burn_set_burn_mode(uint8_t burn_mode)
{
    g_burn_mode = burn_mode;
    BURN_LOG_INFO("set burn_mode to %s", g_burn_mode == BURN_AUTO ? "AUTO" : "MANUAL");
}

void ftd_app_burn_set_wind_up(uint8_t value)
{
    g_burn_wind_up = value;
}

BRUN_STATUS_E ftd_app_burn_get_general_burn_state(void)
{
    BRUN_STATUS_E state = BURN_STOP;
    uint8_t cnt = 0;

    for (uint8_t i = 0; i < BURN_CHANNEL; i++)
    {
        cnt += st_channel_burn_status[i].cur_burnning_status;
    }

    if (cnt == 0)
    {
        state = BURN_STOP;
    }
    else if (cnt == (BURN_DONE * BURN_CHANNEL))
    {
        state = BURN_DONE;
    }
    else
    {
        state = BURN_RUNNING;
    }

    return state;
}

/////////////////////////////// debug code //////////////////////////////////
void ftd_app_burn_debug_hc_set_burn_addr(uint8_t is_test)
{
    burn_test_addr = is_test;
}

void ftd_app_burn_debug_hc_set_burn_state(uint8_t state_id)
{
    switch (state_id)
    {
        // 0x00 to init burn info
        case SLAVE_CMD_NONE:
        {
            ftd_app_burn_init();
            break;
        }
        // burn process single step trigger by state_id
        case SLAVE_CMD_ENTER_BOOT_LOADER:
        case SLAVE_CMD_READ_CODE_CRC:
        case SLAVE_CMD_SET_FLASH_STATUS_REG:
        case SLAVE_CMD_SET_FLASH_STATUS_QUIT_WP:
        case SLAVE_CMD_ERASE_FLASH_SECTOR:
        case SLAVE_CMD_WRITE_FLASH_DATA:
        case SLAVE_CMD_WRITE_RAM_DATA:
        case SLAVE_CMD_RESET_FLASH:
        case SLAVE_CMD_SET_FLASH_STATUS_QUIT_WP_PARM:
        case SLAVE_CMD_ERASE_FLASH_SECTOR_PARM:
        case SLAVE_CMD_WRITE_FLASH_DATA_PARM:
        case SLAVE_CMD_SET_FLASH_STATUS_REG_PARM:
        case SLAVE_CMD_READ_CODE_CRC_PARM:
        case SLAVE_CMD_STATE_SORT:
        {
            for (uint8_t i = 0; i < BURN_CHANNEL; i++)
            {
                st_slave_info_g[i].en_slave_cmd_state = (SLAVE_CMD)state_id;
            }

            ftd_app_burn_process();

            break;
        }
        // switch to mannul mode 
        case 0xFD:
        {

            break;
        }
        // switch system state to burn process
        case 0xFE:
        {
            BURN_LOG_INFO(" swtich to BURN ENGINE ");
            app_ftd_state_machine_change_state(APP_BURN_ENGINE);
            break;
        }
        // burn process single step trigger by state machine
        case 0xFF:
        {
            ftd_app_burn_process();

            break;
        }

        default:
            break;
    }

}

/////////////////////////////// debug code end///////////////////////////////

