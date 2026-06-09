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
#include "ftd_mw_ring_buffer.h"
#include "NuMicro.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "stdint.h"

#define BURN_LOG_WARN   FTD_LOG_WARN
#define BURN_LOG_INFO   FTD_LOG_INFO
#define BURN_LOG_DBUG   FTD_LOG_DEBUG
#define BURN_LOG_ERROR  FTD_LOG_ERROR

//Control the FT test switch 1:enable 0:disable
#define FT_TEST         1
//Control the FT test only when the automated testing is enabled. 1:enable 0:disable
#define FT_TEST_AUTO    1 

static CHANNEL_BURN_STATUS st_channel_burn_status[BURN_CHANNEL] = { 0 };
static uint8_t g_burn_mode = BURN_AUTO;
static SLAVE_INFO st_slave_info_g[BURN_CHANNEL];
static uint8_t g_burn_wind_up = 0x00;

#if DEBUG_USE_UART4
static uint8_t g_ft_test_queue[BURN_CHANNEL] = { 0xFF, 0xFF, 0xFF};
#else
static uint8_t g_ft_test_queue[BURN_CHANNEL] = { 0xFF, 0xFF, 0xFF, 0xFF };
#endif
static uint8_t g_queue_head = 0;
static uint8_t g_queue_tail = 0;
static uint8_t g_queue_count = 0; // 队列元素计数器
static uint8_t g_last_processed_channel = 0;

// Queue is empty when g_queue_count == 0
// Queue is full when g_queue_count == BURN_CHANNEL


static bool ftd_app_burn_is_channel_in_queue(uint8_t channel)
{
    // Search all elements in the queue using count
    for (uint8_t i = 0; i < g_queue_count; i++)
    {
        uint8_t pos = (g_queue_head + i) % BURN_CHANNEL;
        if (g_ft_test_queue[pos] == channel)
            return true;
    }
    return false;
}


static bool ftd_app_burn_add_channel_to_queue(uint8_t channel)
{
    // Check if channel is already in queue
    if (ftd_app_burn_is_channel_in_queue(channel))
        return true;

    // Check if queue is full
    if (g_queue_count == BURN_CHANNEL)
        return false; // Queue is full

    // Add channel to queue
    g_ft_test_queue[g_queue_tail] = channel;
    g_queue_tail = (g_queue_tail + 1) % BURN_CHANNEL;
    g_queue_count++; // 增加计数器
    return true;
}

static void ftd_app_burn_remove_channel_from_queue(uint8_t channel)
{
    // Find the position of the channel in the queue
    uint8_t pos = 0xFF;
    for (uint8_t i = 0; i < g_queue_count; i++)
    {
        uint8_t current_pos = (g_queue_head + i) % BURN_CHANNEL;
        if (g_ft_test_queue[current_pos] == channel)
        {
            pos = current_pos;
            break;
        }
    }

    if (pos == 0xFF)
        return; // Channel not found in queue

    // If removing the head, just move head forward
    if (pos == g_queue_head)
    {
        g_ft_test_queue[g_queue_head] = 0xFF; // Clear the slot
        g_queue_head = (g_queue_head + 1) % BURN_CHANNEL;
        g_queue_count--; // 减少计数器

        // If queue becomes empty, reset both pointers
        if (g_queue_count == 0)
        {
            g_queue_head = 0;
            g_queue_tail = 0;
        }
        return;
    }

    // Shift elements between pos and tail to maintain FIFO order
    uint8_t current = pos;
    while (current != g_queue_tail)
    {
        uint8_t next = (current + 1) % BURN_CHANNEL;
        if (next != g_queue_tail)
        {
            g_ft_test_queue[current] = g_ft_test_queue[next];
        }
        else
        {
            g_ft_test_queue[current] = 0xFF; // Clear the last slot
        }
        current = next;
    }

    // Adjust tail pointer
    g_queue_tail = (g_queue_tail + BURN_CHANNEL - 1) % BURN_CHANNEL;
    g_queue_count--; // 减少计数器

    // If queue becomes empty, reset both pointers
    if (g_queue_count == 0)
    {
        g_queue_head = 0;
        g_queue_tail = 0;
    }
}

SYS_INFO sys_info;

// Channel enable mapping table
static struct {
    bool enabled;              // Channel enable flag
    uint8_t channel_index;     // Channel index (0-3)
} g_channel_map[BURN_CHANNEL] = {
    {true, 0},  // Channel 0: enabled by default
    {true, 1},  // Channel 1: enabled by default
    {true, 2},  // Channel 2: enabled by default
        #if !DEBUG_USE_UART4
    {true, 3}   // Channel 3: enabled by default
        #endif
};

extern uint8_t burn_test_addr;

