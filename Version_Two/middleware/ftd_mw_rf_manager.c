/****************************************************************************
@FILENAME:  ftd_mw_rf_manager.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.25
@COPYRIGHT: FTD co.ltd
****************************************************************************/
#include "ftd_mw_rf_manager.h"
#include "ftd_data_model.h"
#include "ftd_mw_log_manager.h"
#include "ftd_mw_display_manager.h"

#include <stdio.h>
#include <string.h>



uint8_t ftd_mw_rf_manager_get_crc(uint8_t* buff, uint32_t len)
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

uint8_t ftd_mw_rf_check_rx(uint8_t* rx_buffer, uint16_t len)
{
    uint16_t index = len;
    uint8_t  ret = 0x00;

    // the min length of cmd
    while (index > 1)
    {
        // check header: 0x4A 0x59 0x4D 0x43 
        if ((rx_buffer[0] == 0x4A) && (rx_buffer[1] == 0x59) && (rx_buffer[2] == 0x4D) && (rx_buffer[3] == 0x43))
        {
            // Check if buffer has enough data for the minimum command length
            if (index < 11)
            {
                index--;
                memcpy(&rx_buffer[0], &rx_buffer[1], index);  //drop one byte
                continue;
            }

            ret = 0xFF;

            //rf receive packet
            if ((rx_buffer[4] == 0xD4) && (rx_buffer[5] == 0x00) && (rx_buffer[6] == 0x0A)
                && (rx_buffer[7] == 0x00) && (rx_buffer[8] == 0x88) && (rx_buffer[16] == 0x00)
                )
            {
                uint8_t calc_crc = ftd_mw_rf_manager_get_crc(rx_buffer, CONTROL_BLUETOOTH_ACK_LEN - CHECKSUM_SIZE);

                if (calc_crc == rx_buffer[CONTROL_BLUETOOTH_ACK_LEN - CHECKSUM_SIZE])
                {
                    FTD_LOG_TRACE("RECEIVE_PACKET_ACK OK");
                    ret = FTD_RF_EVENT_SEND_PACKET;
                }
                else
                {
                    FTD_LOG_ERROR("calc_crc = %02x, buff_crc = %02x", calc_crc, rx_buffer[CONTROL_BLUETOOTH_ACK_LEN - CHECKSUM_SIZE]);
                    FTD_LOG_DEBUG_BUFF(rx_buffer, CONTROL_BLUETOOTH_ACK_LEN, CONTROL_BLUETOOTH_ACK_LEN);
                }
            }
            //rf send packet
            else if ((rx_buffer[4] == 0xD4) && (rx_buffer[5] == 0x00) && (rx_buffer[6] == 0x0A)
                && (rx_buffer[7] == 0x00) && (rx_buffer[8] == 0x88) && (rx_buffer[16] == 0x01)
                )
            {
                uint8_t calc_crc = ftd_mw_rf_manager_get_crc(rx_buffer, CONTROL_BLUETOOTH_ACK_LEN - CHECKSUM_SIZE);

                if (calc_crc == rx_buffer[CONTROL_BLUETOOTH_ACK_LEN - CHECKSUM_SIZE])
                {
                    FTD_LOG_TRACE("SEND_PACKET_ACK OK");
                    ret = FTD_RF_EVENT_RECEIVE_PACKET;
                }
                else
                {
                    FTD_LOG_ERROR("calc_crc = %02x, buff_crc = %02x", calc_crc, rx_buffer[CONTROL_BLUETOOTH_ACK_LEN - CHECKSUM_SIZE]);
                    FTD_LOG_DEBUG_BUFF(rx_buffer, CONTROL_BLUETOOTH_ACK_LEN, CONTROL_BLUETOOTH_ACK_LEN);
                }
            }
            // rf receive RSSI value
            else if ((rx_buffer[4] == 0xD4) && (rx_buffer[5] == 0x00) && (rx_buffer[6] == 0x08)
                && (rx_buffer[7] == 0x00) && (rx_buffer[8] == 0x8C)
                )
            {
                uint8_t calc_crc = ftd_mw_rf_manager_get_crc(rx_buffer, BLUETOOTH_RECV_RSSI_ACK_LEN - CHECKSUM_SIZE);

                if (calc_crc == rx_buffer[BLUETOOTH_RECV_RSSI_ACK_LEN - CHECKSUM_SIZE])
                {
                    FTD_LOG_DEBUG("RECV_RSSI_ACK OK");
                    ret = FTD_RF_EVENT_RECEIVE_RSSI;
                }
                else
                {
                    FTD_LOG_ERROR("calc_crc = %02x, buff_crc = %02x", calc_crc, rx_buffer[BLUETOOTH_RECV_RSSI_ACK_LEN - CHECKSUM_SIZE]);
                    FTD_LOG_DEBUG_BUFF(rx_buffer, BLUETOOTH_RECV_RSSI_ACK_LEN, BLUETOOTH_RECV_RSSI_ACK_LEN);
                }
            }
           
            // todo: add abnormal process
            if (0xFF == ret)
            {
                index -= 4;
                memcpy(&rx_buffer[0], &rx_buffer[4], index);
                ret = 0;
            }
        }
        else
        {
            index--;
            //FTD_LOG_TRACE("drop 0x%x", rx_buffer[0]);
            memcpy(&rx_buffer[0], &rx_buffer[1], index);  //drop one byte
        }

        if (ret)
        {
            if (0xFF == ret)
            {
                FTD_LOG_DEBUG("____UNROCOGNIZED_CMD____[%d]", index);
                FTD_LOG_DEBUG_BUFF(rx_buffer, len, index);
            }
            break;
        }
    }

    return ret;
}
