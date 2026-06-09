/****************************************************************************
@FILENAME  :   ftd_mw_log_manager.c
@BRIEF     :
@AUTHOR    :
@DATE      :   2025/11/15 15:01:24
****************************************************************************/

#include <stdio.h>
#include "ftd_data_model.h"
#include "ftd_mw_log_manager.h"

/**
 * @brief Print buffer data
 *
 * Prints the contents of a buffer to the uart console.
 *
 * @param buf Pointer to buffer
 * @param buff_size Total len of buffer
 * @param len Length will print from buffer base addr, DO NOT bigger than buff_size
 *
 * @return void
 */
void ftd_log_buffer(uint8_t* buf, uint16_t buff_size, uint16_t len)
{
    if (len > buff_size)
    {
        JYMC_LOG_ERROR("print len[0x%04x] over total size[0x%04x]!!!", len, buff_size);
        return;
    }

    for (uint16_t i = 0; i < len; i++)
    {
        JYMC_LOG_RAW("%02X ", *buf++);
    }
    JYMC_LOG_RAW("\n");
}
