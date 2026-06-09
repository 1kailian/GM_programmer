#ifndef __FTD_MW_HC_MANAGER_H__
#define __FTD_MW_HC_MANAGER_H__ 

#include "ftd_prog_event.h"
#include "ftd_drv_hc_uart.h"

#include <stdint.h>


#define UART_CMD_LEN_MIN        (8)
#define FRAME_HEADER_SIZE       (4)
#define CMD_SIZE                (1)
#define PARAM_LEN_SIZE          (2)
#define EXTEND_CMD_SIZE         (2)
#define EXTEND_CMD_TYPE_SIZE    (2)
#define CHECKSUM_SIZE           (1)
#define MIN_PACKET_SIZE         (FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + CHECKSUM_SIZE)
#define FRAME_HEADER_MAGIC 0x4A594D43    // frame header "JYMC"

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
int8_t ftd_mw_hc_manager_check_data(uint8_t* p_buf);
void ftd_mw_hc_manager_checksum(uint8_t* p_buf);

//p_dest_buff is the raw data received from hs, so we can reuse some of the data when organizing the ack
uint16_t ftd_mw_hc_manager_build_packet(FTD_PROG_EVENT en_hc_event_type, uint8_t* p_dest_buff, uint8_t* p_payload_buff, uint16_t payload_len);

// uint8_t ftd_mw_hc_manager_receive_data(uint8_t **p_p_buf);
uint8_t ftd_mw_hc_manager_receive_data(uint8_t* p_buf);
void ftd_mw_hc_manager_send_data(uint8_t* p_buf, uint32_t len);

FTD_PROG_EVENT ftd_mw_hc_manager_get_event_type(uint8_t* p_buffer);
/**
 * @brief Save segment packet data from the received buffer
 *
 * This function extracts and saves three key pieces of information from the segment start packet:
 * 1. Total number of segments
 * 2. CRC16 checksum value
 * 3. Total data length
 * @param p_buffer Pointer to the received packet buffer
 */
void ftd_mw_hc_manager_save_seg_pkt_data(uint8_t* p_buffer);
uint32_t ftd_mw_hc_manager_get_total_pkt_data_len(void);
uint16_t ftd_mw_hc_manager_get_pkt_crc16(void);
uint16_t ftd_mw_hc_manager_get_total_pkt_num(void);

void ftd_mw_hc_manager_set_total_pkt_data_len(uint32_t seg_pkt);
void ftd_mw_hc_manager_set_pkt_crc16(uint16_t crc16);
void ftd_mw_hc_manager_set_total_pkt_num(uint16_t num);

void ftd_mw_hc_manager_set_received_data_len(uint32_t recv_len);
void ftd_mw_hc_manager_set_pre_pkt_num(uint16_t num);
uint32_t ftd_mw_hc_manager_get_received_data_len(void);
uint16_t ftd_mw_hc_manager_get_pre_pkt_num(void);
int8_t ftd_mw_hc_manager_get_seg_pkt_data(uint8_t* p_buf, uint16_t* p_cur_param_len, uint16_t* p_cur_seg_num);
#endif 
