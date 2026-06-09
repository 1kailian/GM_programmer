/****************************************************************************
@FILENAME:  ftd_mw_hc_manager.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.25
@COPYRIGHT: FTD co.ltd
****************************************************************************/
#include "ftd_mw_hc_manager.h"
#include "ftd_data_model.h"
#include "ftd_mw_log_manager.h"

#include <stdio.h>

static uint16_t g_total_num_in_start_seg_pkt = 0;
static uint16_t g_crc16_in_start_seg_pkt = 0;
static uint32_t g_data_len_in_start_seg_pkt = 0;
// static uint16_t g_seg_pkt_cur = 0;
static uint16_t g_seg_pkt_pre = 0;
static uint32_t g_received_data_len = 0;


/**
 * @brief   Calculate XOR checksum for UART packet and store it in the buffer
 *
 * This function calculates the XOR checksum for the packet data and stores
 * the result in the checksum position at the end of the packet.
 * The length is parsed directly from the packet header.
 *
 * @param   p_buf   Pointer to the packet data buffer
 *
 */
void ftd_mw_hc_manager_checksum(uint8_t* p_buf)
{
    uint32_t i;
    uint8_t checksum = 0;
    uint32_t len = 0;
    uint16_t param_len;

    // Check parameters
    if (p_buf == NULL)
    {
        return;
    }

    // Parse parameter length from packet header
    param_len = (p_buf[FRAME_HEADER_SIZE + CMD_SIZE] << 8) | p_buf[FRAME_HEADER_SIZE + CMD_SIZE + 1];

    // Calculate total data length (excluding checksum byte)
    len = FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + param_len;

    // Calculate XOR checksum over the data
    for (i = 0; i < len; i++)
    {
        checksum ^= p_buf[i];
    }

    // Store checksum in the last byte of the packet
    p_buf[len] = checksum;
}

/**
 * @brief   Check and validate whether the received UART data frame is valid
 *
 * This function parses and validates the data in the receive buffer, including frame header,
 * command, parameter length, and checksum verification.
 * The assumed data format is: 4-byte frame header + 1-byte command + 2-byte parameter length +
 * N-byte parameters + 1-byte checksum.
 *
 * @param   p_buf   Pointer to the receive data buffer
 *
 * @return  int8_t  Returns the check result:
 *                  - 1: Valid packet
 *                  - -1: Data length is less than minimum requirement
 *                  - -2: Frame header error
 *                  - -3: Actual length is less than expected length
 *                  - -4: Checksum mismatch
 *                  - -6: Packet length exceeds buffer limit
 */
int8_t ftd_mw_hc_manager_check_data(uint8_t* p_buf)
{
    uint32_t i;
    uint32_t crc_cal = 0;
    uint32_t frame_header;
    uint32_t len = 0;
    uint16_t param_len;
    uint8_t checksum = 0;
    // Get parameter length first to calculate total length
    param_len = (p_buf[FRAME_HEADER_SIZE + CMD_SIZE] << 8) | p_buf[FRAME_HEADER_SIZE + CMD_SIZE + 1];
    FTD_LOG_TRACE("param_len = %d", param_len);
    // Calculate total packet length
    len = FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + param_len + CHECKSUM_SIZE;

    // Check minimum length
    if (len < UART_CMD_LEN_MIN)
    {
        return -1; // Insufficient length
    }

    // Check if packet exceeds buffer limit
    if (len > PROG_COMMS_BUFF_LEN)
    {
        return -6; // Packet length exceeds buffer limit
    }

    // Check frame header
    frame_header = (p_buf[0] << 24) | (p_buf[1] << 16) | (p_buf[2] << 8) | p_buf[3];
    FTD_LOG_TRACE(" frame_header = 0x%x", frame_header);

    if (frame_header != FRAME_HEADER_MAGIC)
    {
        return -2; // Frame header error
    }

    // Calculate checksum
    for (i = 0; i < (FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + param_len); i++)
    {
        crc_cal ^= p_buf[i];
    }

    // Verify checksum
    checksum = p_buf[FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + param_len];

    if (crc_cal != checksum)
    {
        FTD_LOG_ERROR(" crc_cal = 0x%x checksum = 0x%x", crc_cal, checksum);
        return -4; // Checksum error
    }


    return 1; // Packet is valid
}

