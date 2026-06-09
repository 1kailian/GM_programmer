#ifndef __FTD_MW_SLAVE_MANAGER_H__
#define __FTD_MW_SLAVE_MANAGER_H__ 

#include "ftd_drv_burn_uart.h"
#include "ftd_mw_hc_manager.h"   // for debug 

#include <stdint.h>
#include <stdbool.h>

///////////////////// for deubg

typedef enum
{
    SLAVE_EXT_CMD_ERASE_FLASH = 0x004F,
    SLAVE_EXT_CMD_READ_FLASH  = 0x0050,
    SLAVE_EXT_CMD_PROG_FLASH  = 0x0051,
} EN_SLAVE_EXT_CMD;


#define SLAVE_BOOT_TRIGGER_CNT              (5)
#define SLAVE_BOOT_TRIGGER_DELAY            (80000)//(1000) // 1ms

#define SLAVE_DISABLE_WP_TRIGGER_CNT        (1)

//#define SLAVE_CMD_RETRY_CNT_MAX             (3)

#define SLAVE_FL100_FLASH_BIN_ADDR_MAX      (0x08038000)

#define SLAVE_FLASH_TEST_ADDR               (0x08021000)
#define SLAVE_FLASH_BIN_ADDR                (0x08008000)
#define SLAVE_FLASH_START_ADDR              SLAVE_FLASH_TEST_ADDR
#define SLAVE_FLASH_PAGE_SIZE               (0x00000100) // 256 bytes per page

#define SLAVE_FL100_FLASH_BIN_ADDR_START    (0x08008000)
#define SLAVE_FL100_FLASH_BIN_A_ADDR_MAX    (0x08020000)
#define SLAVE_FL100_FLASH_BIN_B_ADDR_MAX    (0x08038000)
#define SLAVE_FLASH_OP_ADDR_SIZE            (4)

/////////////////// SLAVE FLASH STATUS
#define SLAVE_FLASH_STATUS_LEN              (2)
#define SLAVE_FLASH_STATUS_ACK_LEN          (FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + EXTEND_CMD_SIZE + SLAVE_FLASH_STATUS_LEN + CHECKSUM_SIZE)
/////////////////// SLAVE FLASH STATUS END

/////////////////// SLAVE FLASH ERASE
#define SLAVE_FLASH_ERASE_LEN               (4)
#define SLAVE_FLASH_ERASE_ACK_SIZE          (FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_ERASE_LEN + CHECKSUM_SIZE)
/////////////////// SLAVE FLASH ERASE END

/////////////////// SLAVE FLASH PROG 
#define SLAVE_FLASH_PROG_SIZE               (SLAVE_FLASH_PAGE_SIZE << 1) // 2 pages, must be multiple of 1page
#define SLAVE_FLASH_OP_LENGTH_SIZE          (3)
#define SLAVE_FLASH_PROG_PRAM_LEN_OFFSET    (FRAME_HEADER_SIZE + CMD_SIZE)
#define SLAVE_FLASH_PROG_EXT_CMD_OFFSET     (FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE)
#define SLAVE_FLASH_PROG_ADDR_OFFSET        (FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + EXTEND_CMD_SIZE)
#define SLAVE_FLASH_PROG_LEN_OFFSET         (FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE)
#define SLAVE_FLASH_PROG_DATA_OFFSET        (FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_OP_LENGTH_SIZE)
#define SLAVE_FLASH_PROG_PRAM_LEN_MAX       (EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_OP_LENGTH_SIZE + SLAVE_FLASH_PROG_SIZE)
#define SLAVE_FLASH_PROG_CMD_SIZE_MAX       (FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_OP_LENGTH_SIZE + SLAVE_FLASH_PROG_SIZE + CHECKSUM_SIZE)
#define SLAVE_FLASH_PROG_ACK_LEN            (0x0E)// fixed 14
/////////////////// SLAVE FLASH PROG END

