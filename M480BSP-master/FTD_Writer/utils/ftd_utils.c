/****************************************************************************
@FILENAME:  ftd_utils_crc.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.17
@COPYRIGHT: FTD co.ltd
****************************************************************************/
#include "ftd_utils.h"
#include "NuMicro.h"
#include "ftd_mw_log_manager.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Define functions                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
#if 1
void ftd_utils_hw_crc16_open(void)
{
    SYS_UnlockReg();
    CLK_EnableModuleClock(CRC_MODULE);
    SYS_LockReg();
    /* Configure CRC controller for CRC-8 CPU mode ,no reverse bit...*/
    CRC_Open(CRC_16, 0, 0xFFFF, CRC_CPU_WDATA_8);
}
void ftd_utils_hw_crc16_update(uint8_t *data, uint32_t length)
{
    uint32_t i;

    /* Start to execute CRC-8 CPU operation */
    for(i = 0; i < length; i++)
    {
        CRC_WRITE_DATA(data[i]);
    }
}
uint16_t ftd_utils_hw_crc16_get_result(void)
{
    uint32_t u32CalChecksum;
    /* Get CRC checksum value */
    u32CalChecksum = CRC_GetChecksum();
    /* Disable CRC function */
    CLK_DisableModuleClock(CRC_MODULE);
    return (uint16_t)u32CalChecksum;
;
}

uint16_t ftd_utils_hw_crc16(uint8_t *data, uint32_t length)
{
    ftd_utils_hw_crc16_open();
    ftd_utils_hw_crc16_update(data, length);
    return ftd_utils_hw_crc16_get_result();
}

static const uint16_t crc16_ccitt_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

/**
 * @brief Updates the CRC-16 CCITT checksum.
 *
 * This function calculates the CRC-16 CCITT checksum by processing data bytes
 * and updating the current CRC value. It uses the standard CRC-16 CCITT polynomial (0x1021).
 *
 * @param crc Initial CRC value (use 0xFFFF for new calculation)
 * @param data The data buffer to be processed
 * @param length Number of bytes to process
 * @return uint16_t The updated CRC value after processing the data
 */
uint16_t crc16_ccitt_update(uint16_t crc, uint8_t *data, uint32_t length)
{
    uint16_t current_crc = crc;
    uint8_t *end;

    if (data == ((void *)0) || length == 0) {
        return crc;
    }

    end = data + length;
    
    while (data < end) {
        current_crc = (current_crc << 8) ^ crc16_ccitt_table[((current_crc >> 8) ^ *data++) & 0xFF];
    }
    
    return current_crc;
}
#endif

/**
 * @brief Reverse u16 endian
 *
 * This function reverse each byte of u16 data
 *
 * @param value Input u16 data
 * @return uint16_t The u16 data with reversed endian
 */
