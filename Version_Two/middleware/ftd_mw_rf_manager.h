#ifndef __FTD_MW_RF_MANAGER_H__
#define __FTD_MW_RF_MANAGER_H__

#include "ftd_prog_event.h"
#include "ftd_drv_rf_uart.h"

#include <stdint.h>



#define FRAME_HEADER_SIZE       (4)
#define CMD_SIZE                (1)
#define PARAM_LEN_SIZE          (2)
#define EXTEND_CMD_SIZE         (2)
#define EXTEND_CMD_TYPE_SIZE    (2)
#define CHECKSUM_SIZE           (1)
#define MIN_PACKET_SIZE         (FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + CHECKSUM_SIZE)
#define FRAME_HEADER_MAGIC 0x4A594D43    // frame header "JYMC"

#define BLUETOOTH_ADDR_LEN      (4)
#define MSGID_LEN               (1)
#define TX_PWOER_LEN            (1)
#define CRRL_TYPE_LEN           (1)
#define REAN_PACKET_NUM          (1)
#define RSSI_VALUE_LEN          (1)

#define CONTROL_BLUETOOTH_ACK_LEN (FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + EXTEND_CMD_SIZE + BLUETOOTH_ADDR_LEN + MSGID_LEN + TX_PWOER_LEN + CRRL_TYPE_LEN + REAN_PACKET_NUM + CHECKSUM_SIZE)
#define BLUETOOTH_RECV_RSSI_ACK_LEN (FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + EXTEND_CMD_SIZE + BLUETOOTH_ADDR_LEN + MSGID_LEN + RSSI_VALUE_LEN + CHECKSUM_SIZE)

#define RF_RSSI_SAMPLE_COUNT    (20)
#define RF_SEND_PACKET_MAX_NUM (300)
typedef enum
{
    FTD_RF_EVENT_SEND_PACKET = 0x01,
    FTD_RF_EVENT_RECEIVE_PACKET,
    FTD_RF_EVENT_RECEIVE_RSSI,
}FTD_RF_EVENT;

typedef enum
{
    FTD_CMD_SEND_PACKET      = 0x00,
    FTD_CMD_RECEIVE_PACKET   = 0x01,
}FTD_RF_CMD;



typedef enum
{
    TX_POWER_0DB = 0x00,
    TX_POWER_3DB = 0x03,
    TX_POWER_5DB = 0x05,
    TX_POWER_6DB = 0x06,
    TX_POWER_7DB = 0x07,
    TX_POWER_10DB = 0x0a,

    TX_POWER_F3DB = 0x83,
    TX_POWER_F5DB = 0x85,
    TX_POWER_F10DB = 0x8a,
    TX_POWER_F20DB = 0x94,
    TX_POWER_F30DB = 0x9e,
    TX_POWER_FACTORY = 0xaa,
}BLUETOOTH_TX_POWER;

uint8_t ftd_mw_rf_check_rx(uint8_t* rx_buffer, uint16_t len);




#endif
