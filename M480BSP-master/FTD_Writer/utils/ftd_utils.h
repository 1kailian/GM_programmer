#ifndef __FTD_UTILS_CRC_H__
#define __FTD_UTILS_CRC_H__ 

#include <stdint.h>

#define FTD_UART_PROTOCOL_HEAD                      (0x4a594d43)
#define UART_EXT_CMD                                (0x54)

typedef enum
{
    EXT_CMD_PANNEL2WRITER = 0x0045,

    EXT_CMD_MAX,
} uart_ext_cmd_type;

typedef enum
{
    PANNEL2WRITER_OP_GET_SYSINFO = 0x0002,
    PANNEL2WRITER_OP_FW_SELECT   = 0x0003,
    PANNEL2WRITER_OP_STOP_BURN   = 0x0004,
    PANNEL2WRITER_OP_MODE_SELECT = 0x0005,
    PANNEL2WRITER_OP_START_BURN  = 0x0006,
    PANNEL2WRITER_OP_RETURN      = 0x0007,
    PANNEL2WRITER_OP_SCREEN_VER  = 0x0008,

    PANNEL2WRITER_OP_TYPE_MAX,
} PANNEL2WRITER_OP_TYPE;

typedef enum
{
    PNL2WTR_OP_GET_INFO = 0x0001,
    PNL2WTR_OP_BURN_FW1,
    PNL2WTR_OP_BURN_FW2,
    PNL2WTR_OP_BURN_FW3,
    PNL2WTR_OP_BURN_STOP,           // 5
    PNL2WTR_OP_BURN_MODE_MANUAL,
    PNL2WTR_OP_BURN_MODE_AUTO,
    PNL2WTR_OP_BURN_START,
    PNL2WTR_OP_PAGE_5_BURN,         // 9
    PNL2WTR_OP_PAGE_7_MODE,
    PNL2WTR_OP_SCREEN_VER,          // 11

    UART_OPCODE_MAX,
} PNL2WTR_OP_TYPE;

#if 1
void     ftd_utils_hw_crc16_open(void);
void     ftd_utils_hw_crc16_update(uint8_t *data, uint32_t length);
uint16_t ftd_utils_hw_crc16_get_result(void);
uint16_t ftd_utils_hw_crc16(uint8_t *data, uint32_t length);

/**
 * @brief Updates the CRC-16 CCITT checksum.
 * @param crc First crc value must be 0xFFFF,The current CRC value before processing the new data byte.
 * @param data The data byte to be processed.
 * @return uint16_t The updated CRC value after processing the data byte.
 */
uint16_t crc16_ccitt_update(uint16_t crc, uint8_t *data, uint32_t length);
#endif

uint16_t endian_reverse_u16(uint16_t value);
uint32_t endian_reverse_u32(uint32_t value);
void     endian_reverse_buff(uint8_t *data, uint8_t len);

uint16_t ftd_uart_protocol_decode(uint8_t* p_process_buff, uint16_t len, uint16_t data_len);

#endif 
