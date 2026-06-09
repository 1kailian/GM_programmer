/****************************************************************************
@FILENAME:  ftd_app_prog.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.15
@COPYRIGHT: FTD co.ltd
****************************************************************************/
#include "ftd_app_prog.h"
#include "ftd_prog_event.h"
#include "ftd_mw_hc_manager.h"
#include "ftd_mw_log_manager.h"
#include "ftd_mw_iap_manager.h"
#include "ftd_mw_misc_manager.h"
#include "ftd_utils.h"
#include "ftd_data_model.h"
#include "stdint.h"
#include "NuMicro.h"

#define PROG_LOG_WARN   FTD_LOG_WARN
#define PROG_LOG_INFO   FTD_LOG_INFO
#define PROG_LOG_DBUG   FTD_LOG_DBUG

uint8_t hc_uart_buffer[PROG_COMMS_BUFF_LEN];
extern uint32_t ftd_drv_timer_get_time_ms(void);
extern uint32_t time_cnt;

void ftd_app_prog_init(void)
{
    //host computer uart init
    PROG_LOG_INFO("ftd_app_prog_init \n");
}

void ftd_app_prog_process(void)
{
    uint16_t param_len = 0;
    uint16_t seg_pkt_num = 0;
    int16_t packet_len = 0;
    uint8_t ack_status = 0;
    FTD_PROG_EVENT en_ftd_hc_event = FTD_PROG_EVENT_NONE;

    uint8_t iap_finish_flag = 0;
    uint8_t iap_success_flag = 0;
    static int init = 0;

    //0.send iap mode ack
    if (init == 0)
    {
        init = 1;
        ftd_drv_hc_uart_init();
        uint8_t iap_mode_ack[] = { 0x4A, 0x59, 0x4D, 0x43, 0xD4, 0x00, 0x02,0x00, 0x31, 0xFF };
        ftd_mw_hc_manager_checksum(iap_mode_ack);
        JYMC_LOG_INFO("iap_mode_ack\n");
        for (int i = 0; i < sizeof(iap_mode_ack); i++)
        {
            JYMC_LOG_INFO("[%d]: %02x ", i, iap_mode_ack[i]);
        }
        ftd_mw_hc_manager_send_data(iap_mode_ack, sizeof(iap_mode_ack));
        JYMC_LOG_INFO("\niap_mode_ack finish\n");
    }
    //1.receive host computer data
    if (ftd_mw_hc_manager_receive_data(hc_uart_buffer) == 1)
    {
        ftd_mw_iap_manager_set_time_ms(ftd_mw_misc_manager_get_time_ms());

#if (LOG_TYPE >= LOG_LEVEL_DEBUG)
        // print received buffer for debug
        uint16_t parm_len_idx = FRAME_HEADER_SIZE + CMD_SIZE;
        uint16_t parm_len = (hc_uart_buffer[parm_len_idx] << 8 | hc_uart_buffer[parm_len_idx + 1]);
        uint16_t total_len = FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + parm_len + CHECKSUM_SIZE;
        FTD_LOG_DEBUG("\n___RECV___ hc_uart_buffer[0x%02x] : ", parm_len);

        FTD_LOG_DEBUG_BUFF(hc_uart_buffer, sizeof(hc_uart_buffer), total_len);
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
                case FTD_PROG_EVENT_FW_UPDATE_SEG_DATA_START:
                    ack_status = 0x00; //normal ack
                    ftd_mw_iap_manager_fmc_data_init();
                    ftd_mw_hc_manager_save_seg_pkt_data(hc_uart_buffer);
                    packet_len = ftd_mw_hc_manager_build_packet(en_ftd_hc_event, hc_uart_buffer, &ack_status, 1);
                    break;

                case FTD_PROG_EVENT_FW_UPDATE_SEG_DATA:
                {
                    PROG_LOG_INFO("FW UPDATE");
                    /* seg packet data/writer deploy file */
                    if ((ftd_mw_hc_manager_get_seg_pkt_data(hc_uart_buffer, &param_len, &seg_pkt_num) == 0) &&
                        (ftd_mw_hc_manager_get_pre_pkt_num() <= seg_pkt_num))
                    {


                        ack_status = 0x00; //normal ack
                        FTD_LOG_INFO("__RECV_PACK__ ttl_pkt_num:%d, cur_pkt_num:%d, curr_pkt_len:%d, total_len:%d recved_len:%d", ftd_mw_hc_manager_get_total_pkt_num(), seg_pkt_num, (param_len - 5),
                            ftd_mw_hc_manager_get_total_pkt_data_len(), ftd_mw_hc_manager_get_received_data_len());

                        //param_len-5 ,extend cmd 2B,data type 1B,cur pkt num 2B; 12 is payload data offset in seg pkt
                        // need check write ret success ?
                        // int8_t ret = ftd_mw_cache_manager_write(&hc_uart_buffer[12], ftd_mw_hc_manager_get_total_pkt_data_len(),
                        //     param_len - 5, ftd_mw_hc_manager_get_received_data_len());
                        ftd_mw_iap_manager_set_upgrade_flag(1);
                        int8_t ret = ftd_mw_iap_manager_write(ftd_mw_hc_manager_get_total_pkt_data_len(), &hc_uart_buffer[12], param_len - 5);

                        if (0 != ret)
                        {
                            FTD_LOG_ERROR("ftd_mw_cache_manager_write failed,ret:%d write len:0x%08x curr_len:0x%08x",
                                ret, ftd_mw_hc_manager_get_received_data_len(), param_len - 5);
                            ack_status = 0x01; //err ack
                        }
#if 0
                        // for debug only
                        if (0x01 == seg_pkt_num || ftd_mw_hc_manager_get_total_pkt_num() == seg_pkt_num)
                        {
                            uint8_t         read_buffer[4096] = { 0 };
                            ret = ftd_mw_cache_manager_read(read_buffer, sizeof(read_buffer), sizeof(read_buffer), 0);

                            if (ret == 0)
                            {
                                // debug 
                                FTD_LOG_RAW("READ CACHE SECTOR[%d]:\n", seg_pkt_num);
                                for (uint16_t i = 0; i < sizeof(read_buffer); i++)
                                {
                                    FTD_LOG_RAW(" %02x", read_buffer[i]);
                                }
                                FTD_LOG_RAW("\n");
                            }
                        }
#endif
                        //update received flg
                        ftd_mw_hc_manager_set_received_data_len(ftd_mw_hc_manager_get_received_data_len() + (param_len - 5));
                        ftd_mw_hc_manager_set_pre_pkt_num(seg_pkt_num);
                        if (ftd_mw_hc_manager_get_received_data_len() == ftd_mw_hc_manager_get_total_pkt_data_len())
                        {
                            FTD_LOG_INFO("FW UPDATE FINISH");
                            ftd_mw_iap_manager_finish(); // finish write
                            iap_finish_flag = 1;

                            if (ftd_mw_iap_manager_get_crc16() == ftd_mw_hc_manager_get_pkt_crc16()) //check crc
                            {
                                FTD_LOG_INFO("FW UPDATE CRC OK: 0x%04x, 0x%04x", ftd_mw_iap_manager_get_crc16(), ftd_mw_hc_manager_get_pkt_crc16());
                                ack_status = 0x00;
                                iap_success_flag = 1;
                            }
                            else
                            {
                                FTD_LOG_INFO("FW UPDATE CRC ERR: 0x%04x, 0x%04x", ftd_mw_iap_manager_get_crc16(), ftd_mw_hc_manager_get_pkt_crc16());
                                ack_status = 0x01;
                                iap_success_flag = 0;
                                ftd_mw_iap_manager_erase_app_2();
                            }
                        }
                    }
                    else
                    {
                        FTD_LOG_INFO("seg pkt num error");
                        ftd_mw_hc_manager_set_received_data_len(0);
                        ftd_mw_hc_manager_set_pre_pkt_num(0);
                        ack_status = 0x01;
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
                FTD_LOG_DEBUG("\n___SEND___ hc_uart_buffer[0x%02x] : ", parm_len);
                uint16_t total_len = FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + parm_len + CHECKSUM_SIZE;
                FTD_LOG_DEBUG_BUFF(hc_uart_buffer, sizeof(hc_uart_buffer), total_len);
#endif
                ftd_mw_hc_manager_send_data(hc_uart_buffer, packet_len);
            }
            else
            {
                PROG_LOG_INFO("send buff error! [len:0x%02x]\n", packet_len);
            }

            if (iap_finish_flag)
            {
                if (iap_success_flag)
                {
                    IAP_FLAG_T flag;
                    flag.start_magic = IAP_MAGIC;
                    flag.upgrade = 0x00;
                    flag.app_select = 2;
                    ftd_mw_iap_manager_flag_save(flag);
                    IAP_FLAG_T flag_load;
                    ftd_mw_iap_manager_flag_load(&flag_load);

                    // ftd_mw_iap_manager_deinit();
                    JYMC_LOG_INFO("flag_load.start_magic:0x%08x\r\n", flag_load.start_magic);
                    JYMC_LOG_INFO("flag_load.upgrade:0x%08x\r\n", flag_load.upgrade);

                    if (memcmp(&flag_load, &flag, sizeof(IAP_FLAG_T)) == 0)
                    {
                        PROG_LOG_INFO("memcpy success\r\n");
                        if (ftd_mw_iap_manager_check_addr_valid(APP_2_START_BASE))
                            ftd_mw_iap_manager_jum_to_app(APP_2_START_BASE);
                    }

                }
                else {
                    IAP_FLAG_T flag;
                    flag.start_magic = IAP_MAGIC;
                    flag.upgrade = 0x00;
                    flag.app_select = 1;
                    ftd_mw_iap_manager_flag_save(flag);

                    ftd_mw_iap_manager_jum_to_app(APP_1_START_BASE);
                }
            }
        }
    }

}

void ftd_app_prog_deinit(void)
{
    PROG_LOG_INFO("app_programmer_process\n");
    //host computer uart deinit

    //panel uart deinit

    //flash access deinit
}