void ftd_app_burn_init(void)
{
    uint8_t i = 0;
    //SYS_INFO sys_info;

    BURN_LOG_INFO("ftd_app_burn_init\n");

    ftd_mw_sys_info_manager_get(&sys_info);
    ftd_mw_sys_info_manager_fw_info_dump(&sys_info.st_fw_info[st_channel_burn_status[0].fw_num]);

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
        if (g_channel_map[i].enabled)
        {
            // Reset bootloader state machine for clean start
            ftd_mw_slave_manager_start_ld_sm_reset(i);

            // Ensure UART is at default baudrate for bootloader trigger
            //ftd_drv_burn_uart_single_deinit(i);
            ftd_drv_burn_uart_single_init(i, DEFAULT_BAUDRATE);

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
#if (FT_TEST == 1)
#if (FT_TEST_AUTO == 1)
            // Only use FT test in AUTO mode
            if (BURN_AUTO == g_burn_mode)
            {
                st_channel_burn_status[i].burn_process_state = SLAVE_CMD_FT_ENTER_BOOT_LOADER;
                st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_FT_ENTER_BOOT_LOADER;
            }
            else
            {
                st_channel_burn_status[i].burn_process_state = SLAVE_CMD_ENTER_BOOT_LOADER;
                st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_ENTER_BOOT_LOADER;
            }
#else
            // Use FT test in all modes
            st_channel_burn_status[i].burn_process_state = SLAVE_CMD_FT_ENTER_BOOT_LOADER;
            st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_FT_ENTER_BOOT_LOADER;
#endif
#else
            st_channel_burn_status[i].burn_process_state = SLAVE_CMD_ENTER_BOOT_LOADER;
            st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_ENTER_BOOT_LOADER;
#endif
            st_slave_info_g[i].flash_bin_size = sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_bin_info.size;
            st_slave_info_g[i].chip_id = (uint16_t)sys_info.st_fw_info[st_channel_burn_status[i].fw_num].chip_id;;
            //BURN_LOG_INFO("slave info [%d] bin_size = 0x%08x", i, st_slave_info_g[i].flash_bin_size)
        }
        else
        {
            // Initialize disabled channel to BURN_STOP state
            st_channel_burn_status[i].channel_num = i;
            st_channel_burn_status[i].cur_burnning_status = BURN_STOP;
            st_channel_burn_status[i].burn_total_count = 0x00;
            st_channel_burn_status[i].burn_success_count = 0x00;
            st_channel_burn_status[i].burn_error_code = 0x00;
            st_channel_burn_status[i].burn_process_rate = 0x00;
            st_channel_burn_status[i].burn_process_state = SLAVE_CMD_DONE;

            // Set slave info to done state
            st_slave_info_g[i].slave_uart_channel = i;
            st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_DONE;
            BURN_LOG_INFO("Channel %d initialized to BURN_STOP state", i);
        }

        ftd_mw_display_manager_sync_channel_burn_status(&st_channel_burn_status[i]);
    }


}


