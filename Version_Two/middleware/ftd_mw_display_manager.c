/****************************************************************************
@FILENAME:  ftd_mw_display_manager.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.24
@COPYRIGHT: FTD co.ltd
****************************************************************************/
#include "ftd_app_burn.h"
#include "ftd_mw_slave_manager.h"
#include "ftd_mw_display_manager.h"
#include "ftd_mw_sys_info_manager.h"
#include "ftd_mw_misc_manager.h"
#include "ftd_mw_log_manager.h"
#include "ftd_drv_panel_uart.h"
#include "ftd_drv_gpio.h"
#include "ftd_data_model.h"

#include "ftd_utils.h"

#include "NuMicro.h"
#include <stdio.h>
#include <stdbool.h>


#define PANNEL_CMD_LEN                      (0x12)  // fixed
static uint8_t pannel_tx_tail[3]            = { 0xff, 0xff, 0xff };
static uint8_t burn_status_ready[4]         = { 0xBE, 0xCD, 0xD0, 0xF7 };             // Jiu Xu
static uint8_t burn_status_connecting[6]    = { 0xC1, 0xAC, 0xBD, 0xD3, 0xD6, 0xD0 }; // LianJie Zhong
static uint8_t burn_status_burnning[6]      = { 0xC9, 0xD5, 0xC2, 0xBC, 0xD6, 0xD0 }; // ShaoLu Zhong
static uint8_t burn_status_verify[6]        = { 0xD0, 0xA3, 0xD1, 0xE9, 0xD6, 0xD0 }; // JiaoYan Zhong
static uint8_t burn_status_pass[4]          = { 0xB3, 0xC9, 0xB9, 0xA6 };             // Cheng Gong
static uint8_t burn_status_ng[4]            = { 0xCA, 0xA7, 0xB0, 0xDC };             // Shi Bai
static uint8_t start_burn[8]                = { 0xBF, 0xAA, 0xCA, 0xBC, 0xC9, 0xD5, 0xC2, 0xBC }; // KaiShi ShaoLu

static uint8_t pannel_tx_buffer[64];
static uint8_t pannel_rx_buffer[PANNEL_CMD_LEN]; // !!!The total data length sent from the panel to the MCU is fixed at 14 bytes.
SYS_INFO g_sys_info;
static DISPLAY_INFO st_display_info;

/**
 * Display manager event callback function pointer
 */
static void (*g_fp_display_event_callback)(PANEL_TO_MCU_CMD event, uint8_t param) = NULL;



/**
 * Build UART packet according to TJC-UART-HMI protocol,No ACK mechanism in panel communication
 *
 * UART-HMI Command Frame Structure:
 * page n \xFF \xFF \xFF
 * p[n].tn.txt="dispaly content"\xFF \xFF \xFF
 * 'n' is page id or txt id
 *
 * @param cmd Command type to be sent
 * @param sub_cmd Sub Command type to be sent
 * @param p_buff_in Pointer to parameter data
 * @param buff_len Length of parameter data in bytes, p_buff[0] is sub cmd id when buff_len is 0
 * @return Total length of the packet including header, command, length field, parameters and checksum
 */
static uint16_t ftd_mw_display_manager_build_cmd_packet(MCU_TO_PANEL_CMD cmd, uint16_t sub_cmd, uint8_t* p_buff_in, uint16_t buff_len);

/**
 * Build UART packet according to TJC-UART-HMI protocol,No ACK mechanism in panel communication
 *
 * UART-HMI Command Frame Structure:
 * page n \xFF \xFF \xFF
 * p[n].tn.txt="dispaly content"\xFF \xFF \xFF
 * 'n' is page id or txt id
 *
 * @param cmd Command type to be sent
 * @param p_buff Pointer to parameter data
 * @param buff_len Length of parameter data in bytes
 * @return Total length of the packet including header, command, length field, parameters and checksum
 */
static uint16_t ftd_mw_display_manager_build_packet(MCU_TO_PANEL_CMD cmd, uint8_t* p_buff, uint16_t buff_len);

static uint16_t ftd_mw_display_manager_msg_parse(void)
{
    uint16_t cmd_type = 0;
    cmd_type = ftd_uart_protocol_decode(pannel_rx_buffer, sizeof(pannel_rx_buffer), 0x08); // 0x08 min length of cmd
    return cmd_type;
}


void ftd_mw_display_manager_sync_channel_burn_status(CHANNEL_BURN_STATUS* p_st_channel_burn_status)
{
    uint16_t packet_len = 0;
    uint16_t wait_time = 1000;

    JYMC_LOG_TRACE("sync channel burn status start");
    // temporary solution , do not check rx complete and fifo empty
    //while ((false == ftd_drv_panel_uart_is_rx_complete()) && (false == ftd_drv_panel_uart_is_rx_fifo_empty()));
    JYMC_LOG_TRACE("sync channel burn status wait 1 end");
    ftd_mw_sys_info_manager_get(&g_sys_info);
    //pkt_len = ftd_mw_display_manager_build_packet(MCU_TO_PANEL_CMD_CHANNEL_STATUS, (uint8_t*)p_st_channel_burn_status, sizeof(CHANNEL_BURN_STATUS));
    //ftd_drv_panel_uart_tx_start(pannel_tx_buffer, pkt_len);

    for (uint8_t i = BURN_PROCESS_BAR; i < BURN_STATUS_MAX; i++)
    {
        packet_len = ftd_mw_display_manager_build_cmd_packet(MCU_TO_PANEL_CMD_CHANNEL_STATUS, i, (uint8_t*)p_st_channel_burn_status, sizeof(CHANNEL_BURN_STATUS));

        if (packet_len > 0)
        {
            //if (BURN_BUTTON_STATUS == j)
                //FTD_LOG_TRACE_BUFF(pannel_tx_buffer, sizeof(pannel_tx_buffer), packet_len);
            ftd_drv_panel_uart_tx_start(pannel_tx_buffer, packet_len);
            while (!ftd_drv_panel_uart_is_tx_complete());
            //FTD_LOG_DEBUG_BUFF(pannel_tx_buffer, sizeof(pannel_tx_buffer), packet_len);
            CLK_SysTickLongDelay(5000);
            
        }
    }

    JYMC_LOG_TRACE("sync channel burn status wait 2 in");
    //while (!ftd_drv_panel_uart_is_tx_complete());
    JYMC_LOG_TRACE("sync channel burn status wait 2 end");
    ftd_drv_panel_uart_rx_start(pannel_rx_buffer, sizeof(pannel_rx_buffer)); // because closed rx's pdma int in tx send
}

