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
#include "ftd_data_model.h"
#include "NuMicro.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "stdint.h"

#define BURN_LOG_WARN   FTD_LOG_WARN
#define BURN_LOG_INFO   FTD_LOG_INFO
#define BURN_LOG_DBUG   FTD_LOG_DEBUG

#define BURN_CHANNEL (1) // for debug only, 4 channel normally

static CHANNEL_BURN_STATUS st_channel_burn_status[BURN_CHANNEL];
static SLAVE_INFO st_slave_info_g[BURN_CHANNEL];
static uint8_t g_select_burn_auto_mode = 0;

extern uint8_t burn_test_addr;

void ftd_app_burn_init(void)
{
    uint8_t i;
    SYS_INFO sys_info;

    BURN_LOG_INFO("ftd_app_burn_init\n");

    ftd_mw_sys_info_manager_get(&sys_info);

    // config all channel burn info
    for (i = 0; i < BURN_CHANNEL; i++)
    {
        st_slave_info_g[i].slave_uart_channel = i;
        ftd_mw_slave_manager_set_default_slave_info(&st_slave_info_g[i]);

        //st_channel_burn_status[i].burn_error_code   = 0x00;

        st_slave_info_g[i].flash_bin_size = sys_info.st_fw_info[0].st_bin_info.size;
        BURN_LOG_INFO("slave info [%d] bin_size = 0x%08x", i, st_slave_info_g[i].flash_bin_size);

        if (0 != sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_bin_info.remain_counts)
        {
            if (0 != sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_triple_info.remain_counts
                || 0 != sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_triple_info.remain_counts)
                st_channel_burn_status[i].burn_remain_count = sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_bin_info.remain_counts;
        }
        else
        {
            BURN_LOG_INFO("burn cnt over");
        }
    }
    g_select_burn_auto_mode = 1;
}


