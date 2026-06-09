/****************************************************************************
@FILENAME:  ftd_mw_display_manager.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.24
@COPYRIGHT: FTD co.ltd
****************************************************************************/
#include "ftd_mw_display_manager.h"
#include "ftd_mw_sys_info_manager.h"
#include "ftd_mw_log_manager.h"
#include "ftd_drv_panel_uart.h"
#include "ftd_drv_gpio.h"

#include "ftd_utils.h"

#include "NuMicro.h"
#include <stdio.h>
#include <stdbool.h>


#define PANNEL_CMD_LEN          (0x12)  // fixed
static uint8_t pannel_tx_tail[3] = { 0xff, 0xff, 0xff };
static uint8_t pannel_tx_buffer[32];
static uint8_t pannel_rx_buffer[PANNEL_CMD_LEN]; // !!!The total data length sent from the panel to the MCU is fixed at 14 bytes.

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

void ftd_mw_display_manager_init(SYS_INFO* p_st_sys_info)
{
    // do nothing.
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
        FTD_LOG_DEBUG_BUFF(pannel_rx_buffer, sizeof(pannel_rx_buffer), sizeof(pannel_rx_buffer));
#endif
        // 1.check and parse rx data valid,and callback
        uint16_t pannel_cmd = ftd_mw_display_manager_msg_parse();
        FTD_LOG_INFO("RECV PANEL_CMD_ID: %d", pannel_cmd);

        switch (pannel_cmd)
        {
            case PNL2WTR_OP_GET_INFO:
            {
                uint16_t packet_len = 0;

                FTD_LOG_DEBUG("RECV PANNEL GET SYS_INFO");
                for (uint8_t i = 0; i < SYS_INFO_MAX; i++)
                {
                    pannel_tx_buffer[0] = i;
                    packet_len = ftd_mw_display_manager_build_packet(MCU_TO_PANEL_CMD_SYS_INFO, pannel_tx_buffer, 0);
                    if (packet_len > 0)
                    {
                        ftd_drv_panel_uart_tx_start(pannel_tx_buffer, packet_len);
                        //FTD_LOG_DEBUG("SEND:%d, len: 0x%x", i, packet_len);
                        //FTD_LOG_DEBUG_BUFF(pannel_tx_buffer, sizeof(pannel_tx_buffer), packet_len);
                        while (!ftd_drv_panel_uart_is_tx_complete());
                    }
                }
                break;
            }
            case PNL2WTR_OP_BURN_FW1:
            case PNL2WTR_OP_BURN_FW2:
            case PNL2WTR_OP_BURN_FW3:
            {
                uint8_t  fw_id;
                FTD_LOG_DEBUG("RECV PANNEL FW SELECT %d ", (pannel_cmd - PNL2WTR_OP_BURN_FW1));

                fw_id = (pannel_cmd - PNL2WTR_OP_BURN_FW1); // 0:fw1, 1:fw2, 2:fw3
                g_fp_display_event_callback(PANEL_TO_MCU_CMD_BURNNING_FW_SELECT, fw_id);
                break;
            }
            case PNL2WTR_OP_BURN_STOP:
            {
                uint8_t  app_mode;
                FTD_LOG_DEBUG("RECV PANNEL BURN STOP %d ", pannel_cmd);

                // send panel burn status
                //pannel_tx_buffer[0] = i;
                //packet_len = ftd_mw_display_manager_build_packet(MCU_TO_PANEL_CMD_PAGE_CHANGE, pannel_tx_buffer, 0);

                app_mode = APP_PROGRAMMING;
                g_fp_display_event_callback(PANEL_TO_MCU_CMD_APP_MODE, app_mode);
                break;
            }
            case PNL2WTR_OP_BURN_MODE_MANUAL:
            case PNL2WTR_OP_BURN_MODE_AUTO:
            {
                uint8_t  burn_type;
                FTD_LOG_DEBUG("RECV PANNEL BURN MODE %d ", (pannel_cmd - PNL2WTR_OP_BURN_MODE_MANUAL + 1));

                burn_type = pannel_cmd - PNL2WTR_OP_BURN_MODE_MANUAL + 1; // 1:manual, 2:auto
                g_fp_display_event_callback(PANEL_TO_MCU_CMD_BURNNING_MODE, burn_type);
                break;
            }
            case PNL2WTR_OP_BURN_START:
            {
                uint8_t  app_mode;
                FTD_LOG_DEBUG("RECV PANNEL BURN START %d ", pannel_cmd);

                app_mode = APP_BURN_ENGINE;
                g_fp_display_event_callback(PANEL_TO_MCU_CMD_APP_MODE, app_mode);
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

void ftd_mw_display_manager_sync_channel_burn_status(CHANNEL_BURN_STATUS* p_st_channel_burn_status)
{
    uint16_t pkt_len = 0;
    JYMC_LOG_TRACE("sync channel burn status start");
    // temporary solution , do not check rx complete and fifo empty
    //while ((false == ftd_drv_panel_uart_is_rx_complete()) && (false == ftd_drv_panel_uart_is_rx_fifo_empty()));
    JYMC_LOG_TRACE("sync channel burn status wait 1 end");

    pkt_len = ftd_mw_display_manager_build_packet(MCU_TO_PANEL_CMD_CHANNEL_STATUS, (uint8_t*)p_st_channel_burn_status, sizeof(CHANNEL_BURN_STATUS));
    ftd_drv_panel_uart_tx_start(pannel_tx_buffer, pkt_len);

    JYMC_LOG_TRACE("sync channel burn status wait 2 in");
    while (!ftd_drv_panel_uart_is_tx_complete());
    JYMC_LOG_TRACE("sync channel burn status wait 2 end");
    ftd_drv_panel_uart_rx_start(pannel_rx_buffer, sizeof(pannel_rx_buffer)); // because closed rx's pdma int in tx send
}

void ftd_mw_display_manager_key_press(void)
{
    uint16_t pkt_len = 0;
   // while ((false == ftd_drv_panel_uart_is_rx_complete()) && (false == ftd_drv_panel_uart_is_rx_fifo_empty()));
    pkt_len = ftd_mw_display_manager_build_packet(MCU_TO_PANEL_CMD_KEY_PRESS, NULL, 0);
    ftd_drv_panel_uart_tx_start(pannel_tx_buffer, pkt_len);
    while (!ftd_drv_panel_uart_is_tx_complete());
    ftd_drv_panel_uart_rx_start(pannel_rx_buffer, sizeof(pannel_rx_buffer));
    JYMC_LOG_INFO("ftd_mw_display_manager_key_press-pkt_len[%d]\n", pkt_len);
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
            // copy data
            for (uint8_t i = 0; i < buff_len; i++)
            {
                p_buff[5 + i] = p_buff[i];
            }

            p_buff[0] = 'p';
            p_buff[1] = 'a';
            p_buff[2] = 'g';
            p_buff[3] = 'e';
            p_buff[4] = ' ';

            buff_len += 5;
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
                    sprintf((char*)&p_buff[buff_len], "%d", sys_info.st_fw_info[0].chip_id);
                    FTD_LOG_TRACE("p[2].t6.txt=%d", sys_info.st_fw_info[0].chip_id);
                    buff_len += sizeof(sys_info.st_fw_info[0].chip_id);
                    p_buff[buff_len++] = '"';
                    break;
                }
                case SYS_INFO_FW1_VER:
                {
                    char pannel_tx_info[] = "p[2].t7.txt=\"";
                    endian_reverse_buff((uint8_t*)&sys_info.st_fw_info[0].st_bin_info.ver, sizeof(sys_info.st_fw_info[0].st_bin_info.ver));

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
                    buff_len += sizeof(sys_info.st_fw_info[0].st_bin_info.remain_counts);
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
                    buff_len += sizeof(sys_info.st_fw_info[0].st_triple_info.remain_counts);
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
                    sprintf((char*)&p_buff[buff_len], "%d", sys_info.st_fw_info[1].chip_id);
                    buff_len += sizeof(sys_info.st_fw_info[1].chip_id);
                    p_buff[buff_len++] = '"';
                    break;
                }
                case SYS_INFO_FW2_VER:
                {
                    char pannel_tx_info[] = "p[3].t7.txt=\"";
                    endian_reverse_buff((uint8_t*)&sys_info.st_fw_info[1].st_bin_info.ver, sizeof(sys_info.st_fw_info[1].st_bin_info.ver));

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
                    buff_len += sizeof(sys_info.st_fw_info[1].st_bin_info.remain_counts);
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
                    char pannel_tx_info[] = "p[3].t6.txt=\"";

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    buff_len += sizeof(pannel_tx_info) - 1;
                    sprintf((char*)&p_buff[buff_len], "%d", sys_info.st_fw_info[2].chip_id);
                    buff_len += sizeof(sys_info.st_fw_info[2].chip_id);
                    p_buff[buff_len++] = '"';
                    break;
                }
                case SYS_INFO_FW3_VER:
                {
                    char pannel_tx_info[] = "p[3].t7.txt=\"";
                    endian_reverse_buff((uint8_t*)&sys_info.st_fw_info[2].st_bin_info.ver, sizeof(sys_info.st_fw_info[2].st_bin_info.ver));

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    buff_len += sizeof(pannel_tx_info) - 1;
                    memcpy(&p_buff[buff_len], (uint8_t*)&sys_info.st_fw_info[2].st_bin_info.ver, sizeof(sys_info.st_fw_info[2].st_bin_info.ver));
                    buff_len += sizeof(sys_info.st_fw_info[2].st_bin_info.ver);
                    p_buff[buff_len++] = '"';
                    break;
                }
                case SYS_INFO_FW3_BURNED_CNT:
                {
                    char pannel_tx_info[] = "p[3].t8.txt=\"";

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    buff_len += sizeof(pannel_tx_info) - 1;
                    sprintf((char*)&p_buff[buff_len], "%d", sys_info.st_fw_info[2].st_bin_info.remain_counts);
                    buff_len += sizeof(sys_info.st_fw_info[2].st_bin_info.remain_counts);
                    p_buff[buff_len++] = '"';
                    break;
                }
                case SYS_INFO_FW3_TRIPLE_REMAIN_CNT:
                {
                    char pannel_tx_info[] = "p[3].t9.txt=\"";

                    memcpy(&p_buff[buff_len], (uint8_t*)pannel_tx_info, sizeof(pannel_tx_info) - 1);
                    buff_len += sizeof(pannel_tx_info) - 1;
                    sprintf((char*)&p_buff[buff_len], "%d", sys_info.st_fw_info[2].st_triple_info.remain_counts);
                    buff_len += sizeof(sys_info.st_fw_info[2].st_triple_info.remain_counts);
                    p_buff[buff_len++] = '"';
                    break;
                }
                case SYS_INFO_FW3_ROLLCODE_REMAIN_CNT:
                {
                    char pannel_tx_info[] = "p[3].t10.txt=\"";

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