/////////////////// SLAVE FLASH READ 
#define SLAVE_FLASH_READ_SIZE               (SLAVE_FLASH_PAGE_SIZE << 2) // 4 pages, must be multiple of 1page
#define SLAVE_FLASH_READ_LENGTH_SIZE        (4)
#define SLAVE_FLASH_READ_DATA_OFFSET        (FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_READ_LENGTH_SIZE)
#define SLAVE_FLASH_READ_CMD_SIZE_MAX       (FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_READ_LENGTH_SIZE + CHECKSUM_SIZE)
#define SLAVE_FLASH_READ_CMD_ACK_SIZE_MAX   (FRAME_HEADER_SIZE + CMD_SIZE + PARAM_LEN_SIZE + EXTEND_CMD_SIZE + SLAVE_FLASH_OP_ADDR_SIZE + SLAVE_FLASH_READ_LENGTH_SIZE + SLAVE_FLASH_READ_SIZE + CHECKSUM_SIZE)
/////////////////// SLAVE FLASH READ END

//////////////////////////// debug end
typedef enum
{
    SLAVE_CMD_NONE,                     // 0x00
    SLAVE_CMD_ENTER_BOOT_LOADER,        // 0x01
    SLAVE_CMD_READ_CHIP_ID,
    SLAVE_CMD_REBOOT_CHIP,              // 0x03
    SLAVE_CMD_SET_CPU_CLK,
    SLAVE_CMD_SET_UART_BAUDRATE,

    SLAVE_CMD_READ_PRODUCTION_TEST_STATUS,
    SLAVE_CMD_SET_PROGRAM_JUMP,

    SLAVE_CMD_READ_CODE_CRC,            // 0x08

    SLAVE_CMD_READ_FLASH_ID,
    SLAVE_CMD_READ_FLASH_STATUS_REG,
    SLAVE_CMD_SET_FLASH_STATUS_REG,     // 0x0B
    SLAVE_CMD_SET_FLASH_STATUS_QUIT_WP, // 0x0C
    SLAVE_CMD_SET_FLASH_READ_MODE,
    SLAVE_CMD_SET_FLASH_WRITE_MODE,
    SLAVE_CMD_SET_FLASH_CLK_DIV,
    SLAVE_CMD_SET_FLASH_SAMPLE_POINT,
    SLAVE_CMD_ERASE_ENTIRE_FLASH,
    SLAVE_CMD_ERASE_FLASH_SECTOR,       // 0x12
    SLAVE_CMD_READ_FLASH_OR_RAM_DATA,
    SLAVE_CMD_WRITE_FLASH_DATA,         // 0x14
    SLAVE_CMD_WRITE_RAM_DATA,
    SLAVE_CMD_RESET_FLASH,

    SLAVE_CMD_STATE_SORT,               // 0x17
    SLAVE_CMD_DONE,
    SLAVE_CMD_MAX = 0x7F, //ACK state |= SLAVE_CMD_XXX in mw

} SLAVE_CMD;

typedef struct
{
    uint64_t chip_id;
    uint64_t uart_baudrate;
    uint64_t flash_id;
    uint64_t flash_size;
    uint64_t flash_bin_start_addr;
    uint64_t flash_bin_size;
    uint64_t flash_triple_addr;
    uint8_t  flash_clk_div;
    uint8_t  flash_sample_point;
    uint8_t  flash_clk_sel;
    uint8_t  flash_clk_calib;

    SLAVE_CMD en_slave_cmd_state;
    uint8_t   slave_uart_channel;
    uint8_t   rx_buffer[BURN_UART_DATA_MAX];
} SLAVE_INFO;

typedef struct
{
    uint8_t  fw_num;
    uint32_t bin_length;
    uint32_t cur_read_offset;
} SLAVE_FW_INFO;

void ftd_mw_slave_manager_start_ld(SLAVE_INFO* p_st_slave_info);
/////////////////// debug
bool ftd_mw_slave_manager_start_ld_test(SLAVE_INFO* p_st_slave_info);
bool ftd_mw_slave_manager_disable_wp(SLAVE_INFO* p_st_slave_info);
bool ftd_mw_slave_manager_erase_test(SLAVE_INFO* p_st_slave_info);
bool ftd_mw_slave_manager_program_test(SLAVE_INFO* p_st_slave_info);
bool ftd_mw_slave_manager_enable_wp(SLAVE_INFO* p_st_slave_info);
bool ftd_mw_slave_manager_read_crc_test(SLAVE_INFO* p_st_slave_info);
///////////////////////// debug end
bool ftd_mw_slave_manager_check_ld(SLAVE_INFO* p_st_slave_info);
void ftd_mw_slave_manager_set_default_slave_info(SLAVE_INFO* p_st_slave_info);

#endif 
