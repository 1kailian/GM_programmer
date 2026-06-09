/****************************************************************************
@FILENAME:  ftd_app_prog.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.15
@COPYRIGHT: FTD co.ltd
****************************************************************************/
#include "ftd_app_burn.h"
#include "ftd_mw_slave_manager.h"
#include "ftd_app_prog.h"
#include "ftd_prog_event.h"
#include "ftd_mw_cache_manager.h"
#include "ftd_mw_sys_info_manager.h"
#include "ftd_mw_fw_manager.h"
#include "ftd_mw_display_manager.h"
#include "ftd_mw_hc_manager.h"
#include "ftd_mw_log_manager.h"
#include "ftd_utils.h"
#include "ftd_data_model.h"
#include "stdint.h"
#include "NuMicro.h"

#include "ftd_mw_iap_manager.h"

#if DEBUG_USE_UART4
#define PROG_LOG_ERROR          FTD_LOG_ERROR
#define PROG_LOG_WARN           FTD_LOG_WARN
#define PROG_LOG_INFO           FTD_LOG_INFO
#define PROG_LOG_DEBUG          FTD_LOG_DEBUG
#define PROG_LOG_DEBUG_BUFF     FTD_LOG_DEBUG_BUFF
#define PROG_LOG_TRACE
#else
#define PROG_LOG_ERROR
#define PROG_LOG_WARN
#define PROG_LOG_INFO
#define PROG_LOG_DEBUG
#define PROG_LOG_DEBUG_BUFF
#define PROG_LOG_TRACE
#endif

static uint8_t hc_uart_buffer[PROG_COMMS_BUFF_LEN];

void ftd_app_prog_init(void)
{
    //host computer uart init
    PROG_LOG_INFO("ftd_app_prog_init \n");
}

