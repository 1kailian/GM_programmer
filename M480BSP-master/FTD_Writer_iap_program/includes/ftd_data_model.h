#ifndef __FTD_DATA_MODEL_H__
#define __FTD_DATA_MODEL_H__

#include <stdint.h>
#include <string.h>
#include <stdbool.h> 

#define WRITER_HW_VER_LEN (20)
#define WRITER_SW_VER_LEN (20)
#define WRITER_SN_LEN     (20)
#define MCU_SW_VER_LEN    (20)

#define SUPPORT_FW_NUM    (3)

#define TRIPLE_DATA_LEN   (26)  // product_id 4 + device_secret 16 + mac 6 = 26

typedef enum 
{
    APP_PROGRAMMING,
    APP_BURN_ENGINE,
    APP_WRITER_MANAGER,
} APPLICATION;

typedef enum
{
    BURN_STOP,
    BURN_MANNUL,
    BURN_AUTO,
} BRUN_STATUS_E;

typedef struct
{
    char writer_hw_ver[WRITER_HW_VER_LEN];
    char writer_sw_ver[WRITER_SW_VER_LEN];
    char writer_sn[WRITER_SN_LEN];
} WRITER_INFO;


typedef struct
{
    uint32_t    size;//Byte
    uint32_t    allow_write_counts;
    uint32_t    remain_counts;
    char        ver[MCU_SW_VER_LEN];
    uint16_t    reserved; //for aligned
    uint16_t    bin_crc16; //bin crc16, is not info
} BIN_INFO;

typedef struct
{
    uint16_t    size;//Byte
    uint16_t    config_data_crc16; //config_data crc16, is not info struct
} CONFIG_INFO;

typedef struct
{
    uint32_t    size; //Byte
    uint32_t    allow_write_counts;
    uint32_t    remain_counts;
    uint16_t    reserved; //for aligned
    uint16_t    triple_crc16; //triple data crc,is not info
} TRIPLE_INFO;


typedef struct
{
    uint32_t    start_value;
    uint32_t    end_value;
    uint32_t    allow_write_counts;
    uint32_t    remain_counts;
} ROLLCODE_INFO;

// add bin user config segment, and write into target flash in burn process
// add chip id info segment, and check chip id in burn process
typedef struct
{
    uint8_t         fw_active; //update fw_active by writer after deploy succ,need recalc fw_info_crc16
    uint8_t         deploy_flag; //Upper Computer Deployment Status 0-not deploy 1-deploy
    uint8_t         fw_region; ///0-fw1 1-fw2 2-fw3,Upper Computer Deployment update only
    uint8_t         reserved1; //for aligned
    
    uint32_t        chip_id;
    uint32_t        chip_code_partition_start_addr;
    uint32_t        chip_code_partition_size;
    //uint32_t        chip_config_data_start_addr;
    uint32_t        chip_authcode_partition_addr;

    BIN_INFO        st_bin_info;
    //CONFIG_INFO     st_config_info;
    TRIPLE_INFO     st_triple_info;
    ROLLCODE_INFO   st_roll_code_info;
    uint16_t        reserved2; //for aligned
    uint16_t        fw_info_crc16; //if change FW_INFO content, must update fw_info_crc16
} FW_INFO;


typedef struct
{
    WRITER_INFO st_writer_info;
    FW_INFO     st_fw_info[SUPPORT_FW_NUM];
    uint8_t     support_fw_counts;
    uint8_t     reserved1; //for aligned
    uint16_t    sys_info_crc16;
} SYS_INFO;


typedef struct
{
    uint8_t     channel_num; //0-3
    uint8_t     channel_online; //slace mcu online/offline
    uint8_t     fw_num; //0-2~~~fw0-fw2
    uint8_t     cur_burnning_status; //0-none 1-success 2-fail
    uint32_t    burn_total_count;
    uint32_t    burn_remain_count;
    uint32_t    burn_success_count;
    uint32_t    burn_error_code;
    uint8_t     burn_process_rate;
    uint8_t     reserved1[4]; //for aligned
} CHANNEL_BURN_STATUS; 


#endif 