//p_dest_buff is the raw data received from hs, so we can reuse some of the data when organizing the ack
uint16_t ftd_mw_hc_manager_build_packet(FTD_PROG_EVENT en_hc_event_type, uint8_t* p_dest_buff, uint8_t* p_payload_buff, uint16_t payload_len)
{
    uint16_t packet_len = 0;
    uint16_t param_len = 0;
    uint16_t ext_payload_offset = FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + EXTEND_CMD_SIZE;

    if ((p_dest_buff == NULL) || (p_payload_buff == NULL))
        return 0;

    p_dest_buff[FRAME_HEADER_SIZE] |= 0x80; //ack cmd
    switch (en_hc_event_type)
    {
        case FTD_PROG_EVENT_CHECK_ONLINE:
            // param_len = 1; //no change
            p_dest_buff[FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE] = 0x01; //online
            packet_len = FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + payload_len + CHECKSUM_SIZE;
            break;
        case FTD_PROG_EVENT_DEPLOY_SEG_DATA_START:
        case FTD_PROG_EVENT_FW_UPDATE_SEG_DATA_START:
            ext_payload_offset += 1; //1 is data type size
            param_len = 4; //should be 4
            p_dest_buff[FRAME_HEADER_SIZE + CMD_SIZE] = (param_len >> 8) & 0xFF; //update param length
            p_dest_buff[FRAME_HEADER_SIZE + CMD_SIZE + 1] = param_len & 0xFF;
            memcpy(p_dest_buff + ext_payload_offset, p_payload_buff, payload_len); //set status 1 byte
            packet_len = FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + param_len + CHECKSUM_SIZE;
        case FTD_PROG_EVENT_DEPLOY_SEG_DATA:
        case FTD_PROG_EVENT_FW_UPDATE_SEG_DATA:
            // ext_payload_offset += 1; //1 is data type size(ext cmd 0x54)
            param_len = 7;
            p_dest_buff[FRAME_HEADER_SIZE + CMD_SIZE] = (param_len >> 8) & 0xFF; //update param length
            p_dest_buff[FRAME_HEADER_SIZE + CMD_SIZE + 1] = param_len & 0xFF;
            ext_payload_offset += 3; //skip data type 1 + seg num 2
            memcpy(p_dest_buff + ext_payload_offset, p_payload_buff, payload_len); //set ack status
            packet_len = FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + param_len + CHECKSUM_SIZE;
            break;
        case FTD_PROG_EVENT_GET_SYSINFO:
        case FTD_PROG_EVENT_SET_WRITER_INFO:
        case FTD_PROG_EVENT_BURN_START_TEST_TMP:
        case FTD_PROG_EVENT_BURN_START_TMP:
        case FTD_PROG_EVENT_GET_CRC_RESAULT:
            ext_payload_offset += 2; //2 is data type size(ext cmd 0x45)
            param_len = payload_len + EXTEND_CMD_SIZE + 2; // 2 is data type 0x0002
            p_dest_buff[FRAME_HEADER_SIZE + CMD_SIZE] = (param_len >> 8) & 0xFF; //update param length
            p_dest_buff[FRAME_HEADER_SIZE + CMD_SIZE + 1] = param_len & 0xFF;
            memcpy(p_dest_buff + ext_payload_offset, p_payload_buff, payload_len);
            packet_len = FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + param_len + CHECKSUM_SIZE;
            break;
        default:
            break;
    }

    ftd_mw_hc_manager_checksum(p_dest_buff); //update XOR checksum

    return packet_len;
}

uint8_t ftd_mw_hc_manager_receive_data(uint8_t* p_buf)
{
    static uint8_t init = 0;
    if (init == 0)
    {
        ftd_drv_hc_uart_init();
        JYMC_LOG_INFO("ftd_drv_hc_uart_init\r\n");
        init = 1;
    }

    if (p_buf != NULL)
    {
        return ftd_drv_hc_uart_receive(p_buf);
    }
    else
        return 0;
}

void ftd_mw_hc_manager_send_data(uint8_t* p_buf, uint32_t len)
{
    if (p_buf != NULL)
        ftd_drv_hc_uart_send_sync(p_buf, len);
}