uint16_t endian_reverse_u16(uint16_t value)
{
    return ((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8);
}

/**
 * @brief Reverse u32 endian
 *
 * This function reverse each byte of u32 data
 *
 * @param value Input u32 data
 * @return uint32_t The u32 data with reversed endian
 */
uint32_t endian_reverse_u32(uint32_t value)
{
    return ((value & 0x000000FF) << 24) |
        ((value & 0x0000FF00) << 8) |
        ((value & 0x00FF0000) >> 8) |
        ((value & 0xFF000000) >> 24);
}

/**
 * @brief Reverse buff endian
 *
 * This function reverse each byte of buff
 *
 * @param *data Input buffer pointer
 * @param len Length of buffer
 * @return void
 */
void endian_reverse_buff(uint8_t *data, uint8_t len)
{
    for(uint8_t i = 0; i < len / 2; i++)
    {
        uint8_t temp = data[i];
        data[i] = data[len - i - 1];
        data[len - i - 1] = temp;
    }
}

/**
 * @brief get u8 buffer crc value
 *
 * This function calculate u8 buffer crc
 *
 * @param *data Input buffer pointer
 * @param len Length of buffer
 * @return u8 crc
 */
static uint8_t ftd_utils_get_u8_crc(uint8_t* buff, uint32_t len)
{
    uint8_t checksum = 0;
    uint32_t i;

    // Calculate XOR checksum over the data
    for (i = 0; i < len; i++)
    {
        checksum ^= buff[i];
    }

    return checksum;
}

/**
 * @brief ftd uart protocol handler
 *
 * This function decode uart cmd 
 *
 * @param opcode Cmd raw opcode
 * @param *p_buff Input buffer pointer
 * @param len Length of buffer
 * @param data_len Cmd param length
 * @return uint16_t Cmd opcode
 */
static uint16_t ftd_uart_protocol_handler(uint8_t opcode, uint8_t* p_buff, uint16_t data_len)
{
    uint16_t ret = 0;

    FTD_LOG_TRACE_BUFF(p_buff, data_len, data_len);

    if (UART_EXT_CMD == opcode)
    {
        uint16_t ext_cmd_type = (p_buff[0] << 8) | p_buff[1];

        if (EXT_CMD_PANNEL2WRITER == ext_cmd_type)
        {
            uint16_t sub_type = (p_buff[2] << 8) | p_buff[3];

            switch (sub_type)
            {
                case PANNEL2WRITER_OP_GET_SYSINFO:
                {
                    ret = PNL2WTR_OP_GET_INFO;
                    break;
                }
                case PANNEL2WRITER_OP_FW_SELECT:
                {
                    uint16_t fw_id = p_buff[4];

                    if (fw_id >= 1 && fw_id <= 3)
                    {
                        ret = fw_id + PNL2WTR_OP_BURN_FW1 - 1;

                        if (ret < PNL2WTR_OP_BURN_FW1 || ret > PNL2WTR_OP_BURN_FW3)
                        {
                            ret = 0;
                        }
                    }

                    break;
                }
                case PANNEL2WRITER_OP_STOP_BURN:
                {
                    ret = PNL2WTR_OP_BURN_STOP;
                    break;
                }
                case PANNEL2WRITER_OP_MODE_SELECT:
                {
                    uint16_t burn_mode = p_buff[4];

                    if (burn_mode >= 1 && burn_mode <= 2) 
                    {
                        ret = burn_mode == 1 ? PNL2WTR_OP_BURN_MODE_MANUAL : PNL2WTR_OP_BURN_MODE_AUTO;
                    }

                    break;
                }
                case PANNEL2WRITER_OP_START_BURN:
                {
                    ret = PNL2WTR_OP_BURN_START;
                    break;
                }
                case PANNEL2WRITER_OP_RETURN:
                {
                    switch (p_buff[4])
                    {
                        case 0x05:
                        {
                            ret = PNL2WTR_OP_PAGE_5_BURN;
                            break;
                        }
                        case 0x07:
                        {
                            ret = PNL2WTR_OP_PAGE_7_MODE;
                            break;
                        }

                    }
                    break;
                }
                case PANNEL2WRITER_OP_SCREEN_VER:
                {
                    ret = PNL2WTR_OP_SCREEN_VER;
                }
                default:
                    break;
            }
        }
    }

    FTD_LOG_TRACE("ret %x", ret);
    return ret;
}

/**
 * @brief ftd uart protocol decode
 *
 * This function check and decode uart cmd 
 *
 * @param *data Input buffer pointer
 * @param len Length of buffer
 * @param data_len Min length of cmd will be decode
 * @return uint16_t Cmd opcode
 */
uint16_t ftd_uart_protocol_decode(uint8_t* p_process_buff, uint16_t len, uint16_t data_len)
{
    uint16_t  index = len;
    uint16_t  ret = 0x00;
    uint8_t   opcode;
    uint16_t  parm_len;

    //head(4) + cmd(1) + data_len(2)
    while ((index) >= data_len)
    {
        if ((p_process_buff[0] == 0x4A) && (p_process_buff[1] == 0x59) && (p_process_buff[2] == 0x4D) && (p_process_buff[3] == 0x43))
        {
            opcode = p_process_buff[4];
            parm_len = (p_process_buff[5] << 8) | p_process_buff[6];
            // prevent array overflow
            if ((parm_len + 8) > index)
            {
                index -= 4;
                memcpy(&p_process_buff[0], &p_process_buff[3], index);
                continue;
            }

            // check crc
            uint8_t crc = ftd_utils_get_u8_crc(p_process_buff, parm_len + 7); //  7 = head(4) + cmd(1) + data_len(2)
            if (crc != p_process_buff[parm_len + 7])
            {
                index -= parm_len + 8;
                memcpy(&p_process_buff[0], &p_process_buff[parm_len + 8], index);
                FTD_LOG_ERROR("crc err calc:0x%02x recv:0x%02x index:%d", crc, p_process_buff[parm_len + 7], index);
                continue;
            }

            // call cmd hander
            ret = ftd_uart_protocol_handler(opcode, &p_process_buff[7], parm_len);
            if (0 == ret)
            {
                index -= parm_len + 8;
                memcpy(&p_process_buff[0], &p_process_buff[parm_len + 8], index);
                continue;
            }
        }
        else
        {
            index--;
            memcpy(&p_process_buff[0], &p_process_buff[1], index);
        }

        if (ret != 0)
        {
            index = 0;
            break;
        }
    }

    return ret;
}
