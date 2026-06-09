#ifndef __FTD_MW_DISPLAY_MANAGER_H__
#define __FTD_MW_DISPLAY_MANAGER_H__ 

#include <stdint.h>
#include <stdbool.h>
#include "ftd_data_model.h"

/* No ACK mechanism in panel communication
* The UART communication structure follows the JYMC UART protocol, which includes a fixed header "JYMC", 
* followed by the command (cmd), then a 2-byte length field, then parameters, and finally an XOR checksum. 
* The differentiation lies in the command (cmd) portion, where the cmd is populated according to the XXX_CMD definitions here.
*/
typedef enum
{
    PANEL_TO_MCU_CMD_NONE = 0,
    PANEL_TO_MCU_CMD_APP_MODE, //0-host programmer  1-burn mcu  2-writer-ota

    PANEL_TO_MCU_CMD_BURNNING_MODE, //channel x, 0-manaul 1-auto
    PANEL_TO_MCU_CMD_BURNNING_START,
    PANEL_TO_MCU_CMD_BURNNING_DONE,
    PANEL_TO_MCU_CMD_BURNNING_MANUAL_TRIGGER,
    PANEL_TO_MCU_CMD_BURNNING_FW_SELECT, //param fw_x

} PANEL_TO_MCU_CMD;

typedef enum
{
    MCU_TO_PANEL_CMD_NONE = 0,
    MCU_TO_PANEL_CMD_SYS_INFO, //refer to data_model sys_info
    MCU_TO_PANEL_CMD_CHANNEL_STATUS, //channel num + on/off line + fw_x + current sucess/fail +burn total count+success count
    MCU_TO_PANEL_CMD_KEY_PRESS, //key event no param
    MCU_TO_PANEL_CMD_PAGE_CHANGE, 
} MCU_TO_PANEL_CMD;


typedef enum
{
    SYS_INFO_HW_VER = 0,
    SYS_INFO_SW_VER,
    SYS_INFO_SN,

    SYS_INFO_FW1_CHIP_ID,                   // 3
    SYS_INFO_FW1_VER,
    SYS_INFO_FW1_BURNED_CNT,
    SYS_INFO_FW1_TRIPLE_REMAIN_CNT,
    SYS_INFO_FW1_ROLLCODE_REMAIN_CNT,

    SYS_INFO_FW2_CHIP_ID,                   // 8
    SYS_INFO_FW2_VER,
    SYS_INFO_FW2_BURNED_CNT,
    SYS_INFO_FW2_TRIPLE_REMAIN_CNT,
    SYS_INFO_FW2_ROLLCODE_REMAIN_CNT,

    SYS_INFO_FW3_CHIP_ID,                   // 13
    SYS_INFO_FW3_VER,
    SYS_INFO_FW3_BURNED_CNT,
    SYS_INFO_FW3_TRIPLE_REMAIN_CNT,
    SYS_INFO_FW3_ROLLCODE_REMAIN_CNT,

    SYS_INFO_MAX,
} SYS_INFO_INDEX;


typedef enum
{
    BURN_PROCESS_BAR,
    BURN_PROCESS_STATUS,

    BURN_CNT,
    BURN_REMAIN_CNT,
    BURN_BUTTON_STATUS,

    BURN_CHIP_TYPE,
    BURN_FW_VER,

    BURN_STATUS_MAX,
} BURN_STATUS_INDEX;


// typedef enum
// {
//     HMI_HOME                = 0x00,
//     HMI_DEPLOY_GUIDE,

//     HMI_FW_SELECT_1,        // 2
//     HMI_FW_SELECT_2,
//     HMI_FW_SELECT_3,

//     HMI_FW_BURN_STATUS,     // 5

//     HMI_DEPLOY_ONGOING,     // 6

//     HMI_BURN_MODE_SELECT,   // 7

//     HMI_BURN_CHNANEL_SELECT, // 8

//     HMI_MAX,
// } HMI_PAGE_ID;

typedef enum
{
    HMI_HOME = 0x00,
    HMI_DEPLOY_GUIDE,

    HMI_FW_SELECT_1,        // 2
    HMI_FW_SELECT_2,
    HMI_FW_SELECT_3,

    HMI_FW_BURN_STATUS,     // 5

    HMI_BURN_MODE_SELECT,   // 6

    HMI_BURN_CHNANEL_SELECT, // 7

    HMI_MAX,
} HMI_PAGE_ID;


typedef struct
{
    uint8_t select_fw_id;
    uint8_t last_page_id;
    uint8_t curr_page_id;
    uint8_t reserved[1];
} DISPLAY_INFO;


/**
 * Build UART packet according to JYMC protocol,No ACK mechanism in panel communication
 * 
 * JYMC UART Protocol Frame Structure:
 * +------------+--------+----------------+-----------------+----------+
 * | Fixed Header| Command| 2-byte Length  |    Parameters   | XOR Check|
 * |  "JYMC"    |(XXX_CMD)|    Field       |                 |   -sum   |
 * |  (4 bytes) |(1 byte) |   (2 bytes)    |   (N bytes)     | (1 byte) |
 * +------------+--------+----------------+-----------------+----------+
 * 
 * @param cmd Command to be sent
 * @param p_param Pointer to parameter data
 * @param param_len Length of parameter data in bytes
 * @return Total length of the packet including header, command, length field, parameters and checksum
 */
uint16_t ftd_mw_display_manager_build_packet(MCU_TO_PANEL_CMD cmd, uint8_t *p_param, uint16_t param_len);

/**
 * Unpack UART packet according to JYMC protocol
 * 
 * @param buffer Input packet data to be unpacked
 * @param p_cmd Pointer to store the extracted command
 * @param p_param Pointer to store the extracted parameter data
 * @return true: packet is valid, false: packet is invalid
 */
bool ftd_mw_display_manager_unpacket(uint8_t *buffer, PANEL_TO_MCU_CMD *p_cmd, uint8_t *p_param);

void ftd_mw_display_manager_sync_channel_burn_status(CHANNEL_BURN_STATUS* p_st_channel_burn_status);
void ftd_mw_display_manager_set_button_state(BRUN_STATUS_E state);
void ftd_mw_display_manager_key_press(void);

void ftd_mw_display_manager_init(SYS_INFO *p_st_sys_info);

/**
 * Set the event callback function for display manager
 * 
 * @param fp_callback Pointer to the callback function
 * 
 * @note The callback function will be called when display events occur
 * @note Callback function signature: void callback_func(PANEL_TO_MCU_CMD event, uint8_t param)
 */
void ftd_mw_display_manager_set_event_call_back(void (*fp_callback)(PANEL_TO_MCU_CMD event, uint8_t param));
void ftd_mw_display_manager_rx_process(void);
#endif 