FTD_PROG_EVENT ftd_mw_hc_manager_get_event_type(uint8_t* p_buffer)
{
    FTD_PROG_EVENT en_event_type = FTD_PROG_EVENT_NONE;
    uint8_t ext_data_type_offset = 0;
    uint8_t ext_cmd_offset = 0;

    uint16_t extend_cmd = 0;

    ext_cmd_offset = FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE;
    ext_data_type_offset = ext_cmd_offset + EXTEND_CMD_SIZE;

    // if(ftd_mw_hc_manager_check_data(p_buffer) >= 0)
    {
        // get cmd type,use swtich case process
        switch (p_buffer[FRAME_HEADER_SIZE])
        {
            case 0x0d: //general protocol,check device online 
                en_event_type = FTD_PROG_EVENT_CHECK_ONLINE;
                break;
            case 0x54:
            {
                extend_cmd = (p_buffer[ext_cmd_offset] << 8) | p_buffer[ext_cmd_offset + 1];
                //general protocol,0x000D is seg start cmd,0x000E is seg data
                if ((extend_cmd == 0x000D) && (p_buffer[ext_data_type_offset] == 0x0A))
                    en_event_type = FTD_PROG_EVENT_FW_UPDATE_SEG_DATA_START;
                else if ((extend_cmd == 0x000E) && (p_buffer[ext_data_type_offset] == 0x0A))
                    en_event_type = FTD_PROG_EVENT_FW_UPDATE_SEG_DATA;
                else
                {
                }
            }
            break;
            default:
                break;
        }
    }
    return en_event_type;
}


/**
 * @brief Save segment packet data from the received buffer
 *
 * This function extracts and saves three key pieces of information from the segment start packet:
 * 1. Total number of segments
 * 2. CRC16 checksum value
 * 3. Total data length
 * @param p_buffer Pointer to the received packet buffer
 */
void ftd_mw_hc_manager_save_seg_pkt_data(uint8_t* p_buffer)
{
    uint8_t data_type_offset = FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + EXTEND_CMD_SIZE;

    // Check for valid input parameter
    if (p_buffer == NULL)
        return;

    // Extract total number of segments
    // Position: after data type field (offset + 1 and + 2 bytes)
    g_total_num_in_start_seg_pkt = (p_buffer[data_type_offset + 1] << 8) | p_buffer[data_type_offset + 2];
    FTD_LOG_TRACE("Total number of segments: %d", g_total_num_in_start_seg_pkt);

    // Extract total data length (24-bit)
    // Position: after data type and segment count fields (offset + 3, + 4 and + 5 bytes)
    g_data_len_in_start_seg_pkt = ((uint32_t)p_buffer[data_type_offset + 3] << 16) |
        ((uint32_t)p_buffer[data_type_offset + 4] << 8) |
        (uint32_t)p_buffer[data_type_offset + 5];
    FTD_LOG_TRACE("Total length of data: %d", g_data_len_in_start_seg_pkt);

    // Extract CRC16 checksum value
    // Position: after data type, segment count and data length fields (offset + 6 and + 7 bytes)
    g_crc16_in_start_seg_pkt = (p_buffer[data_type_offset + 6] << 8) | p_buffer[data_type_offset + 7];

    //reset packet receive flg
    // g_seg_pkt_cur = 0;
    g_seg_pkt_pre = 0;
    g_received_data_len = 0;
}

uint32_t ftd_mw_hc_manager_get_total_pkt_data_len(void)
{
    return g_data_len_in_start_seg_pkt;
}

uint16_t ftd_mw_hc_manager_get_pkt_crc16(void)
{
    return g_crc16_in_start_seg_pkt;
}

uint16_t ftd_mw_hc_manager_get_total_pkt_num(void)
{
    return g_total_num_in_start_seg_pkt;
}


void ftd_mw_hc_manager_set_total_pkt_data_len(uint32_t seg_pkt)
{
    g_data_len_in_start_seg_pkt = seg_pkt;
    JYMC_LOG_INFO("Total packet data length set: %d", g_data_len_in_start_seg_pkt);
}

void ftd_mw_hc_manager_set_pkt_crc16(uint16_t crc16)
{
    g_crc16_in_start_seg_pkt = crc16;
    JYMC_LOG_INFO("Packet CRC16 set: %d", g_crc16_in_start_seg_pkt);
}