void ftd_mw_display_manager_set_button_state(BRUN_STATUS_E state)
{
    CHANNEL_BURN_STATUS st_channel_burn_status;

    st_channel_burn_status.cur_burnning_status = state;

    uint16_t packet_len = ftd_mw_display_manager_build_cmd_packet(MCU_TO_PANEL_CMD_CHANNEL_STATUS, BURN_BUTTON_STATUS,
        (uint8_t*)&st_channel_burn_status, sizeof(CHANNEL_BURN_STATUS));

    if (packet_len > 0)
    {
        ftd_drv_panel_uart_tx_start(pannel_tx_buffer, packet_len);
        //FTD_LOG_DEBUG("SEND:%d, len: 0x%x", i, packet_len);
        while (!ftd_drv_panel_uart_is_tx_complete());
        //FTD_LOG_DEBUG_BUFF(pannel_tx_buffer, sizeof(pannel_tx_buffer), packet_len);
        CLK_SysTickLongDelay(5000);
        ftd_drv_panel_uart_rx_start(pannel_rx_buffer, PANNEL_CMD_LEN);
    }
}

void ftd_mw_display_manager_key_press(void)
{
    if (HMI_FW_BURN_STATUS == st_display_info.curr_page_id)
    {
        uint8_t  app_mode;
        uint16_t wait_time = 1000;

        ftd_mw_misc_manager_buzzer_open(10);

        pannel_tx_buffer[0] = 5;
        uint16_t pkt_len = ftd_mw_display_manager_build_packet(MCU_TO_PANEL_CMD_PAGE_CHANGE, pannel_tx_buffer, 0);
        ftd_drv_panel_uart_tx_start(pannel_tx_buffer, pkt_len);
        while (!ftd_drv_panel_uart_is_tx_complete() && wait_time-- > 0);
        //FTD_LOG_DEBUG_BUFF(pannel_tx_buffer, sizeof(pannel_tx_buffer), pkt_len);
        CLK_SysTickLongDelay(5000);
        if (BURN_STOP == ftd_app_burn_get_general_burn_state() || APP_BURN_ENGINE != app_ftd_state_machine_get_current_state())
        {
            ftd_mw_display_manager_set_button_state(BURN_RUNNING);
            app_mode = APP_BURN_ENGINE;
            g_fp_display_event_callback(PANEL_TO_MCU_CMD_APP_MODE, app_mode);
        }

        ftd_drv_panel_uart_rx_start(pannel_rx_buffer, PANNEL_CMD_LEN);
    }
}


void ftd_mw_display_manager_init(SYS_INFO* p_st_sys_info)
{
    uint16_t packet_len = 0;

    memcpy(&g_sys_info, p_st_sys_info, sizeof(SYS_INFO));
    
    JYMC_LOG_INFO("mw_display init");
    ftd_drv_panel_uart_rx_start(pannel_rx_buffer, PANNEL_CMD_LEN);

    // reset display info
    memset(&st_display_info, 0x00, sizeof(DISPLAY_INFO));

    // send all data to panel
    packet_len = ftd_mw_display_manager_build_cmd_packet(MCU_TO_PANEL_CMD_PAGE_CHANGE, HMI_HOME, NULL, 0);
    if (packet_len > 0)
    {
        ftd_drv_panel_uart_tx_start(pannel_tx_buffer, packet_len);
        while (!ftd_drv_panel_uart_is_tx_complete());
        FTD_LOG_TRACE_BUFF(pannel_tx_buffer, sizeof(pannel_tx_buffer), packet_len);
    }

    // reset display info
    memset(&st_display_info, 0x00, sizeof(DISPLAY_INFO));

    for (uint8_t i = 0; i < SYS_INFO_MAX; i++)
    {
        pannel_tx_buffer[0] = i;
        packet_len = ftd_mw_display_manager_build_packet(MCU_TO_PANEL_CMD_SYS_INFO, pannel_tx_buffer, 0);
        if (packet_len > 0)
        {
            ftd_drv_panel_uart_tx_start(pannel_tx_buffer, packet_len);
            //FTD_LOG_DEBUG("SEND:%d, len: 0x%x", i, packet_len);
            while (!ftd_drv_panel_uart_is_tx_complete());
            //FTD_LOG_DEBUG_BUFF(pannel_tx_buffer, sizeof(pannel_tx_buffer), packet_len);
            CLK_SysTickLongDelay(5000);
        }
    }


    for (uint8_t i = 0; i < BURN_CHANNEL; i++)
    {
        CHANNEL_BURN_STATUS st_channel_burn_status;

        st_channel_burn_status.channel_num = i;
        st_channel_burn_status.cur_burnning_status = BURN_STOP;
        st_channel_burn_status.burn_error_code = SLAVE_CMD_NONE;
        st_channel_burn_status.burn_process_rate = 0x00;
        st_channel_burn_status.burn_total_count = 0x00;
        st_channel_burn_status.burn_success_count = 0x00;

        for (uint8_t j = BURN_PROCESS_BAR; j < BURN_STATUS_MAX; j++)
        {
            packet_len = ftd_mw_display_manager_build_cmd_packet(MCU_TO_PANEL_CMD_CHANNEL_STATUS, j, (uint8_t*)&st_channel_burn_status, sizeof(CHANNEL_BURN_STATUS));

            if (packet_len > 0)
            {
                //if (BURN_BUTTON_STATUS == j)
                    //FTD_LOG_TRACE_BUFF(pannel_tx_buffer, sizeof(pannel_tx_buffer), packet_len);
                ftd_drv_panel_uart_tx_start(pannel_tx_buffer, packet_len);
                while (!ftd_drv_panel_uart_is_tx_complete());
                //FTD_LOG_DEBUG_BUFF(pannel_tx_buffer, sizeof(pannel_tx_buffer), packet_len);
                CLK_SysTickLongDelay(5000);
            }
        }
    }
}