CHANNEL_BURN_STATUS st_channel_burn_status_test;
void ftd_app_prog_process(void)
{
    uint16_t param_len = 0;
    uint16_t seg_pkt_num = 0;
    int16_t packet_len = 0;
    uint8_t ack_status = 0;
    FW_INFO st_fw_info;
    SYS_INFO st_sys_info;
    FTD_PROG_EVENT en_ftd_hc_event = FTD_PROG_EVENT_NONE;

    bool b_update_display = false;
    // ftd_mw_display_manager_init(&st_sys_info);//test

    //1.receive host computer data
    if (ftd_mw_hc_manager_receive_data(hc_uart_buffer) == 1)
    {
        b_update_display = true;

#if (LOG_TYPE >= LOG_LEVEL_DEBUG)
        // print received buffer for debug
        uint16_t parm_len_idx = FRAME_HEADER_SIZE + CMD_SIZE;
        uint16_t parm_len = (hc_uart_buffer[parm_len_idx] << 8 | hc_uart_buffer[parm_len_idx + 1]);
        uint16_t total_len = FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + parm_len + CHECKSUM_SIZE;
        PROG_LOG_DEBUG("\n___RECV___ hc_uart_buffer[0x%02x] : ", parm_len);

        PROG_LOG_DEBUG_BUFF(hc_uart_buffer, sizeof(hc_uart_buffer), total_len);
#endif
        PROG_LOG_INFO("APP_PROG [0x%x][0x%x][0x%x][0x%x][0x%x] \n", hc_uart_buffer[0], hc_uart_buffer[1], hc_uart_buffer[2], hc_uart_buffer[3], hc_uart_buffer[4]);
        //2.check data is ok
        if (ftd_mw_hc_manager_check_data(hc_uart_buffer) == 1)
        {
            //3. get event type from hc_uart_buffer, convert cmd type to event type
            en_ftd_hc_event = ftd_mw_hc_manager_get_event_type(hc_uart_buffer);
            PROG_LOG_INFO(" en_ftd_hc_event : %d\n", en_ftd_hc_event);

            switch (en_ftd_hc_event)
            {
                case FTD_PROG_EVENT_CHECK_ONLINE:
                {
                    ack_status = 0x01;//online
                    packet_len = ftd_mw_hc_manager_build_packet(en_ftd_hc_event, hc_uart_buffer, &ack_status, 1);
                    break;
                }
                case FTD_PROG_EVENT_ENTER_IAP_MODE:
                {
                    IAP_FLAG_T flag;
                    flag.start_magic = IAP_MAGIC;
                    flag.upgrade = 0x01;
                    flag.app_select = 2;
                    ftd_mw_iap_manager_flag_save(flag);
                    ftd_mw_iap_manager_reset();
                }
                /// TEMP USE FOR BURN TEST, DEBUG ONLY //
                case FTD_PROG_EVENT_BURN_START_TEST_TMP:
                case FTD_PROG_EVENT_BURN_START_TMP:
                {
                    uint8_t test_addr;
                    ack_status = 0x00;

                    PROG_LOG_INFO(" FTD_PROG_EVENT_BURN %d \n ", hc_uart_buffer[11]);

                    test_addr = (FTD_PROG_EVENT_BURN_START_TEST_TMP == en_ftd_hc_event) ? 0x01 : 0x00;
                    ftd_app_burn_debug_hc_set_burn_addr(test_addr);

                    ftd_app_burn_debug_hc_set_burn_state(hc_uart_buffer[11]);

                    packet_len = ftd_mw_hc_manager_build_packet(en_ftd_hc_event, hc_uart_buffer, &ack_status, 1);

                    break;
                }
                /////////////////////////////////////////
                case FTD_PROG_EVENT_GET_SYSINFO:
                {
                    SYS_INFO sys_info_tmp;
                    ftd_mw_sys_info_manager_get(&st_sys_info);
                    memcpy(&sys_info_tmp, &st_sys_info, sizeof(SYS_INFO));
                    ftd_mw_sys_info_manager_reverse_endian_sys_info(&sys_info_tmp);
                    packet_len = ftd_mw_hc_manager_build_packet(en_ftd_hc_event, hc_uart_buffer, (uint8_t*)&sys_info_tmp, sizeof(SYS_INFO));
                    break;
                }
                case FTD_PROG_EVENT_DEPLOY_SEG_DATA_START:
                {
                    ack_status = 0x00; //normal ack
                    ftd_mw_hc_manager_save_seg_pkt_data(hc_uart_buffer);
                    packet_len = ftd_mw_hc_manager_build_packet(en_ftd_hc_event, hc_uart_buffer, &ack_status, 1);
                    break;
                }
                case FTD_PROG_EVENT_FW_UPDATE_SEG_DATA_START:
                {

                }
                case FTD_PROG_EVENT_DEPLOY_SEG_DATA:
                {
                    /* seg packet data/writer deploy file */
                    if ((ftd_mw_hc_manager_get_seg_pkt_data(hc_uart_buffer, &param_len, &seg_pkt_num) == 0) &&
                        (ftd_mw_hc_manager_get_pre_pkt_num() <= seg_pkt_num))
                    {
                        ack_status = 0x00; //normal ack

                        /* parse fw_info */
                        // for debug only
#if DEBUG_USE_UART4
                        if (0x01 == seg_pkt_num)
                        {
                            FW_INFO fw_info_tmp;
                            PROG_LOG_INFO("fw_info_tmp size:%x fw_info_buffer size :%x", sizeof(fw_info_tmp), param_len - 5);

                            memcpy(&fw_info_tmp, &hc_uart_buffer[12], sizeof(FW_INFO));
                            ftd_mw_sys_info_manager_reverse_endian_fw_info(&fw_info_tmp);
                            ftd_mw_sys_info_manager_fw_info_dump(&fw_info_tmp);
                        }
#endif

                        PROG_LOG_INFO("__RECV_PACK__ ttl_pkt_num:%d, cur_pkt_num:%d, curr_pkt_len:%d, total_len:%d recved_len:%d", ftd_mw_hc_manager_get_total_pkt_num(), seg_pkt_num, (param_len - 5),
                            ftd_mw_hc_manager_get_total_pkt_data_len(), ftd_mw_hc_manager_get_received_data_len());

                        //param_len-5 ,extend cmd 2B,data type 1B,cur pkt num 2B; 12 is payload data offset in seg pkt
                        // need check write ret success ?
                        int8_t ret = ftd_mw_cache_manager_write(&hc_uart_buffer[12], ftd_mw_hc_manager_get_total_pkt_data_len(),
                            param_len - 5, ftd_mw_hc_manager_get_received_data_len());

                        if (0 != ret)
                        {
                            PROG_LOG_ERROR("ftd_mw_cache_manager_write failed,ret:%d write len:0x%08x curr_len:0x%08x",
                                ret, ftd_mw_hc_manager_get_received_data_len(), param_len - 5);
                            ack_status = 0x01; //err ack
                        }
#if 0
                        // for debug only
                        if (0x01 == seg_pkt_num || ftd_mw_hc_manager_get_total_pkt_num() == seg_pkt_num)
                        {
                            uint8_t         read_buffer[512] = { 0 };
                            ret = ftd_mw_cache_manager_read(read_buffer, sizeof(read_buffer), sizeof(read_buffer), 0);

                            if (ret == 0)
                            {
                                // debug 
                                PROG_LOG_RAW("READ CACHE SECTOR[%d]:\n", seg_pkt_num);
                                PROG_LOG_DEBUG_BUFF(read_buffer, sizeof(read_buffer), sizeof(read_buffer));
                            }
                        }
#endif
                        //update received flg
                        ftd_mw_hc_manager_set_received_data_len(ftd_mw_hc_manager_get_received_data_len() + (param_len - 5));
                        ftd_mw_hc_manager_set_pre_pkt_num(seg_pkt_num);
                    }
                    else
                    {
                        ftd_mw_hc_manager_set_received_data_len(0);
                        ftd_mw_hc_manager_set_pre_pkt_num(0);
                        ack_status = 0x01;
                    }

                    packet_len = ftd_mw_hc_manager_build_packet(en_ftd_hc_event, hc_uart_buffer, &ack_status, 1);
                    break;
                }
                case FTD_PROG_EVENT_FW_UPDATE_SEG_DATA:
                {
                    PROG_LOG_INFO("FW UPDATE");
                    break;
                }
                case FTD_PROG_EVENT_SET_WRITER_INFO:
                {
                    WRITER_INFO writer_info_tmp;
                    uint16_t data_idx;

                    ack_status = 0x00; //normal ack
                    data_idx = FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + EXTEND_CMD_SIZE + EXTEND_CMD_TYPE_SIZE;

                    memcpy(&writer_info_tmp, &hc_uart_buffer[data_idx], sizeof(WRITER_INFO));
                    ftd_mw_sys_info_manager_reverse_endian_writer_info(&writer_info_tmp);
                    ftd_mw_sys_info_manager_writer_info_dump(&writer_info_tmp);

                    SYS_INFO sys_info_tmp;
                    ftd_mw_sys_info_manager_get(&sys_info_tmp);
                    memcpy(&sys_info_tmp.st_writer_info, &writer_info_tmp, sizeof(WRITER_INFO));
                    ftd_mw_sys_info_manager_set(&sys_info_tmp);
                    int8_t ret = ftd_mw_sys_info_manager_save_nv();

                    PROG_LOG_DEBUG("sys info write ret %d", ret);

                    packet_len = ftd_mw_hc_manager_build_packet(en_ftd_hc_event, hc_uart_buffer, &ack_status, 1);
                    break;
                }
                case FTD_PROG_EVENT_GET_CRC_RESAULT: //M480 will deploy data from cache to fwx,and cal crc16 with fwx
                {
                    ack_status = 0x01;

                    int8_t ret = ftd_mw_cache_manager_get_fw_info(&st_fw_info);
                    PROG_LOG_DEBUG("[PROG_GET_CRC_RET] get_fw_info ret %d", ret);

                    if (ret >= 0)
                    {
                        ret = ftd_mw_fw_manager_deploy(&st_fw_info);
                        if (ret >= 0) //deploy data and check bin/triple crc16
                        {
                            PROG_LOG_INFO("[PROG_GET_CRC_RET] deploy SUCCESS");
                            ack_status = 0x00;
                        }
                        else
                        {
                            PROG_LOG_ERROR("[PROG_GET_CRC_RET] deploy FAIL, ret:%d", ret);
                        }
                    }

                    packet_len = ftd_mw_hc_manager_build_packet(en_ftd_hc_event, hc_uart_buffer, &ack_status, 1);
                    break;
                }
                default:
                    break;
            }

            //send ack
            if ((packet_len > UART_CMD_LEN_MIN) && (packet_len <= PROG_COMMS_BUFF_LEN))
            {
#if (LOG_TYPE >= LOG_LEVEL_DEBUG)
                // print send buffer for debug
                uint16_t parm_len_idx = FRAME_HEADER_SIZE + CMD_SIZE;
                uint16_t parm_len = (hc_uart_buffer[parm_len_idx] << 8 | hc_uart_buffer[parm_len_idx + 1]);
                PROG_LOG_DEBUG("\n___SEND___ hc_uart_buffer[0x%02x] : ", parm_len);
                uint16_t total_len = FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + parm_len + CHECKSUM_SIZE;
                PROG_LOG_DEBUG_BUFF(hc_uart_buffer, sizeof(hc_uart_buffer), total_len);
#endif
                ftd_mw_hc_manager_send_data(hc_uart_buffer, packet_len);
            }
            else
            {
                PROG_LOG_INFO("send buff error! [len:0x%02x]\n", packet_len);
            }
        }
    }

    if (b_update_display)
        ftd_mw_display_manager_sync_channel_burn_status(&st_channel_burn_status_test);
}

void ftd_app_prog_deinit(void)
{
    PROG_LOG_INFO("app_programmer_process\n");
    //host computer uart deinit

    //panel uart deinit

    //flash access deinit
}