void ftd_app_burn_process(void)
{
    bool b_update_display = false;


    for (uint8_t j = 0; j < BURN_CHANNEL; j++)
    {
        uint8_t i = (g_last_processed_channel + j) % BURN_CHANNEL;

        // Check if channel is enabled
        if (!g_channel_map[i].enabled)
        {
            // Skip disabled channel
            continue;
        }

        //BURN_LOG_INFO("___________CHN:%d en_slave_cmd_state:0x%02x", st_slave_info_g[i].slave_uart_channel, st_slave_info_g[i].en_slave_cmd_state);

        switch (st_slave_info_g[i].en_slave_cmd_state)
        {
#if (1 == FT_TEST)
            case SLAVE_CMD_FT_ENTER_BOOT_LOADER:
            {
                if (BURN_AUTO == g_burn_mode)
                {
                    // Check if boot state machine is already running
                    BOOT_SM_STATE boot_state = ftd_mw_slave_manager_start_ld_sm_get_state(st_slave_info_g[i].slave_uart_channel);

                    if (boot_state == BOOT_SM_STATE_IDLE)
                    {
                        // Boot SM not started yet, check if start signal is received
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
                            BURN_LOG_DBUG("RECV SALVE START SIGNAL CHN:%d ", i);
                        }
                        else
                        {
                            // No signal received yet, check wind up flag
                            if (0x00 != g_burn_wind_up)
                            {
                                st_channel_burn_status[i].cur_burnning_status = BURN_DONE;
                                st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_DONE;
                            }

                            b_update_display = false;
                            break;
                        }
                    }
                    // else: Boot SM already running, continue to execute state machine below
                }
                else if (BURN_BATCH == g_burn_mode)
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

                    if (0x00 != g_burn_wind_up)
                    {
                        st_channel_burn_status[i].cur_burnning_status = BURN_DONE;
                        st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_DONE;
                        b_update_display = false;
                        break;
                    }
                }
                else if (BURN_MANNUL == g_burn_mode)
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
                // Call bootloader state machine (non-blocking)
                // Return value: true = SUCCESS, false = in progress or error
                bool boot_result = ftd_mw_slave_manager_start_ld_sm(&st_slave_info_g[i], &st_channel_burn_status[i]);

                // Check if completed
                if (boot_result)
                {
                    // Boot trigger succeeded - slave entered bootloader
#ifdef FT_TEST_ROM
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_FT_MODIFY_BAUD_RATE;
#else
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_FT_SET_FLASH_STATUS_QUIT_WP;
#endif
                    st_channel_burn_status[i].burn_process_rate = 0;
                    st_channel_burn_status[i].burn_process_state = st_slave_info_g[i].en_slave_cmd_state;
                    BURN_LOG_INFO("CH%d: Bootloader trigger succeeded", i);

                    // Reset boot SM for next use
                    ftd_mw_slave_manager_start_ld_sm_reset(i);
                    b_update_display = true;
                }
                else if (ftd_mw_slave_manager_start_ld_sm_is_error(i))
                {
                    // Error occurred
                    BURN_LOG_INFO("CH%d: Bootloader trigger failed", i);

                    // Reset boot SM for next use
                    ftd_mw_slave_manager_start_ld_sm_reset(i);

                    if (BURN_BATCH == g_burn_mode)
                    {
                        st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_FT_ENTER_BOOT_LOADER;
                    }
                    else
                    {
                        st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                        st_channel_burn_status[i].burn_error_code = SLAVE_CMD_FT_ENTER_BOOT_LOADER;
                    }
                    b_update_display = true;
                }
                // else: still in progress, keep current state

                break;
            }
            case SLAVE_CMD_FT_MODIFY_BAUD_RATE:
            {
                if (true == ftd_mw_slave_manager_modify_baud_rate(&st_slave_info_g[i], &st_channel_burn_status[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_WRITE_RAM_DATA;
                    st_channel_burn_status[i].burn_process_rate = 7;
                    //ftd_drv_burn_uart_single_deinit(st_slave_info_g[i].slave_uart_channel);
                    ftd_drv_burn_uart_single_init(st_slave_info_g[i].slave_uart_channel, TRANSFER_BAUDRATE);
                }
                else
                {
                    FTD_LOG_ERROR("CH%d SLAVE_CMD_MODIFY_BAUD_RATE:false", i);
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_MODIFY_BAUD_RATE;
                }
            }
            case SLAVE_CMD_WRITE_RAM_DATA:
            {
                if (true == ftd_mw_slave_ft_test_manager_program_test_sm(&st_slave_info_g[i], &st_channel_burn_status[i]))
                {
                    BURN_LOG_INFO("CH%d: SLAVE_CMD_WRITE_RAM_DATA OK", i);
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_SET_PROGRAM_JUMP;
                    st_channel_burn_status[i].burn_process_rate = 0;
                }
                else
                {
                    // Check if it's an error or still in progress
                    if (ftd_mw_slave_ft_test_manager_program_test_sm_is_error(st_slave_info_g[i].slave_uart_channel))
                    {
                        BURN_LOG_ERROR("CH%d: SLAVE_CMD_WRITE_RAM_DATA ERROR", i);
                        st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                        st_channel_burn_status[i].burn_error_code = SLAVE_CMD_WRITE_RAM_DATA;
                        b_update_display = true;
                    }
                    else
                    {
                        // Still in progress, keep current state
                        b_update_display = false;
                    }
                }
                break;
            }
            case SLAVE_CMD_SET_PROGRAM_JUMP:
            {
                // Use blocking version
                bool jump_result = ftd_mw_slave_manager_program_jump(&st_slave_info_g[i], FT_TEST_SLAVE_BIN_JUMP_ADDR);

                if (jump_result)
                {
                    BURN_LOG_INFO("CH%d: SLAVE_CMD_SET_PROGRAM_JUMP OK", i);
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_FT_WRITE_CHIP_ID;
                    st_channel_burn_status[i].burn_process_rate = 0;

                }
                else
                {
                    BURN_LOG_ERROR("CH%d: SLAVE_CMD_SET_PROGRAM_JUMP FAILED", i);
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_SET_PROGRAM_JUMP;
                }
                b_update_display = true;
                break;
            }
            case SLAVE_CMD_FT_SET_FLASH_STATUS_QUIT_WP:
            {

                if (true == ftd_mw_slave_manager_disable_wp(&st_slave_info_g[i]))
                {
                    BURN_LOG_INFO("CH%d: SLAVE_CMD_FT_SET_FLASH_STATUS_QUIT_WP OK", i);
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_FT_ERASE_FLASH_SECTOR;
                    st_channel_burn_status[i].burn_process_rate = 0;

                }
                else
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_FT_SET_FLASH_STATUS_QUIT_WP;
                }

                break;
            }
            case SLAVE_CMD_FT_ERASE_FLASH_SECTOR:
            {
                if (true == ftd_mw_slave_manager_erase_test(&st_slave_info_g[i]))
                {
                    BURN_LOG_INFO("CH%d: SLAVE_CMD_FT_ERASE_FLASH_SECTOR OK", i);
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_FT_WRITE_FLASH_DATA;
                    st_channel_burn_status[i].burn_process_rate = 0;
                }
                else
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_FT_ERASE_FLASH_SECTOR;
                }
                b_update_display = true;
                break;
            }
            case SLAVE_CMD_FT_WRITE_FLASH_DATA:
            {
                if (true == ftd_mw_slave_ft_test_manager_program_test_sm(&st_slave_info_g[i], &st_channel_burn_status[i]))
                {
                    BURN_LOG_INFO("CH%d: SLAVE_CMD_FT_WRITE_FLASH_DATA OK", i);
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_FT_SET_FLASH_STATUS_REG;
                    st_channel_burn_status[i].burn_process_rate = 0;
                }
                else
                {
                    // Check if it's an error or still in progress
                    if (ftd_mw_slave_ft_test_manager_program_test_sm_is_error(st_slave_info_g[i].slave_uart_channel))
                    {
                        BURN_LOG_ERROR("CH%d: SLAVE_CMD_FT_WRITE_FLASH_DATA ERROR", i);
                        st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                        st_channel_burn_status[i].burn_error_code = SLAVE_CMD_FT_WRITE_FLASH_DATA;
                        b_update_display = true;
                    }
                    else
                    {
                        // Still in progress, keep current state
                        b_update_display = false;
                    }
                }
                break;
            }
            case SLAVE_CMD_FT_SET_FLASH_STATUS_REG:
            {
                if (true == ftd_mw_slave_manager_enable_wp(&st_slave_info_g[i]))
                {
                    BURN_LOG_INFO("CH%d: SLAVE_CMD_FT_SET_FLASH_STATUS_REG OK", i);
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_FT_RESTATE;
                    st_channel_burn_status[i].burn_process_rate = 0;
                }
                else
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_process_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_FT_SET_FLASH_STATUS_REG;
                }

                break;
            }
            case SLAVE_CMD_FT_RESTATE:
            {
                if (true == ftd_mw_slave_manager_restart_test(&st_slave_info_g[i]))
                {
                    BURN_LOG_INFO("CH%d: SLAVE_CMD_FT_RESTATE OK", i);
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_SEND_PACKET_ADDR;
                    //ftd_drv_burn_uart_single_deinit(st_slave_info_g[i].slave_uart_channel);
                    ftd_drv_burn_uart_single_init(st_slave_info_g[i].slave_uart_channel, TRANSFER_BAUDRATE);
                }
                else
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_FT_RESTATE;
                }
                break;
            }
            case SLAVE_CMD_FT_WRITE_CHIP_ID:
            {
                if (true == ftd_mw_slave_manager_write_chip_id(&st_slave_info_g[i]))
                {
                    BURN_LOG_INFO("CH%d: SLAVE_CMD_FT_WRITE_CHIP_ID OK", i);
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_SEND_PACKET_ADDR;
                    st_channel_burn_status[i].burn_process_rate = 0;
                }
                else
                {
                    BURN_LOG_ERROR("CH%d: SLAVE_CMD_FT_WRITE_CHIP_ID ERROR", i);
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_FT_WRITE_CHIP_ID;
                }
                break;
            }
            case SLAVE_CMD_SEND_PACKET_ADDR:
            {


                if (true == ftd_mw_slave_manager_test_send_packet_address(&st_slave_info_g[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_TEST_CURRENT_AT_0DB;
                    st_channel_burn_status[i].burn_process_rate = 0;
                    FTD_LOG_DEBUG("slave:%d, send packet address success", st_slave_info_g[i].slave_uart_channel);
                }
                else
                {
                    // FT test failed, remove channel from queue
                    BURN_LOG_ERROR("CHN:%d FT test failed at send packet address, removing from queue", i);
                    ftd_app_burn_remove_channel_from_queue(i);

                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_SEND_PACKET_ADDR;
                    FTD_LOG_ERROR("slave:%d, send packet address failed", st_slave_info_g[i].slave_uart_channel);
                }

                break;
            }
            case SLAVE_CMD_TEST_CURRENT_AT_0DB:
            {
                // Add channel to FT test queue if not already in queue
                ftd_app_burn_add_channel_to_queue(i);

                // Check if this channel is at the head of the queue
                if (g_ft_test_queue[g_queue_head] != i)
                {
                    // Not this channel's turn, wait
                    b_update_display = false;
                    break;
                }
                if (true == ftd_mw_slave_manager_test_current_at_0db(&st_slave_info_g[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_TEST_CURRENT_AT_5DB;
                    st_channel_burn_status[i].burn_process_rate = 1;
                    FTD_LOG_DEBUG("slave:%d, test current at 0db success", st_slave_info_g[i].slave_uart_channel);
                }
                else
                {
                    // FT test failed, remove channel from queue
                    BURN_LOG_ERROR("CHN:%d FT test failed at 0db current, removing from queue", i);
                    ftd_app_burn_remove_channel_from_queue(i);

                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_TEST_CURRENT_AT_0DB;
                    FTD_LOG_ERROR("slave:%d, test current at 0db failed", st_slave_info_g[i].slave_uart_channel);
                }
                break;
            }
            case SLAVE_CMD_TEST_CURRENT_AT_5DB:
            {
                if (true == ftd_mw_slave_manager_test_current_at_5db(&st_slave_info_g[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_TEST_RECEIVE_SENSITIVITY;
                    st_channel_burn_status[i].burn_process_rate = 2;
                    FTD_LOG_DEBUG("slave:%d, test current at 5db success", st_slave_info_g[i].slave_uart_channel);
                }
                else
                {
                    // FT test failed, remove channel from queue
                    BURN_LOG_ERROR("CHN:%d FT test failed at 5db current, removing from queue", i);
                    ftd_app_burn_remove_channel_from_queue(i);

                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_TEST_CURRENT_AT_5DB;
                    FTD_LOG_ERROR("slave:%d, test current at 5db failed", st_slave_info_g[i].slave_uart_channel);
                }
                break;
            }

            case SLAVE_CMD_TEST_RECEIVE_SENSITIVITY:
            {

                if (true == ftd_mw_slave_manager_test_receiving_sensitivity(&st_slave_info_g[i]))
                {
                    // FT test completed, remove channel from queue and advance head
                    BURN_LOG_INFO("CHN:%d FT test completed, removing from queue", i);
                    ftd_app_burn_remove_channel_from_queue(i);

                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_TEST_SLEEP_CURRENT;
                    FTD_LOG_DEBUG("slave:%d, test receive sensitivity success", st_slave_info_g[i].slave_uart_channel);
                }
                else
                {
                    // FT test failed, remove channel from queue
                    BURN_LOG_ERROR("CHN:%d FT test failed at receive sensitivity, removing from queue", i);
                    ftd_app_burn_remove_channel_from_queue(i);

                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_TEST_RECEIVE_SENSITIVITY;
                    FTD_LOG_ERROR("slave:%d, test receive sensitivity failed", st_slave_info_g[i].slave_uart_channel);
                }
                break;
            }
            case SLAVE_CMD_TEST_CURRENT_AT_STATIC:
            {
                if (true == ftd_mw_slave_manager_test_current_at_static_sm(&st_slave_info_g[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_CALIBRATE_MADC;
                    st_channel_burn_status[i].burn_process_rate = 3;
                    FTD_LOG_DEBUG("slave:%d, test current at static success", st_slave_info_g[i].slave_uart_channel);
                }
                else if (ftd_mw_slave_manager_test_sm_is_error(st_slave_info_g[i].slave_uart_channel))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_TEST_CURRENT_AT_STATIC;
                    FTD_LOG_ERROR("slave:%d, test current at static failed", st_slave_info_g[i].slave_uart_channel);
                }
                else
                {
                    // Still in progress, do nothing
                    return;
                }
                break;
            }
            case SLAVE_CMD_CALIBRATE_MADC:
            {
                if (true == ftd_mw_slave_manager_calibrate_madc_sm(&st_slave_info_g[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_CALIBRATE_GPADC;
                    FTD_LOG_DEBUG("slave:%d, calibrate madc success", st_slave_info_g[i].slave_uart_channel);
                }
                else if (ftd_mw_slave_manager_test_sm_is_error(st_slave_info_g[i].slave_uart_channel))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_CALIBRATE_MADC;
                    FTD_LOG_ERROR("slave:%d, calibrate madc failed", st_slave_info_g[i].slave_uart_channel);
                }
                else
                {
                    // Still in progress, do nothing
                    return;
                }
                break;
            }
            case SLAVE_CMD_CALIBRATE_GPADC:
            {
                if (true == ftd_mw_slave_manager_calibrate_gpadc_sm(&st_slave_info_g[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_CALIBRATE_CLOCK;
                    st_channel_burn_status[i].burn_process_rate = 4;
                    FTD_LOG_DEBUG("slave:%d, calibrate gpadc success", st_slave_info_g[i].slave_uart_channel);
                }
                else if (ftd_mw_slave_manager_test_sm_is_error(st_slave_info_g[i].slave_uart_channel))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_CALIBRATE_GPADC;
                    FTD_LOG_ERROR("slave:%d, calibrate gpadc failed", st_slave_info_g[i].slave_uart_channel);
                }
                else
                {
                    // Still in progress, do nothing
                    return;
                }
                break;
            }
            case SLAVE_CMD_CALIBRATE_CLOCK:
            {
                if (true == ftd_mw_slave_manager_calibrate_clock_sm(&st_slave_info_g[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_TEST_SLEEP_CURRENT;
                    st_channel_burn_status[i].burn_process_rate = 4;
                    FTD_LOG_DEBUG("slave:%d, calibrate clock success", st_slave_info_g[i].slave_uart_channel);
                }
                else if (ftd_mw_slave_manager_test_sm_is_error(st_slave_info_g[i].slave_uart_channel))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_CALIBRATE_CLOCK;
                    FTD_LOG_ERROR("slave:%d, calibrate clock failed", st_slave_info_g[i].slave_uart_channel);
                }
                else
                {
                    // Still in progress, do nothing
                    return;
                }
                break;
            }
            case SLAVE_CMD_TEST_SLEEP_CURRENT:
            {
                if (true == ftd_mw_slave_manager_test_sleep_current_sm(&st_slave_info_g[i]))
                {
                    // Continue to boot loader for firmware burn (parallel execution allowed)
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_ENTER_BOOT_LOADER;
                    st_channel_burn_status[i].burn_process_rate = 4;
                    FTD_LOG_DEBUG("slave:%d, test sleep current success", st_slave_info_g[i].slave_uart_channel);
                }
                else if (ftd_mw_slave_manager_test_sm_is_error(st_slave_info_g[i].slave_uart_channel))
                {
                    // FT test failed, still remove channel from queue
                    BURN_LOG_ERROR("CHN:%d FT test failed, removing from queue", i);
                    ftd_app_burn_remove_channel_from_queue(i);

                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_TEST_SLEEP_CURRENT;
                    FTD_LOG_ERROR("slave:%d, test sleep current failed", st_slave_info_g[i].slave_uart_channel);
                }
                else
                {
                    // Still in progress, do nothing
                    return;
                }
                break;
            }
            // case SLAVE_CMD_JUMP_BOOTLOADER:
            // {
            //     if (true == ftd_mw_slave_manager_test_sleep_current(&st_slave_info_g[i]))
            //     {
            //         // Continue to boot loader for firmware burn (parallel execution allowed)
            //         st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_CHECK_CURRENT_VOLTAGE;
            //         st_channel_burn_status[i].burn_process_rate = 4;
            //         FTD_LOG_DEBUG("slave:%d, test sleep current success", st_slave_info_g[i].slave_uart_channel);
            //     }
            //     else
            //     {
            //         st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
            //         st_channel_burn_status[i].burn_error_code = SLAVE_CMD_FT_ENTER_BOOT_LOADER;
            //         FTD_LOG_ERROR("slave:%d, test sleep current failed", st_slave_info_g[i].slave_uart_channel);
            //     }
            //     //ftd_drv_burn_uart_single_deinit(st_slave_info_g[i].slave_uart_channel);
            //     ftd_drv_burn_uart_single_init(st_slave_info_g[i].slave_uart_channel, DEFAULT_BAUDRATE);
            //     break;
            // }
            #endif
            case SLAVE_CMD_ENTER_BOOT_LOADER:
            {
                if (BURN_AUTO == g_burn_mode)
                {
                    // Check if boot state machine is already running
                    BOOT_SM_STATE boot_state = ftd_mw_slave_manager_start_ld_sm_get_state(st_slave_info_g[i].slave_uart_channel);

                    if (boot_state == BOOT_SM_STATE_IDLE)
                    {
                        // FT test: no need to check start signal, directly start bootloader
#if (FT_TEST == 1)
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
                        BURN_LOG_DBUG("FT test: directly start bootloader CHN:%d ", i);
#else
    // Normal burn: check if start signal is received
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
                            BURN_LOG_DBUG("RECV SALVE START SIGNAL CHN:%d ", i);
                        }
                        else
                        {
                            // No signal received yet, check wind up flag
                            if (0x00 != g_burn_wind_up)
                            {
                                st_channel_burn_status[i].cur_burnning_status = BURN_DONE;
                                st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_DONE;
                            }

                            b_update_display = false;
                            break;
                        }
#endif
                    }
                    // else: Boot SM already running, continue to execute state machine below
                }
                else if (BURN_BATCH == g_burn_mode)
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

                    if (0x00 != g_burn_wind_up)
                    {
                        st_channel_burn_status[i].cur_burnning_status = BURN_DONE;
                        st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_DONE;
                        b_update_display = false;
                        break;
                    }
                }
                else if (BURN_MANNUL == g_burn_mode)
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
                // Call bootloader state machine (non-blocking)
                // Return value: true = SUCCESS, false = in progress or error
                bool boot_result = ftd_mw_slave_manager_start_ld_sm(&st_slave_info_g[i], &st_channel_burn_status[i]);

                // Check if completed
                if (boot_result)
                {
                    // Boot trigger succeeded - slave entered bootloader
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_CHECK_CURRENT_VOLTAGE;
                    st_channel_burn_status[i].burn_process_rate = 5;
                    st_channel_burn_status[i].burn_process_state = st_slave_info_g[i].en_slave_cmd_state;
                    BURN_LOG_INFO("CH%d: Bootloader trigger succeeded", i);

                    // Reset boot SM for next use
                    ftd_mw_slave_manager_start_ld_sm_reset(i);
                    b_update_display = true;
                }
                else if (ftd_mw_slave_manager_start_ld_sm_is_error(i))
                {
                    // Error occurred
                    FTD_LOG_ERROR("CH%d: Bootloader trigger failed", i);

                    // Reset boot SM for next use
                    ftd_mw_slave_manager_start_ld_sm_reset(i);

                    if (BURN_BATCH == g_burn_mode)
                    {
                        st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_ENTER_BOOT_LOADER;
                    }
                    else
                    {
                        st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                        st_channel_burn_status[i].burn_error_code = SLAVE_CMD_ENTER_BOOT_LOADER;
                    }
                    b_update_display = true;
                }
                // else: still in progress, keep current state

                break;
            }
            case SLAVE_CMD_CHECK_CURRENT_VOLTAGE:
            {
                int ret = ftd_mw_powermon_manager_get_burn_status(st_slave_info_g[i].slave_uart_channel);
                if (ret == 0)
                {
                    FTD_LOG_INFO("burn  status:%s", ret == 0 ? "true" : "false");
                    // change to state sort send pass signal directly
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_MODIFY_BAUD_RATE;
                    st_channel_burn_status[i].burn_process_rate = 6;
                }
                else
                {
                    FTD_LOG_ERROR("burn  status:%s", ret == 0 ? "true" : "false");
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_CHECK_CURRENT_VOLTAGE;
                }
                break;
            }
            case SLAVE_CMD_MODIFY_BAUD_RATE:
            {
                if (true == ftd_mw_slave_manager_modify_baud_rate(&st_slave_info_g[i], &st_channel_burn_status[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_SET_FLASH_STATUS_QUIT_WP;
                    st_channel_burn_status[i].burn_process_rate = 7;
                    //ftd_drv_burn_uart_single_deinit(st_slave_info_g[i].slave_uart_channel);
                    ftd_drv_burn_uart_single_init(st_slave_info_g[i].slave_uart_channel, TRANSFER_BAUDRATE);
                }
                else
                {
                    FTD_LOG_ERROR("CH%d SLAVE_CMD_MODIFY_BAUD_RATE:false", i);
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
                    FTD_LOG_ERROR("CH%d SLAVE_CMD_SET_FLASH_STATUS_QUIT_WP:false", i);
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
                    FTD_LOG_ERROR("CH%d SLAVE_CMD_ERASE_FLASH_SECTOR:false", i);
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
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_READ_CODE_CRC;
                    st_channel_burn_status[i].burn_process_rate = 30;
                    BURN_LOG_INFO("CH%d: Programming completed successfully", i);
                    b_update_display = true;
                }
                else if (ftd_mw_slave_manager_program_sm_is_error(i))
                {
                    // Error occurred
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_WRITE_FLASH_DATA;
                    FTD_LOG_ERROR("CH%d: Programming failed", i);
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
                    FTD_LOG_ERROR("CH%d SLAVE_CMD_SET_FLASH_STATUS_REG:false", i);
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
                bool result = ftd_mw_slave_manager_read_crc_sm(&st_slave_info_g[i], &st_channel_burn_status[i]);

                // Check if completed
                if (result)
                {
                    // CRC check passed
                    if (0x10 < sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_config_info.size
                        || 0 != sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_triple_info.remain_counts
                        || 0 != sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_roll_code_info.remain_counts)
                    {
                        st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_ERASE_FLASH_SECTOR_PARM;
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
                    BURN_LOG_INFO("CH%d: CRC check succeed", i);
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
                    FTD_LOG_ERROR("CH%d: CRC check failed", i);
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
                    FTD_LOG_ERROR("CH%d SLAVE_CMD_SET_FLASH_STATUS_QUIT_WP_PARM:false", i);
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
                    FTD_LOG_ERROR("CH%d SLAVE_CMD_ERASE_FLASH_SECTOR_PARM:false", i);
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_ERASE_FLASH_SECTOR_PARM;
                }
                break;
            }
            case SLAVE_CMD_WRITE_FLASH_DATA_PARM:
            {
                // Check if we need to burn triple data
                if (0 != sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_triple_info.remain_counts)
                {
                    BURN_LOG_INFO("CH%d: Start burning triple data", i);
                }

                if (true == ftd_mw_slave_manager_program_parm(&st_slave_info_g[i], &st_channel_burn_status[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_READ_CODE_CRC_PARM;
                    st_channel_burn_status[i].burn_process_rate = 70;
                }
                else
                {
                    FTD_LOG_ERROR("CH%d SLAVE_CMD_WRITE_FLASH_DATA_PARM:false", i);
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_WRITE_FLASH_DATA_PARM;
                }

                break;
            }

            case SLAVE_CMD_READ_CODE_CRC_PARM:
            {
                if (true == ftd_mw_slave_manager_read_crc_parm(&st_slave_info_g[i], &st_channel_burn_status[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_SET_FLASH_STATUS_REG_PARM;
                    st_channel_burn_status[i].burn_process_rate = 100;
                }
                else
                {
                    FTD_LOG_ERROR("CH%d SLAVE_CMD_READ_CODE_CRC_PARM:false", i);
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_READ_CODE_CRC;
                }



                break;
            }
            case SLAVE_CMD_SET_FLASH_STATUS_REG_PARM:
            {
                if (true == ftd_mw_slave_manager_enable_wp(&st_slave_info_g[i]))
                {
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_process_rate = 100;
                }
                else
                {
                    FTD_LOG_ERROR("CH%d SLAVE_CMD_SET_FLASH_STATUS_REG_PARM:false", i);
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_STATE_SORT;
                    st_channel_burn_status[i].burn_error_code = SLAVE_CMD_SET_FLASH_STATUS_REG_PARM;
                }
                b_update_display = true;
                break;
            }
            case SLAVE_CMD_STATE_SORT:
            {
#if (FT_TEST == 1)
                // Remove channel from FT test queue if present
                ftd_app_burn_remove_channel_from_queue(i);
#endif

                ftd_mw_slave_manager_sort(&st_slave_info_g[i]);
                //ftd_drv_burn_uart_single_deinit(st_slave_info_g[i].slave_uart_channel);
                ftd_drv_burn_uart_single_init(st_slave_info_g[i].slave_uart_channel, DEFAULT_BAUDRATE);

                if (BURN_MANNUL == g_burn_mode)
                {
                    // Manual mode: burn once per channel, then stop
                    st_channel_burn_status[i].cur_burnning_status = BURN_DONE;
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_DONE;
                    st_channel_burn_status[i].burn_process_state = st_slave_info_g[i].en_slave_cmd_state;
                    st_channel_burn_status[i].burn_total_count++;

                    // Record burn result
                    if (SLAVE_CMD_NONE == st_channel_burn_status[i].burn_error_code)
                    { 
                        st_channel_burn_status[i].burn_success_count++;
                        sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_bin_info.remain_counts--;
                        if (sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_triple_info.remain_counts > 0)
                        {
                            sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_triple_info.remain_counts--;
                        }
                        if (sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_roll_code_info.remain_counts > 0)
                        {
                            sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_roll_code_info.remain_counts--;
                        }
                        ftd_mw_sys_info_manager_set(&sys_info);
                        
                    }
                }
                else if (BURN_BATCH == g_burn_mode)
                {
                    // Manual mode: decrease count and set channel to DONE
                    // st_channel_burn_status[i].cur_burnning_status = BURN_DONE;
                    st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_DONE;
                    st_channel_burn_status[i].burn_process_state = st_slave_info_g[i].en_slave_cmd_state;
                    st_channel_burn_status[i].burn_total_count++;

                    // Record burn result
                    if (SLAVE_CMD_NONE == st_channel_burn_status[i].burn_error_code)
                    {
                        st_channel_burn_status[i].burn_success_count++;
                        sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_bin_info.remain_counts--;
                        if (sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_triple_info.remain_counts > 0)
                        {
                            sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_triple_info.remain_counts--;
                        }
                        if (sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_roll_code_info.remain_counts > 0)
                        {
                            sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_roll_code_info.remain_counts--;
                        }
                        ftd_mw_sys_info_manager_set(&sys_info);
                        ftd_mw_ring_buffer_update_remain_counts(&sys_info);
                    }
                    else
                    {

                    }
                }
                else
                {
                    // check total remain count
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

                        st_channel_burn_status[i].burn_total_count++;

                        if (SLAVE_CMD_NONE == st_channel_burn_status[i].burn_error_code)
                        {
                            // generate PASS signal to automatic fixture
                            ftd_mw_burn_signal_manager_ch_set_signal((BURN_CH)st_slave_info_g[i].slave_uart_channel, STATE_PASS);
                            st_channel_burn_status[i].burn_success_count++;
                            sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_bin_info.remain_counts--;
                            if (sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_triple_info.remain_counts > 0)
                            {
                                sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_triple_info.remain_counts--;
                            }
                            if (sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_roll_code_info.remain_counts > 0)
                            {
                                sys_info.st_fw_info[st_channel_burn_status[i].fw_num].st_roll_code_info.remain_counts--;
                            }
                            ftd_mw_sys_info_manager_set(&sys_info);
                            ftd_mw_ring_buffer_update_remain_counts(&sys_info);
                            FTD_LOG_INFO("SEND SALVE succeeded SIGNAL CHN:%d ", i);
                        }
                        else
                        {
                            // generate NG signal to automatic fixture
                            ftd_mw_burn_signal_manager_ch_set_signal((BURN_CH)st_slave_info_g[i].slave_uart_channel, STATE_NG);
                            FTD_LOG_INFO("SEND SALVE failed SIGNAL CHN:%d ", i);
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
#if (FT_TEST == 1)
#if (FT_TEST_AUTO == 1)
                            // Only use FT test in AUTO mode
                            if (BURN_AUTO == g_burn_mode)
                            {
                                st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_FT_ENTER_BOOT_LOADER;
                            }
                            else
                            {
                                st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_ENTER_BOOT_LOADER;
                            }
#else
                            // Use FT test in all modes
                            st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_FT_ENTER_BOOT_LOADER;
#endif
#else
                            st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_ENTER_BOOT_LOADER;
#endif
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
                if (BURN_BATCH == g_burn_mode)
                {
                    // Check if wind up flag is set (user requested to stop batch burn)
                    if (0x00 != g_burn_wind_up)
                    {
                        if (st_channel_burn_status[i].cur_burnning_status != BURN_DONE)
                        {
                            BURN_LOG_INFO("CH%d: Wind up flag set, batch burn stopped", i);
                            st_channel_burn_status[i].cur_burnning_status = BURN_DONE;
                            b_update_display = true;
                        }
                    }
                    else
                    {
                        // Continue next batch burn
#if (FT_TEST == 1)
#if (FT_TEST_AUTO == 1)
                        // Only use FT test in AUTO mode
                        if (BURN_AUTO == g_burn_mode)
                        {
                            st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_FT_ENTER_BOOT_LOADER;
                        }
                        else
                        {
                            st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_ENTER_BOOT_LOADER;
                        }
#else
                        // Use FT test in all modes
                        st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_FT_ENTER_BOOT_LOADER;
#endif
#else
                        st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_ENTER_BOOT_LOADER;
#endif
                        st_channel_burn_status[i].burn_process_state = st_slave_info_g[i].en_slave_cmd_state;
                        st_channel_burn_status[i].burn_error_code = SLAVE_CMD_NONE;
                        st_channel_burn_status[i].burn_process_rate = 0;
                        // Reset burn status to RUNNING for next batch
                        st_channel_burn_status[i].cur_burnning_status = BURN_RUNNING;
                        b_update_display = true;
                    }
                }
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
        // Reset state machine and stop UART DMA
        ftd_mw_slave_manager_start_ld_sm_reset(i);

        // Power off and cleanup
        ftd_mw_slave_manager_sort(&st_slave_info_g[i]);

        // Reset UART to default baudrate for next bootloader trigger
        //ftd_drv_burn_uart_single_deinit(st_slave_info_g[i].slave_uart_channel);
        ftd_drv_burn_uart_single_init(st_slave_info_g[i].slave_uart_channel, DEFAULT_BAUDRATE);

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
    ftd_mw_ring_buffer_force_update_remain_counts(&sys_info);
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

/**
 * @brief Set channel enable state
 * @param channel Channel index (0-3)
 * @param enable true to enable channel, false to disable
 */
void ftd_app_burn_set_channel_enable(uint8_t channel, bool enable)
{
    if (channel < BURN_CHANNEL)
    {
        bool was_enabled = g_channel_map[channel].enabled;
        g_channel_map[channel].enabled = enable;

        if (enable != was_enabled)
        {
            if (enable)
            {
                // Channel enabled: reset to BURN_RUNNING state
                st_channel_burn_status[channel].cur_burnning_status = BURN_RUNNING;
#if (FT_TEST == 1)
                st_channel_burn_status[channel].burn_process_state = SLAVE_CMD_TEST_CURRENT_AT_0DB;
                st_slave_info_g[channel].en_slave_cmd_state = SLAVE_CMD_TEST_CURRENT_AT_0DB;
#else
                st_channel_burn_status[channel].burn_process_state = SLAVE_CMD_ENTER_BOOT_LOADER;
                st_slave_info_g[channel].en_slave_cmd_state = SLAVE_CMD_ENTER_BOOT_LOADER;
#endif

                // Re-initialize UART for bootloader trigger
                //ftd_drv_burn_uart_single_deinit(channel);
                ftd_drv_burn_uart_single_init(channel, DEFAULT_BAUDRATE);

                BURN_LOG_INFO("Channel %d enabled, state reset to BURN_RUNNING", channel);
            }
            else
            {
                // Channel disabled: set to BURN_STOP state
                st_channel_burn_status[channel].cur_burnning_status = BURN_STOP;
                st_channel_burn_status[channel].burn_process_state = SLAVE_CMD_DONE;
                st_slave_info_g[channel].en_slave_cmd_state = SLAVE_CMD_DONE;

                BURN_LOG_INFO("Channel %d disabled, state set to BURN_STOP", channel);
            }

            // Sync status to display
            ftd_mw_display_manager_sync_channel_burn_status(&st_channel_burn_status[channel]);
        }
        else
        {
            BURN_LOG_INFO("Channel %d already %s", channel, enable ? "enabled" : "disabled");
        }
    }
    else
    {
        BURN_LOG_ERROR("Invalid channel: %d", channel);
    }
}

/**
 * @brief Get channel enable state
 * @param channel Channel index (0-3)
 * @return true if channel is enabled, false otherwise
 */
bool ftd_app_burn_get_channel_enable(uint8_t channel)
{
    if (channel < BURN_CHANNEL)
    {
        return g_channel_map[channel].enabled;
    }
    else
    {
        BURN_LOG_ERROR("Invalid channel: %d", channel);
        return false;
    }
}

/**
 * @brief Set channels enable state by mask
 * @param mask Bitmask where each bit represents a channel's enable state
 *             Bit 0: Channel 0, Bit 1: Channel 1, Bit 2: Channel 2, Bit 3: Channel 3
 */
void ftd_app_burn_set_channels_enable_by_mask(uint8_t mask)
{
    for (uint8_t i = 0; i < BURN_CHANNEL; i++)
    {
        bool enable = (mask & (1 << i)) != 0;
        bool was_enabled = g_channel_map[i].enabled;

        if (enable != was_enabled)
        {
            g_channel_map[i].enabled = enable;

            if (enable)
            {
                // Channel enabled: reset to BURN_RUNNING state
                st_channel_burn_status[i].cur_burnning_status = BURN_RUNNING;
#if (FT_TEST == 1)
                st_channel_burn_status[i].burn_process_state = SLAVE_CMD_TEST_CURRENT_AT_0DB;
                st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_TEST_CURRENT_AT_0DB;
#else
                st_channel_burn_status[i].burn_process_state = SLAVE_CMD_ENTER_BOOT_LOADER;
                st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_ENTER_BOOT_LOADER;
#endif

                // Re-initialize UART for bootloader trigger
                //ftd_drv_burn_uart_single_deinit(i);
                ftd_drv_burn_uart_single_init(i, DEFAULT_BAUDRATE);

                BURN_LOG_INFO("Channel %d enabled, state reset to BURN_RUNNING", i);
            }
            else
            {
                // Channel disabled: set to BURN_STOP state
                st_channel_burn_status[i].cur_burnning_status = BURN_STOP;
                st_channel_burn_status[i].burn_process_state = SLAVE_CMD_DONE;
                st_slave_info_g[i].en_slave_cmd_state = SLAVE_CMD_DONE;

                BURN_LOG_INFO("Channel %d disabled, state set to BURN_STOP", i);
            }

            // Sync status to display
            ftd_mw_display_manager_sync_channel_burn_status(&st_channel_burn_status[i]);
        }
        else
        {
            BURN_LOG_INFO("Channel %d already %s", i, enable ? "enabled" : "disabled");
        }
    }
    BURN_LOG_INFO("Channels enabled by mask: 0x%02X", mask);
}

BRUN_STATUS_E ftd_app_burn_get_general_burn_state(void)
{
    BRUN_STATUS_E state = BURN_STOP;
    uint8_t cnt = 0;
    uint8_t enabled_channel_count = 0;
    uint8_t done_channel_count = 0;

    for (uint8_t i = 0; i < BURN_CHANNEL; i++)
    {
        // Only consider enabled channels
        if (g_channel_map[i].enabled)
        {
            enabled_channel_count++;
            if (st_channel_burn_status[i].cur_burnning_status == BURN_DONE)
            {
                done_channel_count++;
            }
            else if (st_channel_burn_status[i].cur_burnning_status == BURN_RUNNING)
            {
                cnt++;
            }
        }
    }

    if (enabled_channel_count == 0)
    {
        // No enabled channels, return stop
        state = BURN_STOP;
    }
    else if (done_channel_count == enabled_channel_count)
    {
        // All enabled channels are done
        state = BURN_DONE;
    }
    else if (cnt > 0)
    {
        // Some enabled channels are running
        state = BURN_RUNNING;
    }
    else
    {
        // All enabled channels are stopped
        state = BURN_STOP;
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