/**
 * Set the event callback function for display manager
 *
 * @param fp_callback Pointer to the callback function
 *
 * @note The callback function will be called when display events occur
 * @note Callback function signature: void callback_func(PANEL_TO_MCU_CMD event, uint8_t param)
 */
void ftd_mw_display_manager_set_event_call_back(void (*fp_callback)(PANEL_TO_MCU_CMD event, uint8_t param))
{
    g_fp_display_event_callback = fp_callback;
}


void ftd_mw_display_manager_rx_process(void)
{
    static bool rx_start = false;
    uint16_t wait_time = 1000;

    if (!rx_start)
    {
        rx_start = true;
        JYMC_LOG_INFO("ftd_drv_panel_uart_rx_start");
        ftd_drv_panel_uart_rx_start(pannel_rx_buffer, PANNEL_CMD_LEN);
    }

    if (ftd_drv_panel_uart_is_rx_complete())
    {
        #if (LOG_TYPE >= LOG_LEVEL_DEBUG)
        FTD_LOG_DEBUG("panel_rx_buffer_len:%d", sizeof(pannel_rx_buffer));
        //FTD_LOG_DEBUG_BUFF(pannel_rx_buffer, sizeof(pannel_rx_buffer), sizeof(pannel_rx_buffer));
        CLK_SysTickLongDelay(5000);
        #endif
        // 1.check and parse rx data valid,and callback
        uint16_t pannel_cmd = ftd_mw_display_manager_msg_parse();
        FTD_LOG_INFO("RECV PANEL_CMD_ID: %d", pannel_cmd);

        ftd_mw_sys_info_manager_get(&g_sys_info);

        switch (pannel_cmd)
        {
            // panel get sys info
            case PNL2WTR_OP_GET_INFO:
            {
                uint16_t packet_len = 0;

                // reset display info
                memset(&st_display_info, 0x00, sizeof(DISPLAY_INFO));
                st_display_info.curr_page_id = HMI_HOME;

                FTD_LOG_DEBUG("RECV PANNEL GET SYS_INFO");
                for (uint8_t i = 0; i < SYS_INFO_MAX; i++)
                {
                    pannel_tx_buffer[0] = i;
                    packet_len = ftd_mw_display_manager_build_packet(MCU_TO_PANEL_CMD_SYS_INFO, pannel_tx_buffer, 0);
                    if (packet_len > 0)
                    {
                        ftd_drv_panel_uart_tx_start(pannel_tx_buffer, packet_len);
                        //FTD_LOG_DEBUG("SEND:%d, len: 0x%x", i, packet_len);
                        while (!ftd_drv_panel_uart_is_tx_complete());
                        //FTD_LOG_DEBUG_BUFF(pannel_tx_buffer, sizeof(pannel_tx_buffer), packet_len);
                        CLK_SysTickLongDelay(5000);
                    }
                }

                FTD_LOG_DEBUG("SEND BURN STATUS INFO");

                for (uint8_t i = 0; i < BURN_CHANNEL; i++)
                {
                    CHANNEL_BURN_STATUS st_channel_burn_status;

                    st_channel_burn_status.channel_num = i;
                    st_channel_burn_status.cur_burnning_status = BURN_STOP;
                    st_channel_burn_status.burn_error_code = SLAVE_CMD_NONE;
                    st_channel_burn_status.burn_process_rate = 0x00;
                    st_channel_burn_status.burn_total_count = 0x00;
                    st_channel_burn_status.burn_success_count = 0x00;

                    for (uint8_t j = BURN_PROCESS_BAR; j < BURN_STATUS_MAX; j++)
                    {
                        packet_len = ftd_mw_display_manager_build_cmd_packet(MCU_TO_PANEL_CMD_CHANNEL_STATUS, j, (uint8_t*)&st_channel_burn_status, sizeof(CHANNEL_BURN_STATUS));

                        if (packet_len > 0)
                        {
                            //if (BURN_BUTTON_STATUS == j)
                                //FTD_LOG_TRACE_BUFF(pannel_tx_buffer, sizeof(pannel_tx_buffer), packet_len);
                            ftd_drv_panel_uart_tx_start(pannel_tx_buffer, packet_len);
                            while (!ftd_drv_panel_uart_is_tx_complete());
                            //FTD_LOG_DEBUG_BUFF(pannel_tx_buffer, sizeof(pannel_tx_buffer), packet_len);
                            CLK_SysTickLongDelay(5000);
                        }
                    }
                }

                break;
            }
            // panel select fw id
            case PNL2WTR_OP_BURN_FW1:
            case PNL2WTR_OP_BURN_FW2:
            case PNL2WTR_OP_BURN_FW3:
            {
                uint8_t  fw_id;
                FTD_LOG_DEBUG("RECV PANNEL FW SELECT %d ", (pannel_cmd - PNL2WTR_OP_BURN_FW1));

                // update display info
                st_display_info.select_fw_id = (pannel_cmd - PNL2WTR_OP_BURN_FW1);
                st_display_info.last_page_id = (pannel_cmd - PNL2WTR_OP_BURN_FW1) + HMI_FW_SELECT_1;
                st_display_info.curr_page_id = HMI_BURN_CHNANEL_SELECT;

                // change to page 8, channel select page
                pannel_tx_buffer[0] = HMI_BURN_CHNANEL_SELECT;
                uint16_t pkt_len = ftd_mw_display_manager_build_packet(MCU_TO_PANEL_CMD_PAGE_CHANGE, pannel_tx_buffer, 0);
                ftd_drv_panel_uart_tx_start(pannel_tx_buffer, pkt_len);
                while (!ftd_drv_panel_uart_is_tx_complete() && wait_time-- > 0);
                //FTD_LOG_DEBUG_BUFF(pannel_tx_buffer, sizeof(pannel_tx_buffer), pkt_len);
                CLK_SysTickLongDelay(5000);

                fw_id = (pannel_cmd - PNL2WTR_OP_BURN_FW1); // 0:fw1, 1:fw2, 2:fw3
                g_fp_display_event_callback(PANEL_TO_MCU_CMD_BURNNING_FW_SELECT, fw_id);

                break;
            }
            case PNL2WTR_OP_BURN_STOP:
            {
                //uint8_t  app_mode;
                FTD_LOG_DEBUG("RECV PANNEL BURN STOP %d ", pannel_cmd);

                //app_mode = APP_PROGRAMMING;
                //g_fp_display_event_callback(PANEL_TO_MCU_CMD_APP_MODE, app_mode);

                ftd_app_burn_set_wind_up(0x01);
                break;
            }
            // panel set burn mode
            case PNL2WTR_OP_BURN_MODE_MANUAL:
            case PNL2WTR_OP_BURN_MODE_AUTO:
            case PNL2WTR_OP_BURN_MODE_BATCH:
            {
                uint8_t  burn_type;
                FTD_LOG_DEBUG("RECV PANNEL BURN MODE %d ", (pannel_cmd - PNL2WTR_OP_BURN_MODE_MANUAL));

                // update display info
                st_display_info.last_page_id = st_display_info.curr_page_id;
                st_display_info.curr_page_id = HMI_FW_BURN_STATUS;

                // change to page 5, burn state page
                pannel_tx_buffer[0] = HMI_FW_BURN_STATUS;
                uint16_t pkt_len = ftd_mw_display_manager_build_packet(MCU_TO_PANEL_CMD_PAGE_CHANGE, pannel_tx_buffer, 0);
                ftd_drv_panel_uart_tx_start(pannel_tx_buffer, pkt_len);
                while (!ftd_drv_panel_uart_is_tx_complete() && wait_time-- > 0);
                //FTD_LOG_DEBUG_BUFF(pannel_tx_buffer, sizeof(pannel_tx_buffer), pkt_len);
                CLK_SysTickLongDelay(5000);
                
                // update burn status display immediately
                for (uint8_t i = 0; i < BURN_CHANNEL; i++)
                {
                    CHANNEL_BURN_STATUS st_channel_burn_status;
                    
                    st_channel_burn_status.channel_num = i;
                    st_channel_burn_status.fw_num = st_display_info.select_fw_id; // use selected firmware id
                    st_channel_burn_status.cur_burnning_status = BURN_STOP;
                    st_channel_burn_status.burn_error_code = SLAVE_CMD_NONE;
                    st_channel_burn_status.burn_process_rate = 0x00;
                    st_channel_burn_status.burn_total_count = 0x00;
                    st_channel_burn_status.burn_success_count = 0x00;
                    
                    ftd_mw_display_manager_sync_channel_burn_status(&st_channel_burn_status);
                }

                burn_type = pannel_cmd - PNL2WTR_OP_BURN_MODE_MANUAL; // 0:manual, 1:auto, 2:batch
                g_fp_display_event_callback(PANEL_TO_MCU_CMD_BURNNING_MODE, burn_type);
                break;
            }
            // panel trigger burn start
            case PNL2WTR_OP_BURN_START:
            {
                uint8_t  app_mode;
                FTD_LOG_DEBUG("RECV PANNEL BURN START %d ", pannel_cmd);

                if (BURN_STOP == ftd_app_burn_get_general_burn_state() || APP_BURN_ENGINE != app_ftd_state_machine_get_current_state())
                {
                    ftd_mw_display_manager_set_button_state(BURN_RUNNING);
                    app_mode = APP_BURN_ENGINE;
                    g_fp_display_event_callback(PANEL_TO_MCU_CMD_APP_MODE, app_mode);
                }

                break;
            }
            // back to last page
            case PNL2WTR_OP_PAGE_5_BURN:
            case PNL2WTR_OP_PAGE_7_MODE:
            case PNL2WTR_OP_PAGE_8_CHANNEL:
            {
                uint16_t packet_len = 0;
                uint16_t page_id = HMI_HOME;

                if (HMI_FW_BURN_STATUS == st_display_info.curr_page_id || PNL2WTR_OP_PAGE_5_BURN == pannel_cmd)
                {
                    page_id = HMI_BURN_MODE_SELECT;
                }
                else if (HMI_BURN_MODE_SELECT == st_display_info.curr_page_id || PNL2WTR_OP_PAGE_7_MODE == pannel_cmd)
                {
                    page_id = HMI_BURN_CHNANEL_SELECT;
                }
                else if (HMI_BURN_CHNANEL_SELECT == st_display_info.curr_page_id || PNL2WTR_OP_PAGE_8_CHANNEL == pannel_cmd)
                {
                    page_id = st_display_info.select_fw_id + HMI_FW_SELECT_1;
                }

                FTD_LOG_TRACE("lst %d cur %d fw_id %d", st_display_info.last_page_id, st_display_info.curr_page_id, st_display_info.select_fw_id);

                // Save select_fw_id before reset
                uint8_t saved_fw_id = st_display_info.select_fw_id;

                st_display_info.last_page_id = st_display_info.curr_page_id;
                st_display_info.curr_page_id = page_id;
                // reset display info but keep select_fw_id
                memset(&st_display_info, 0x00, sizeof(DISPLAY_INFO));
                st_display_info.select_fw_id = saved_fw_id;

                for (uint8_t i = 0; i < SYS_INFO_MAX; i++)
                {
                    pannel_tx_buffer[0] = i;
                    packet_len = ftd_mw_display_manager_build_packet(MCU_TO_PANEL_CMD_SYS_INFO, pannel_tx_buffer, 0);
                    if (packet_len > 0)
                    {
                        ftd_drv_panel_uart_tx_start(pannel_tx_buffer, packet_len);
                        //FTD_LOG_DEBUG("SEND:%d, len: 0x%x", i, packet_len);
                        while (!ftd_drv_panel_uart_is_tx_complete());
                        //FTD_LOG_DEBUG_BUFF(pannel_tx_buffer, sizeof(pannel_tx_buffer), packet_len);
                        CLK_SysTickLongDelay(5000);
                    }
                }

                packet_len = ftd_mw_display_manager_build_cmd_packet(MCU_TO_PANEL_CMD_PAGE_CHANGE, page_id, NULL, 0);

                if (packet_len > 0)
                {
                    ftd_drv_panel_uart_tx_start(pannel_tx_buffer, packet_len);
                    while (!ftd_drv_panel_uart_is_tx_complete());
                    FTD_LOG_TRACE_BUFF(pannel_tx_buffer, sizeof(pannel_tx_buffer), packet_len);
                }

                break;
            }
            case PNL2WTR_OP_SCREEN_VER:
            {
                uint8_t screen_ver = pannel_rx_buffer[FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + 4];
                uint8_t curr_ver = (g_sys_info.st_writer_info.writer_sw_ver[12] - 0x30) << 4 | (g_sys_info.st_writer_info.writer_sw_ver[11] - 0x30);

                FTD_LOG_DEBUG("RECV PANNEL SW VER %x, curr ver %x", screen_ver, curr_ver);

                if (screen_ver != curr_ver)
                {
                    g_sys_info.st_writer_info.writer_sw_ver[12] = (screen_ver & 0xf0) >> 4;
                    g_sys_info.st_writer_info.writer_sw_ver[11] =  screen_ver & 0x0f;
                    g_sys_info.st_writer_info.writer_sw_ver[12] += 0x30;
                    g_sys_info.st_writer_info.writer_sw_ver[11] += 0x30;

                    ftd_mw_sys_info_manager_set(&g_sys_info);
                    ftd_mw_sys_info_manager_save_nv();
                    //ftd_mw_sys_info_manager_writer_info_dump(&g_sys_info.st_writer_info);
                }
                break;
            }
            case PNL2WTR_OP_ENABLE_CHANNEL:
            {
                uint8_t channel_enable_mask = pannel_rx_buffer[FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + 4];
                FTD_LOG_DEBUG("RECV PANNEL ENABLE CHANNEL %x", channel_enable_mask);
                ftd_app_burn_set_channels_enable_by_mask(channel_enable_mask);

                // After channel selection, jump to burn mode select page
                // update display info
                st_display_info.last_page_id = HMI_BURN_CHNANEL_SELECT;
                st_display_info.curr_page_id = HMI_BURN_MODE_SELECT;

                // change to page 7, mode select page
                pannel_tx_buffer[0] = HMI_BURN_MODE_SELECT;
                uint16_t pkt_len = ftd_mw_display_manager_build_packet(MCU_TO_PANEL_CMD_PAGE_CHANGE, pannel_tx_buffer, 0);
                ftd_drv_panel_uart_tx_start(pannel_tx_buffer, pkt_len);
                while (!ftd_drv_panel_uart_is_tx_complete() && wait_time-- > 0);
                //FTD_LOG_DEBUG_BUFF(pannel_tx_buffer, sizeof(pannel_tx_buffer), pkt_len);
                CLK_SysTickLongDelay(5000);

                break;
            }

            default:
                break;
        }

        //2.set rx buffer to zero
        memset(pannel_rx_buffer, 0x00, sizeof(pannel_rx_buffer));

        //3.start rx
        ftd_drv_panel_uart_rx_start(pannel_rx_buffer, PANNEL_CMD_LEN);
    }
}