void ftd_app_burn_process(void)
{
    bool b_update_display = false;

    uint8_t i = 0;
    for (uint8_t i = 0; i < BURN_CHANNEL; i++)
    {
        // BURN_LOG_INFO("___________CHN:%d en_slave_cmd_state:0x%02x", st_slave_info_g[i].slave_uart_channel, st_slave_info_g[i].en_slave_cmd_state);

        switch (st_slave_info_g[i].en_slave_cmd_state)
        {
            case SLAVE_CMD_ENTER_BOOT_LOADER:
            {
                if (1 == g_select_burn_auto_mode)
                {
                    // check if start signal is received???
                    if (ftd_mw_burn_signal_manager_get_ch_state(st_slave_info_g[i].slave_uart_channel))
                    {
                        ftd_mw_burn_signal_manager_ch_set_signal((BURN_CH)st_slave_info_g[i].slave_uart_channel, STATE_BUSY);
                        GPIO_DisableInt(PC, 11);
                        // st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_ENTER_BOOT_LOADER;
                        st_channel_burn_status[i].burn_error_code = 0x00;
                        BURN_LOG_INFO("RECV SALVE START SIGNAL CHN:%d ", i);
                    }
                    else
                    {
                        break;
                    }
                }
            quit_wp_err_back:
                if (true == ftd_mw_slave_manager_start_ld_test(&st_slave_info_g[i]))
                {
                    // change to state sort send pass signal directly
                    //st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_SET_FLASH_STATUS_QUIT_WP;
                    b_update_display = true;

                }
                else
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_ENTER_BOOT_LOADER;
                    b_update_display = true;
                }

                break;
            }
            case SLAVE_CMD_SET_FLASH_STATUS_QUIT_WP:
            {
                static int cnt = 0;
                if (true == ftd_mw_slave_manager_disable_wp(&st_slave_info_g[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_ERASE_FLASH_SECTOR;
                    b_update_display = true;
                }
                else
                {
                    if (0 == cnt)
                    {
                        cnt++;
                        goto quit_wp_err_back:
                    }
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_SET_FLASH_STATUS_QUIT_WP;
                    cnt = 0;
                    b_update_display = true;
                }

                break;
            }
            case SLAVE_CMD_ERASE_FLASH_SECTOR:
            {
                if (true == ftd_mw_slave_manager_erase_test(&st_slave_info_g[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_WRITE_FLASH_DATA;
                    b_update_display = true;
                }
                else
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_ERASE_FLASH_SECTOR;
                    b_update_display = true;
                }

                break;
            }
            case SLAVE_CMD_WRITE_FLASH_DATA:
            {
                if (true == ftd_mw_slave_manager_program_test(&st_slave_info_g[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_SET_FLASH_STATUS_REG;
                    b_update_display = true;
                }
                else
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_WRITE_FLASH_DATA;
                    b_update_display = true;
                }

                break;
            }
            case SLAVE_CMD_SET_FLASH_STATUS_REG:
            {
                if (true == ftd_mw_slave_manager_enable_wp(&st_slave_info_g[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_READ_CODE_CRC;
                    b_update_display = true;
                }
                else
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_SET_FLASH_STATUS_REG;
                    b_update_display = true;
                }

                break;
            }
            case SLAVE_CMD_READ_CODE_CRC:
            {
                if (true == ftd_mw_slave_manager_read_crc_test(&st_slave_info_g[i]))
                {
                    b_update_display = true;
                }
                else
                {
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_READ_CODE_CRC;
                    b_update_display = true;
                }

                st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;

                break;
            }
            case SLAVE_CMD_STATE_SORT:
            {
                st_channel_burn_status[i].burn_remain_count -= 1;

                if (0 == g_select_burn_auto_mode)
                {
                    // channel will cycle in SLAVE_CMD_DONE
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_DONE;
                }
                else
                {
                    if (0 == st_channel_burn_status[i].burn_remain_count)
                    {
                        st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_DONE;
                    }
                    else
                    {
                        //if (ftd_mw_misc_manager_get_time_ms() % 2 == 0)
                        if (0x00 == st_channel_burn_status[i].burn_error_code)
                        {
                            // generate PASS signal to automatic fixture
                            ftd_mw_burn_signal_manager_ch_set_signal((BURN_CH)st_slave_info_g[i].slave_uart_channel, STATE_PASS);
                        }
                        else
                        {
                            // generate NG signal to automatic fixture
                            ftd_mw_burn_signal_manager_ch_set_signal((BURN_CH)st_slave_info_g[i].slave_uart_channel, STATE_NG);
                            // add debug log to print error code
                        }
                        GPIO_EnableInt(PC, 11, GPIO_INT_FALLING); // enable GPIO interrupt
                        st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_ENTER_BOOT_LOADER;
                    }
                }
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
    }

    // sync st_channel_burn_status to panel
    if (b_update_display)
        ftd_mw_display_manager_sync_channel_burn_status(&st_channel_burn_status[i]);
}

void ftd_app_burn_deinit(void)
{
    BURN_LOG_INFO("ftd_app_burn_deinit \n");
}

void ftd_app_burn_fw_select(uint8_t fw_num)
{
    uint8_t i;
    for (i = 0; i < BURN_CHANNEL; i++)
    {
        st_channel_burn_status[i].fw_num = fw_num;
    }
}

void ftd_app_burn_set_auto(uint8_t auto_burn)
{
    g_select_burn_auto_mode = auto_burn;
}

void ftd_app_burn_manual_trigger(void)
{
    uint8_t i;

    g_select_burn_auto_mode = 0;

    //if(g_select_burn_auto_mode == 1)
    //{
    BURN_LOG_INFO("ftd_app_burn_manual_trigger\n");
    for (i = 0; i < BURN_CHANNEL; i++)
    {
        st_channel_burn_status[i].burn_remain_count = 1;
    }
    //}

}

uint8_t ftd_app_burn_get_burn_done_state(void)
{
    uint8_t done_cnt, state;

    done_cnt = 0;

    for (uint8_t i = 0; i < BURN_CHANNEL; i++)
    {
        if (SLAVE_CMD_DONE == st_slave_info_g[i].en_slave_cmd_state)
        {
            done_cnt++;
        }
    }

    state = (done_cnt == BURN_CHANNEL) ? true : false;

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
            ftd_app_burn_manual_trigger();
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