void ftd_mw_hc_manager_set_total_pkt_num(uint16_t num)
{
    g_total_num_in_start_seg_pkt = num;
    JYMC_LOG_INFO("Total packet number set: %d", g_total_num_in_start_seg_pkt);
}


void ftd_mw_hc_manager_set_received_data_len(uint32_t recv_len)
{
    g_received_data_len = recv_len;
}

void ftd_mw_hc_manager_set_pre_pkt_num(uint16_t num)
{
    g_seg_pkt_pre = num;
}

uint32_t ftd_mw_hc_manager_get_received_data_len(void)
{
    return g_received_data_len;
}
uint16_t ftd_mw_hc_manager_get_pre_pkt_num(void)
{
    return g_seg_pkt_pre;
}


int8_t ftd_mw_hc_manager_get_seg_pkt_data(uint8_t* p_buf, uint16_t* p_cur_param_len, uint16_t* p_cur_seg_num)
{
    uint8_t data_type_offset = 0;
    int16_t extend_cmd = (p_buf[FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE] << 8) | p_buf[FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + 1];

    data_type_offset = FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + EXTEND_CMD_SIZE;
    FTD_LOG_INFO("ext_cmd %04x data_type %02x", extend_cmd, p_buf[data_type_offset]);

    if ((extend_cmd == 0x000E) && (p_buf[data_type_offset] == 0x0A)) //seg packet data/writer deploy file
    {
        *p_cur_param_len = (p_buf[FRAME_HEADER_SIZE + CMD_SIZE] << 8) | p_buf[FRAME_HEADER_SIZE + CMD_SIZE + 1];
        *p_cur_seg_num = (p_buf[data_type_offset + 1] << 8) | p_buf[data_type_offset + 2];
        FTD_LOG_INFO("seg_num %04x", *p_cur_seg_num);
        return 0;
    }
    return -1;
}

// int8_t ftd_mw_hc_manager_seg_pkt_data_process(uint8_t* p_buf)
// {
//     uint8_t data_type_offset = 0;
//     uint16_t param_len = 0;
//     uint16_t ack_param_len = 0;
//     int16_t packet_len = FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + CHECKSUM_SIZE;
//     int16_t extend_cmd = (p_buf[FRAME_HEADER_SIZE + CMD_SIZE] << 8) | p_buf[FRAME_HEADER_SIZE + CMD_SIZE + 1];

//     data_type_offset = FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + EXTEND_CMD_SIZE;
//     if ((extend_cmd == 0x000E) && (p_buf[data_type_offset] == 0x09)) //seg packet data/writer deploy file
//     {
//         param_len = (p_buf[FRAME_HEADER_SIZE + CMD_SIZE] << 8) | p_buf[FRAME_HEADER_SIZE + CMD_SIZE + 1];
//         g_seg_pkt_cur = (p_buf[data_type_offset + 1] << 8) | p_buf[data_type_offset + 2];
//         if (g_seg_pkt_cur == (g_seg_pkt_pre + 1))
//         {
//             g_seg_pkt_pre = g_seg_pkt_cur;

//             //param_len-5 ,extend cmd 2B,data type 1B,cur pkt num 2B
//             ftd_mw_cache_manager_write(&p_buf[data_type_offset + 3], total_data_len, param_len - 5, g_received_data_len);
//             g_received_data_len += param_len;
//         }
//         else
//         {
//             FTD_LOG_INFO(" !!!!seg packet pkt discontinuous, !!! \n"); //No processing is done here; only a final CRC check is performed.
//         }

//         //judge deploy data receive complete
//         if ((seg_pkt_cur == ftd_mw_hc_manager_get_total_pkt_num) && (g_received_data_len == ftd_mw_hc_manager_get_total_pkt_data_len()))
//         {
//             FTD_LOG_INFO(" deploy data receive complete !!!\n");
//             return 0;
//         }
//         else if ((seg_pkt_cur == ftd_mw_hc_manager_get_total_pkt_num) || (g_received_data_len == ftd_mw_hc_manager_get_total_pkt_data_len()))
//         {
//             FTD_LOG_INFO(" !!! deploy data receive error !!!\n");
//             return -1;
//         }
//         else
//         {
//             return -1;
//         }
//     }
//     return -1;
// }