/**
 * Build UART packet according to TJC-UART-HMI protocol,No ACK mechanism in panel communication
 *
 * UART-HMI Command Frame Structure:
 * page n \xFF \xFF \xFF
 * p[n].tn.txt="dispaly content"\xFF \xFF \xFF
 * 'n' is page id or txt id
 *
 * @param cmd Command type to be sent
 * @param sub_cmd Sub Command type to be sent
 * @param p_buff_in Pointer to parameter data
 * @param buff_len Length of parameter data in bytes
 * @return Total length of the packet including header, command, length field, parameters and checksum
 */
static uint16_t ftd_mw_display_manager_build_cmd_packet(MCU_TO_PANEL_CMD cmd, uint16_t sub_cmd, uint8_t* p_buff_in, uint16_t buff_len)
{
    uint16_t pkt_len = 0;

    if (NULL == p_buff_in && 0 < buff_len)
        return 0;

    // 1. check cmd type and copy p_buff_in
    switch (cmd)
    {
        case MCU_TO_PANEL_CMD_CHANNEL_STATUS:
        {
            CHANNEL_BURN_STATUS* p_st_channel_burn_status;

            if (sizeof(CHANNEL_BURN_STATUS) == buff_len)
                p_st_channel_burn_status = (CHANNEL_BURN_STATUS*)p_buff_in;
            else
                return 0;

            switch (sub_cmd)
            {
                case BURN_PROCESS_BAR:
                {
                    char pannel_tx_info[] = "p[5].j0.val=";

                    pannel_tx_info[6] = p_st_channel_burn_status->channel_num + 0x30; // add 0x30, convert to ASCII

                    memcpy(&pannel_tx_buffer[pkt_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1); // do not copy the last char
                    pkt_len += sizeof(pannel_tx_info) - 1;

                    sprintf((char*)&pannel_tx_buffer[pkt_len], "%03d", p_st_channel_burn_status->burn_process_rate);
                    pkt_len += 0x03;

                    break;
                }
                case BURN_PROCESS_STATUS:
                {
                    char pannel_tx_info[] = "p[5].t0.txt=\"";

                    pannel_tx_info[6] = p_st_channel_burn_status->channel_num + 0x30; // add 0x30, convert to ASCII

                    memcpy(&pannel_tx_buffer[pkt_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    pkt_len += sizeof(pannel_tx_info) - 1;

                    if (BURN_STOP == p_st_channel_burn_status->cur_burnning_status)
                    {
                        memcpy(&pannel_tx_buffer[pkt_len], (uint8_t*)burn_status_ready, sizeof(burn_status_ready));
                        pkt_len += sizeof(burn_status_ready);
                    }
                    else if (BURN_RUNNING == p_st_channel_burn_status->cur_burnning_status)
                    {
                        if (SLAVE_CMD_NONE == p_st_channel_burn_status->burn_error_code)
                        {
                            if (SLAVE_CMD_ENTER_BOOT_LOADER == p_st_channel_burn_status->burn_process_state)
                            {
                                memcpy(&pannel_tx_buffer[pkt_len], (uint8_t*)burn_status_connecting, sizeof(burn_status_connecting));
                                pkt_len += sizeof(burn_status_connecting);
                            }
                            else if (SLAVE_CMD_READ_CODE_CRC == p_st_channel_burn_status->burn_process_state || 
                                SLAVE_CMD_READ_CODE_CRC_PARM == p_st_channel_burn_status->burn_process_state)
                            {
                                memcpy(&pannel_tx_buffer[pkt_len], (uint8_t*)burn_status_verify, sizeof(burn_status_verify));
                                pkt_len += sizeof(burn_status_verify);
                            }
                            else if (SLAVE_CMD_STATE_SORT == p_st_channel_burn_status->burn_process_state)
                            {
                                memcpy(&pannel_tx_buffer[pkt_len], (uint8_t*)burn_status_pass, sizeof(burn_status_pass));
                                pkt_len += sizeof(burn_status_pass);
                            }
                            else
                            {
                                memcpy(&pannel_tx_buffer[pkt_len], (uint8_t*)burn_status_burnning, sizeof(burn_status_burnning));
                                pkt_len += sizeof(burn_status_burnning);
                            }
                        }
                        else
                        {
                            memcpy(&pannel_tx_buffer[pkt_len], (uint8_t*)burn_status_ng, sizeof(burn_status_ng));
                            pkt_len += sizeof(burn_status_ng);
                        }
                    }
                    else if (BURN_DONE == p_st_channel_burn_status->cur_burnning_status)
                    {
                        if (SLAVE_CMD_NONE == p_st_channel_burn_status->burn_error_code)
                        {
                            memcpy(&pannel_tx_buffer[pkt_len], (uint8_t*)burn_status_pass, sizeof(burn_status_pass));
                            pkt_len += sizeof(burn_status_pass);
                        }
                        else
                        {
                            memcpy(&pannel_tx_buffer[pkt_len], (uint8_t*)burn_status_ng, sizeof(burn_status_ng));
                            pkt_len += sizeof(burn_status_ng);
                        }
                    }

                    pannel_tx_buffer[pkt_len++] = '"';

                    break;
                }
                case BURN_CNT:
                {
                    char pannel_tx_info[] = "p[5].t16.txt=\"";

                    pannel_tx_info[7] += p_st_channel_burn_status->channel_num;

                    memcpy(&pannel_tx_buffer[pkt_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    pkt_len += sizeof(pannel_tx_info) - 1;

                    sprintf((char*)&pannel_tx_buffer[pkt_len], "%05d", p_st_channel_burn_status->burn_success_count);
                    pkt_len += 5;

                    sprintf((char*)&pannel_tx_buffer[pkt_len], "/");
                    pkt_len += 1;

                    sprintf((char*)&pannel_tx_buffer[pkt_len], "%05d", p_st_channel_burn_status->burn_total_count);
                    pkt_len += 5;

                    pannel_tx_buffer[pkt_len++] = '"';

                    break;
                }
                case BURN_REMAIN_CNT:
                {
                    char pannel_tx_info[] = "p[5].t14.txt=\"";
                    memcpy(&pannel_tx_buffer[pkt_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    pkt_len += sizeof(pannel_tx_info) - 1;

                    sprintf((char*)&pannel_tx_buffer[pkt_len], "%d", g_sys_info.st_fw_info[p_st_channel_burn_status->fw_num].st_bin_info.remain_counts);
                    pkt_len += sizeof(g_sys_info.st_fw_info[p_st_channel_burn_status->fw_num].st_bin_info.remain_counts) * 4;
                    pannel_tx_buffer[pkt_len++] = '"';

                    break;
                }

                case BURN_BUTTON_STATUS:
                {
                    char pannel_tx_info[] = "p[5].t12.txt=\"";
                    memcpy(&pannel_tx_buffer[pkt_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    pkt_len += sizeof(pannel_tx_info) - 1;

                    /* burn engine is running */
                    if (BURN_RUNNING == p_st_channel_burn_status->cur_burnning_status || BURN_RUNNING == ftd_app_burn_get_general_burn_state())
                    {
                        memcpy(&pannel_tx_buffer[pkt_len], (uint8_t*)burn_status_burnning, sizeof(burn_status_burnning));
                        pkt_len += sizeof(burn_status_burnning);
                    }
                    /* burn engine is stop */
                    else if (BURN_STOP == ftd_app_burn_get_general_burn_state() || BURN_DONE == ftd_app_burn_get_general_burn_state())
                    {
                        memcpy(&pannel_tx_buffer[pkt_len], (uint8_t*)start_burn, sizeof(start_burn));
                        pkt_len += sizeof(start_burn);
                    }

                    pannel_tx_buffer[pkt_len++] = '"';

                    break;
                }

                case BURN_CHIP_TYPE:
                {
                    char pannel_tx_info[] = "p[5].t10.txt=\"";
                    memcpy(&pannel_tx_buffer[pkt_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    pkt_len += sizeof(pannel_tx_info) - 1;

                    sprintf((char*)&pannel_tx_buffer[pkt_len], "%04X", g_sys_info.st_fw_info[p_st_channel_burn_status->fw_num].chip_id);

                    pkt_len += 4;
                    pannel_tx_buffer[pkt_len++] = '"';

                    break;
                }
                case BURN_FW_VER:
                {
                    char pannel_tx_info[] = "p[5].t13.txt=\"";
                    memcpy(&pannel_tx_buffer[pkt_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    pkt_len += sizeof(pannel_tx_info) - 1;

                    memcpy(&pannel_tx_buffer[pkt_len], (uint8_t*)g_sys_info.st_fw_info[p_st_channel_burn_status->fw_num].st_bin_info.ver, sizeof(g_sys_info.st_fw_info[p_st_channel_burn_status->fw_num].st_bin_info.ver));

                    ////FTD_LOG_DEBUG_BUFF(g_sys_info.st_fw_info[p_st_channel_burn_status->fw_num].st_bin_info.ver, sizeof(g_sys_info.st_fw_info[p_st_channel_burn_status->fw_num].st_bin_info.ver), 
                    //    sizeof(g_sys_info.st_fw_info[p_st_channel_burn_status->fw_num].st_bin_info.ver));

                    pkt_len += sizeof(g_sys_info.st_fw_info[p_st_channel_burn_status->fw_num].st_bin_info.ver);
                    pannel_tx_buffer[pkt_len++] = '"';

                    break;
                }

                default:
                    break;
            }
            break;
        }
        case MCU_TO_PANEL_CMD_PAGE_CHANGE:
        {
            char pannel_tx_info[] = "page 0"; // actual size is 7

            pannel_tx_info[5] = sub_cmd & 0xFF;
            pannel_tx_info[5] += 0x30; // convert to ASCII

            memcpy(&pannel_tx_buffer[pkt_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1); // do not copy the last char
            pkt_len += sizeof(pannel_tx_info) - 1;

            break;
        }
        default:
        {
            break;
        }

    }

    // 2. add cmd tail
    if (0 != pkt_len)
    {
        memcpy(&pannel_tx_buffer[pkt_len], pannel_tx_tail, sizeof(pannel_tx_tail));
        pkt_len += sizeof(pannel_tx_tail);
    }

    return pkt_len;
}
/**
 * Build UART packet according to TJC-UART-HMI protocol,No ACK mechanism in panel communication
 *
 * UART-HMI Command Frame Structure:
 * page n \xFF \xFF \xFF
 * p[n].tn.txt="dispaly content"\xFF \xFF \xFF
 * 'n' is page id or txt id
 *
 * @param cmd Command type to be sent
 * @param p_buff Pointer to parameter data
 * @param buff_len Length of parameter data in bytes
 * @return Total length of the packet including header, command, length field, parameters and checksum
 */
static uint16_t ftd_mw_display_manager_build_packet(MCU_TO_PANEL_CMD cmd, uint8_t* p_buff, uint16_t buff_len)
{
    if (p_buff == NULL)
    {
        return 0;
    }

    // 1. check cmd type and copy p_param
    switch (cmd)
    {
        case MCU_TO_PANEL_CMD_PAGE_CHANGE:
        {
            p_buff[5] = p_buff[0] + 0x30;

            p_buff[0] = 'p';
            p_buff[1] = 'a';
            p_buff[2] = 'g';
            p_buff[3] = 'e';
            p_buff[4] = ' ';

            buff_len = 6;
            break;
        }
        case MCU_TO_PANEL_CMD_SYS_INFO:
        {
            SYS_INFO sys_info;
            ftd_mw_sys_info_manager_get(&sys_info);

            switch (p_buff[0])
            {
                case SYS_INFO_HW_VER:
                {
                    char pannel_tx_info[] = "p[0].t4.txt=\"";
                    endian_reverse_buff((uint8_t*)&sys_info.st_writer_info.writer_hw_ver, sizeof(sys_info.st_writer_info.writer_hw_ver));

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    buff_len += sizeof(pannel_tx_info) - 1;
                    memcpy(&p_buff[buff_len], (uint8_t*)&sys_info.st_writer_info.writer_hw_ver, 0x09);
                    buff_len += 0x09;
                    p_buff[buff_len++] = '"';

                    break;
                }
                case SYS_INFO_SW_VER:
                {
                    char pannel_tx_info[] = "p[0].t5.txt=\"";
                    endian_reverse_buff((uint8_t*)&sys_info.st_writer_info.writer_sw_ver, sizeof(sys_info.st_writer_info.writer_sw_ver));

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    buff_len += sizeof(pannel_tx_info) - 1;
                    memcpy(&p_buff[buff_len], (uint8_t*)&sys_info.st_writer_info.writer_sw_ver, 0x09);
                    buff_len += 0x09;
                    p_buff[buff_len++] = '"';
                    break;
                }
                case SYS_INFO_SN:
                {
                    char pannel_tx_info[] = "p[0].t6.txt=\"";
                    endian_reverse_buff((uint8_t*)&sys_info.st_writer_info.writer_sn, sizeof(sys_info.st_writer_info.writer_sn));

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info));
                    buff_len += sizeof(pannel_tx_info) - 1;
                    memcpy(&p_buff[buff_len], (uint8_t*)&sys_info.st_writer_info.writer_sn, 0x09);
                    buff_len += 0x09;
                    p_buff[buff_len++] = '"';

                    break;
                }
                case SYS_INFO_FW1_CHIP_ID:
                {
                    char pannel_tx_info[] = "p[2].t6.txt=\"";

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    buff_len += sizeof(pannel_tx_info) - 1;
                    sprintf((char*)&p_buff[buff_len], "%04X", sys_info.st_fw_info[0].chip_id);
                    FTD_LOG_TRACE("p[2].t6.txt=%04X", sys_info.st_fw_info[0].chip_id);
                    buff_len += 4;
                    p_buff[buff_len++] = '"';
                    break;
                }
                case SYS_INFO_FW1_VER:
                {
                    char pannel_tx_info[] = "p[2].t7.txt=\"";
                    //endian_reverse_buff((uint8_t*)&sys_info.st_fw_info[0].st_bin_info.ver, sizeof(sys_info.st_fw_info[0].st_bin_info.ver));

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    buff_len += sizeof(pannel_tx_info) - 1;
                    memcpy(&p_buff[buff_len], (uint8_t*)&sys_info.st_fw_info[0].st_bin_info.ver, sizeof(sys_info.st_fw_info[0].st_bin_info.ver));
                    FTD_LOG_TRACE("p[2].t7.txt=%d", sys_info.st_fw_info[0].st_bin_info.ver);
                    buff_len += sizeof(sys_info.st_fw_info[0].st_bin_info.ver);
                    p_buff[buff_len++] = '"';
                    break;
                }
                case SYS_INFO_FW1_BURNED_CNT:
                {
                    char pannel_tx_info[] = "p[2].t8.txt=\"";

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    buff_len += sizeof(pannel_tx_info) - 1;
                    sprintf((char*)&p_buff[buff_len], "%d", sys_info.st_fw_info[0].st_bin_info.remain_counts);
                    FTD_LOG_TRACE("p[2].t8.txt=%d", sys_info.st_fw_info[0].st_bin_info.remain_counts);
                    buff_len += strlen((char*)&p_buff[buff_len]);
                    p_buff[buff_len++] = '"';
                    break;
                }
                case SYS_INFO_FW1_TRIPLE_REMAIN_CNT:
                {
                    char pannel_tx_info[] = "p[2].t9.txt=\"";

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    buff_len += sizeof(pannel_tx_info) - 1;
                    sprintf((char*)&p_buff[buff_len], "%d", sys_info.st_fw_info[0].st_triple_info.remain_counts);
                    FTD_LOG_TRACE("p[2].t9.txt=%d", sys_info.st_fw_info[0].st_triple_info.remain_counts);
                    buff_len += sizeof(sys_info.st_fw_info[0].st_triple_info.remain_counts) * 2;
                    p_buff[buff_len++] = '"';
                    break;
                }
                case SYS_INFO_FW1_ROLLCODE_REMAIN_CNT:
                {
                    char pannel_tx_info[] = "p[2].t10.txt=\"";

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, 14);
                    buff_len += 14;
                    sprintf((char*)&p_buff[buff_len], "%d", sys_info.st_fw_info[0].st_roll_code_info.remain_counts);
                    FTD_LOG_TRACE("p[2].t10.txt=%d", sys_info.st_fw_info[0].st_roll_code_info.remain_counts);
                    buff_len += sizeof(sys_info.st_fw_info[0].st_roll_code_info.remain_counts);
                    p_buff[buff_len++] = '"';
                    break;
                }
                case SYS_INFO_FW2_CHIP_ID:
                {
                    char pannel_tx_info[] = "p[3].t6.txt=\"";

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    buff_len += sizeof(pannel_tx_info) - 1;
                    sprintf((char*)&p_buff[buff_len], "%04X", sys_info.st_fw_info[1].chip_id);
                    buff_len += 4;
                    p_buff[buff_len++] = '"';
                    break;
                }
                case SYS_INFO_FW2_VER:
                {
                    char pannel_tx_info[] = "p[3].t7.txt=\"";
                    //endian_reverse_buff((uint8_t*)&sys_info.st_fw_info[1].st_bin_info.ver, sizeof(sys_info.st_fw_info[1].st_bin_info.ver));

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    buff_len += sizeof(pannel_tx_info) - 1;
                    memcpy(&p_buff[buff_len], (uint8_t*)&sys_info.st_fw_info[1].st_bin_info.ver, sizeof(sys_info.st_fw_info[1].st_bin_info.ver));
                    buff_len += sizeof(sys_info.st_fw_info[1].st_bin_info.ver);
                    p_buff[buff_len++] = '"';
                    break;
                }
                case SYS_INFO_FW2_BURNED_CNT:
                {
                    char pannel_tx_info[] = "p[3].t8.txt=\"";

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    buff_len += sizeof(pannel_tx_info) - 1;
                    sprintf((char*)&p_buff[buff_len], "%d", sys_info.st_fw_info[1].st_bin_info.remain_counts);
                    buff_len += strlen((char*)&p_buff[buff_len]);
                    p_buff[buff_len++] = '"';
                    break;
                }
                case SYS_INFO_FW2_TRIPLE_REMAIN_CNT:
                {
                    char pannel_tx_info[] = "p[3].t9.txt=\"";

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    buff_len += sizeof(pannel_tx_info) - 1;
                    sprintf((char*)&p_buff[buff_len], "%d", sys_info.st_fw_info[1].st_triple_info.remain_counts);
                    buff_len += sizeof(sys_info.st_fw_info[1].st_triple_info.remain_counts);
                    p_buff[buff_len++] = '"';
                    break;
                }
                case SYS_INFO_FW2_ROLLCODE_REMAIN_CNT:
                {
                    char pannel_tx_info[] = "p[3].t10.txt=\"";

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, 14);
                    buff_len += 14;
                    sprintf((char*)&p_buff[buff_len], "%d", sys_info.st_fw_info[1].st_roll_code_info.remain_counts);
                    buff_len += sizeof(sys_info.st_fw_info[1].st_roll_code_info.remain_counts);
                    p_buff[buff_len++] = '"';
                    break;
                }
                case SYS_INFO_FW3_CHIP_ID:
                {
                    char pannel_tx_info[] = "p[4].t6.txt=\"";

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    buff_len += sizeof(pannel_tx_info) - 1;
                    sprintf((char*)&p_buff[buff_len], "%04X", sys_info.st_fw_info[2].chip_id);
                    buff_len += 4;
                    p_buff[buff_len++] = '"';
                    break;
                }
                case SYS_INFO_FW3_VER:
                {
                    char pannel_tx_info[] = "p[4].t7.txt=\"";
                    //endian_reverse_buff((uint8_t*)&sys_info.st_fw_info[2].st_bin_info.ver, sizeof(sys_info.st_fw_info[2].st_bin_info.ver));

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    buff_len += sizeof(pannel_tx_info) - 1;
                    memcpy(&p_buff[buff_len], (uint8_t*)&sys_info.st_fw_info[2].st_bin_info.ver, sizeof(sys_info.st_fw_info[2].st_bin_info.ver));
                    buff_len += sizeof(sys_info.st_fw_info[2].st_bin_info.ver);
                    p_buff[buff_len++] = '"';
                    break;
                }
                case SYS_INFO_FW3_BURNED_CNT:
                {
                    char pannel_tx_info[] = "p[4].t8.txt=\"";

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    buff_len += sizeof(pannel_tx_info) - 1;
                    sprintf((char*)&p_buff[buff_len], "%d", sys_info.st_fw_info[2].st_bin_info.remain_counts);
                    buff_len += strlen((char*)&p_buff[buff_len]);
                    p_buff[buff_len++] = '"';
                    break;
                }
                case SYS_INFO_FW3_TRIPLE_REMAIN_CNT:
                {
                    char pannel_tx_info[] = "p[4].t9.txt=\"";

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    buff_len += sizeof(pannel_tx_info) - 1;
                    sprintf((char*)&p_buff[buff_len], "%d", sys_info.st_fw_info[2].st_triple_info.remain_counts);
                    buff_len += sizeof(sys_info.st_fw_info[2].st_triple_info.remain_counts);
                    p_buff[buff_len++] = '"';
                    break;
                }
                case SYS_INFO_FW3_ROLLCODE_REMAIN_CNT:
                {
                    char pannel_tx_info[] = "p[4].t10.txt=\"";

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    buff_len += sizeof(pannel_tx_info) - 1;
                    sprintf((char*)&p_buff[buff_len], "%d", sys_info.st_fw_info[2].st_roll_code_info.remain_counts);
                    buff_len += sizeof(sys_info.st_fw_info[2].st_roll_code_info.remain_counts);
                    p_buff[buff_len++] = '"';
                    break;
                }
                default:
                {
                    break;
                }

            }
            break;
        }
        default:
        {
            break;
        }
    }

    // 2. add cmd tail
    memcpy(&p_buff[buff_len], pannel_tx_tail, sizeof(pannel_tx_tail));
    buff_len += sizeof(pannel_tx_tail);

    return buff_len;
}
