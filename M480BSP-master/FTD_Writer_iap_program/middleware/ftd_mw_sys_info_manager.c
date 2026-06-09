/****************************************************************************
@FILENAME:  ftd_mw_sys_info_manager.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.16
@COPYRIGHT: FTD co.ltd
****************************************************************************/

#include "ftd_mw_sys_info_manager.h"
#include "ftd_mw_log_manager.h"
#include "ftd_drv_flash_access.h"
#include "ftd_utils.h"
#include "ftd_data_model.h"
#include "NuMicro.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h> 

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
const WRITER_INFO const_writer_info_default =
{
    .writer_hw_ver  = "V01.00.01",
    .writer_sw_ver  = "V00.00.01",
    //.writer_sn      = "10101010101010101010",
    .writer_sn      = "511290001",
};

static SYS_INFO sys_info_g;

/*---------------------------------------------------------------------------------------------------------*/
/* Define functions                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief Read system information manager data
 * 
 * Reads system information data from specified partition into provided buffer
 * 
 * @param p_st_sys_info Pointer to SYS_INFO structure for storing the read system information data
 * 
 * @return int8_t Operation result status code
 *         - Success: Returns 0 or positive number
 *         - Failure: Returns negative error code
 */
static int8_t ftd_mw_sys_info_manager_read(SYS_INFO *p_st_sys_info)
{
    if( ftd_drv_flash_operation(PARTITION_WRITER_MANAGER, OPERATION_READ, 0, 
                                    sizeof(SYS_INFO), (uint8_t *)p_st_sys_info) < 0)
    {
        JYMC_LOG_WARN("SYS_INFO read err");
        return -1;
    }
    
    if(ftd_utils_hw_crc16((uint8_t *)p_st_sys_info, sizeof(SYS_INFO) - sizeof(p_st_sys_info->sys_info_crc16)) 
            != p_st_sys_info->sys_info_crc16) // sys_info error or Not deployed by the host computer.
    {
        /* reset to default value */
        JYMC_LOG_WARN("SYS_INFO crc err, reset to default value");
        memset(&p_st_sys_info->st_writer_info,      0x00, sizeof(WRITER_INFO));
        memcpy(&p_st_sys_info->st_writer_info.writer_hw_ver, const_writer_info_default.writer_hw_ver, sizeof(const_writer_info_default.writer_hw_ver));
        memcpy(&p_st_sys_info->st_writer_info.writer_sw_ver, const_writer_info_default.writer_sw_ver, sizeof(const_writer_info_default.writer_sw_ver));
        memcpy(&p_st_sys_info->st_writer_info.writer_sn,     const_writer_info_default.writer_sn,     sizeof(const_writer_info_default.writer_sn));
        endian_reverse_buff((uint8_t*)&p_st_sys_info->st_writer_info.writer_hw_ver, sizeof(p_st_sys_info->st_writer_info.writer_hw_ver));
        endian_reverse_buff((uint8_t*)&p_st_sys_info->st_writer_info.writer_sw_ver, sizeof(p_st_sys_info->st_writer_info.writer_sw_ver));
        endian_reverse_buff((uint8_t*)&p_st_sys_info->st_writer_info.writer_sn, sizeof(p_st_sys_info->st_writer_info.writer_sn));
        memset(p_st_sys_info->st_fw_info,           0x00, (sizeof(FW_INFO) * SUPPORT_FW_NUM));
        p_st_sys_info->support_fw_counts            = SUPPORT_FW_NUM;

        /* re-calc crc16 */
        p_st_sys_info->sys_info_crc16 = ftd_utils_hw_crc16((uint8_t *)p_st_sys_info, sizeof(SYS_INFO) - sizeof(p_st_sys_info->sys_info_crc16));

        /* save in retention memory */
        ftd_mw_sys_info_manager_save_nv();
    }
    return 0;
}

/**
 * @brief Write system information manager data
 * 
 * Writes system information data from provided buffer to specified flash partition.
 * The operation includes erasing the target partition first, then writing the new data.
 * 
 * @param p_st_sys_info Pointer to SYS_INFO structure containing the system information data to be written
 * 
 * @return int8_t Operation result status code
 *         - Success: Returns 0
 *         - Failure: Returns negative error code from underlying flash driver
 */
static int8_t ftd_mw_sys_info_manager_write(SYS_INFO *p_st_sys_info)
{
    int8_t ret = 0;

    ret = ftd_drv_flash_operation(PARTITION_WRITER_MANAGER, OPERATION_ERASE, 0, 
                                    sizeof(SYS_INFO), NULL);
    if (ret < 0)
        return ret;

    ret = ftd_drv_flash_operation(PARTITION_WRITER_MANAGER, OPERATION_WRITE, 0, 
                                    sizeof(SYS_INFO), (uint8_t *)p_st_sys_info);
    
    return ret;
}

/**
 * @brief Write system information manager data
 *
 * Writes system information data from provided buffer to specified flash partition.
 * The operation includes erasing the target partition first, then writing the new data.
 *
 * @param p_st_sys_info Pointer to SYS_INFO structure containing the system information data to be written
 *
 * @return int8_t Operation result status code
 *         - Success: Returns 0
 *         - Failure: Returns negative error code from underlying flash driver
 */
int8_t ftd_mw_sys_info_manager_save_nv(void)
{
    int8_t ret = 0;

    ret = ftd_drv_flash_operation(PARTITION_WRITER_MANAGER, OPERATION_ERASE, 0,
        sizeof(SYS_INFO), NULL);
    if (ret < 0)
        return ret;

    ret = ftd_drv_flash_operation(PARTITION_WRITER_MANAGER, OPERATION_WRITE, 0,
        sizeof(SYS_INFO), (uint8_t*)&sys_info_g);

    return ret;
}


/**
 * @brief get system information manager data
 *
 * Writes system information data from provided buffer to specified flash partition.
 * The operation includes erasing the target partition first, then writing the new data.
 *
 * @param p_st_sys_info Pointer to SYS_INFO structure containing the system information data to be get
 *
 * @return void
 */
void   ftd_mw_sys_info_manager_get(SYS_INFO* p_st_sys_info)
{
    memcpy(p_st_sys_info, &sys_info_g, sizeof(SYS_INFO));
}

/**
 * @brief get system information manager data
 *
 * Writes system information data from provided buffer to specified flash partition.
 * The operation includes erasing the target partition first, then writing the new data.
 *
 * @param p_st_sys_info Pointer to SYS_INFO structure containing the system information data to be set
 *
 * @return void
 */
void   ftd_mw_sys_info_manager_set(SYS_INFO* p_st_sys_info)
{
    /* update crc16 */
    p_st_sys_info->sys_info_crc16 = ftd_utils_hw_crc16((uint8_t*)p_st_sys_info, sizeof(SYS_INFO) - sizeof(p_st_sys_info->sys_info_crc16));

    memcpy(&sys_info_g, p_st_sys_info, sizeof(SYS_INFO));
}

/**
 * @brief init system information manager data
 *
 * init system information data by read specified flash partition.
 *
 * @param void 
 *
 * @return int8_t Operation result status code
 *         - Success: Returns 0
 *         - Failure: Returns negative error code from underlying flash driver
 */
int8_t ftd_mw_sys_info_manager_init(void)
{
    int8_t ret;
    ret = ftd_mw_sys_info_manager_read(&sys_info_g);

    // print sys_info
    if (ret == 0)
    {
        JYMC_LOG_DEBUG("SYS_INFO Size :%x", sizeof(SYS_INFO));
        ftd_mw_sys_info_manager_sys_info_dump(&sys_info_g);
    }

    return ret;
}

/**
 * @brief endian_reverse fw_info
 *
 * reverse each byte of fw_info
 *
 * @param p_fw_info Pointer of FW_INFO
 *
 * @return void
 */
void ftd_mw_sys_info_manager_reverse_endian_writer_info(WRITER_INFO* p_writer_info)
{
    endian_reverse_buff((uint8_t*)p_writer_info->writer_hw_ver, sizeof(p_writer_info->writer_hw_ver));
    endian_reverse_buff((uint8_t*)p_writer_info->writer_sw_ver, sizeof(p_writer_info->writer_sw_ver));
    endian_reverse_buff((uint8_t*)p_writer_info->writer_sn,     sizeof(p_writer_info->writer_sn));
}

/**
 * @brief endian_reverse fw_info
 *
 * reverse each byte of fw_info
 *
 * @param p_fw_info Pointer of FW_INFO
 *
 * @return void
 */
void ftd_mw_sys_info_manager_reverse_endian_fw_info(FW_INFO* p_fw_info)
{
    p_fw_info->chip_id                          = endian_reverse_u32(p_fw_info->chip_id);
    p_fw_info->chip_code_partition_start_addr   = endian_reverse_u32(p_fw_info->chip_code_partition_start_addr);
    p_fw_info->chip_code_partition_size         = endian_reverse_u32(p_fw_info->chip_code_partition_size);
    p_fw_info->chip_authcode_partition_addr     = endian_reverse_u32(p_fw_info->chip_authcode_partition_addr);

    p_fw_info->st_bin_info.size                 = endian_reverse_u32(p_fw_info->st_bin_info.size);
    p_fw_info->st_bin_info.allow_write_counts   = endian_reverse_u32(p_fw_info->st_bin_info.allow_write_counts);
    p_fw_info->st_bin_info.remain_counts        = endian_reverse_u32(p_fw_info->st_bin_info.remain_counts);
    endian_reverse_buff((uint8_t*)p_fw_info->st_bin_info.ver, sizeof(p_fw_info->st_bin_info.ver));
    p_fw_info->st_bin_info.reserved             = endian_reverse_u16(p_fw_info->st_bin_info.reserved);
    p_fw_info->st_bin_info.bin_crc16            = endian_reverse_u16(p_fw_info->st_bin_info.bin_crc16);

    p_fw_info->st_triple_info.size              = endian_reverse_u32(p_fw_info->st_triple_info.size);
    p_fw_info->st_triple_info.allow_write_counts = endian_reverse_u32(p_fw_info->st_triple_info.allow_write_counts);
    p_fw_info->st_triple_info.remain_counts     = endian_reverse_u32(p_fw_info->st_triple_info.remain_counts);
    p_fw_info->st_triple_info.reserved          = endian_reverse_u16(p_fw_info->st_triple_info.reserved);
    p_fw_info->st_triple_info.triple_crc16      = endian_reverse_u16(p_fw_info->st_triple_info.triple_crc16);

    p_fw_info->st_roll_code_info.start_value    = endian_reverse_u32(p_fw_info->st_roll_code_info.start_value);
    p_fw_info->st_roll_code_info.end_value      = endian_reverse_u32(p_fw_info->st_roll_code_info.end_value);
    p_fw_info->st_roll_code_info.allow_write_counts = endian_reverse_u32(p_fw_info->st_roll_code_info.allow_write_counts);
    p_fw_info->st_roll_code_info.remain_counts  = endian_reverse_u32(p_fw_info->st_roll_code_info.remain_counts);

    p_fw_info->reserved2                        = endian_reverse_u16(p_fw_info->reserved2);
    p_fw_info->fw_info_crc16                    = endian_reverse_u16(p_fw_info->fw_info_crc16);
}

/**
 * @brief endian_reverse sys_info
 *
 * reverse each byte of sys_info
 *
 * @param p_sys_info Pointer to sys_info
 *
 * @return void
 */
void ftd_mw_sys_info_manager_reverse_endian_sys_info(SYS_INFO* p_sys_info)
{
    endian_reverse_buff((uint8_t*)p_sys_info->st_writer_info.writer_hw_ver, sizeof(p_sys_info->st_writer_info.writer_hw_ver));
    endian_reverse_buff((uint8_t*)p_sys_info->st_writer_info.writer_sw_ver, sizeof(p_sys_info->st_writer_info.writer_sw_ver));
    endian_reverse_buff((uint8_t*)p_sys_info->st_writer_info.writer_sn, sizeof(p_sys_info->st_writer_info.writer_sn));    
    for (uint8_t i = 0; i < SUPPORT_FW_NUM; i++)
    {
        p_sys_info->st_fw_info[i].chip_id                           = endian_reverse_u32(p_sys_info->st_fw_info[i].chip_id);
        p_sys_info->st_fw_info[i].chip_code_partition_start_addr    = endian_reverse_u32(p_sys_info->st_fw_info[i].chip_code_partition_start_addr);
        p_sys_info->st_fw_info[i].chip_code_partition_size          = endian_reverse_u32(p_sys_info->st_fw_info[i].chip_code_partition_size);
        p_sys_info->st_fw_info[i].chip_authcode_partition_addr      = endian_reverse_u32(p_sys_info->st_fw_info[i].chip_authcode_partition_addr);  
        p_sys_info->st_fw_info[i].st_bin_info.size                  = endian_reverse_u32(p_sys_info->st_fw_info[i].st_bin_info.size);
        p_sys_info->st_fw_info[i].st_bin_info.allow_write_counts    = endian_reverse_u32(p_sys_info->st_fw_info[i].st_bin_info.allow_write_counts);
        p_sys_info->st_fw_info[i].st_bin_info.remain_counts         = endian_reverse_u32(p_sys_info->st_fw_info[i].st_bin_info.remain_counts);
        endian_reverse_buff((uint8_t*)p_sys_info->st_fw_info[i].st_bin_info.ver, sizeof(p_sys_info->st_fw_info[i].st_bin_info.ver));
        p_sys_info->st_fw_info[i].st_bin_info.reserved              = endian_reverse_u16(p_sys_info->st_fw_info[i].st_bin_info.reserved);
        p_sys_info->st_fw_info[i].st_bin_info.bin_crc16             = endian_reverse_u16(p_sys_info->st_fw_info[i].st_bin_info.bin_crc16); 
        p_sys_info->st_fw_info[i].st_triple_info.size               = endian_reverse_u32(p_sys_info->st_fw_info[i].st_triple_info.size);
        p_sys_info->st_fw_info[i].st_triple_info.allow_write_counts = endian_reverse_u32(p_sys_info->st_fw_info[i].st_triple_info.allow_write_counts);
        p_sys_info->st_fw_info[i].st_triple_info.remain_counts      = endian_reverse_u32(p_sys_info->st_fw_info[i].st_triple_info.remain_counts);
        p_sys_info->st_fw_info[i].st_triple_info.reserved           = endian_reverse_u16(p_sys_info->st_fw_info[i].st_triple_info.reserved);
        p_sys_info->st_fw_info[i].st_triple_info.triple_crc16       = endian_reverse_u16(p_sys_info->st_fw_info[i].st_triple_info.triple_crc16);   
        p_sys_info->st_fw_info[i].st_roll_code_info.start_value     = endian_reverse_u32(p_sys_info->st_fw_info[i].st_roll_code_info.start_value);
        p_sys_info->st_fw_info[i].st_roll_code_info.end_value       = endian_reverse_u32(p_sys_info->st_fw_info[i].st_roll_code_info.end_value);
        p_sys_info->st_fw_info[i].st_roll_code_info.allow_write_counts = endian_reverse_u32(p_sys_info->st_fw_info[i].st_roll_code_info.allow_write_counts);
        p_sys_info->st_fw_info[i].st_roll_code_info.remain_counts   = endian_reverse_u32(p_sys_info->st_fw_info[i].st_roll_code_info.remain_counts);   
        p_sys_info->st_fw_info[i].reserved2                         = endian_reverse_u16(p_sys_info->st_fw_info[i].reserved2);
        p_sys_info->st_fw_info[i].fw_info_crc16                     = endian_reverse_u16(p_sys_info->st_fw_info[i].fw_info_crc16);
    }   
    p_sys_info->sys_info_crc16 = endian_reverse_u16(p_sys_info->sys_info_crc16);
}

/************************************ for debug ************************************/

/**
 * @brief dump writer_info
 *
 * dump writer_info for debug
 *
 * @param p_writer_info Pointer of WRITER_INFO
 *
 * @return void 
 */
void ftd_mw_sys_info_manager_writer_info_dump(WRITER_INFO *p_writer_info)
{
#if (LOG_TYPE >= LOG_LEVEL_DEBUG)
    JYMC_LOG_DEBUG("\n [WRITER_INFO]");
    JYMC_LOG_RAW(" HW_VER");
    for (uint8_t i = 0; i < WRITER_HW_VER_LEN; i++)
    {
        JYMC_LOG_RAW(" %02x", p_writer_info->writer_hw_ver[i]);
    }
    JYMC_LOG_RAW("\n SW_VER");
    for (uint8_t i = 0; i < WRITER_SW_VER_LEN; i++)
    {
        JYMC_LOG_RAW(" %02x", p_writer_info->writer_sw_ver[i]);
    }
    JYMC_LOG_RAW("\n SN    ");
    for (uint8_t i = 0; i < WRITER_SN_LEN; i++)
    {
        JYMC_LOG_RAW(" %02x", p_writer_info->writer_sn[i]);
    }
    JYMC_LOG_RAW("\n");
#endif
}

/**
 * @brief dump fw_info
 *
 * dump fw_info for debug
 *
 * @param p_fw_info Pointer of FW_INFO
 *
 * @return void
 */
void ftd_mw_sys_info_manager_fw_info_dump(FW_INFO* p_fw_info)
{
#if (LOG_TYPE >= LOG_LEVEL_DEBUG)
    JYMC_LOG_DEBUG("\n [FW_INFO]");
    JYMC_LOG_RAW(" fw_active:      0x%02x \n deploy_flag:    0x%02x \n fw_region:      0x%02x \n reserved1:      0x%02x \n",
        p_fw_info->fw_active, p_fw_info->deploy_flag, p_fw_info->fw_region, p_fw_info->reserved1);
    JYMC_LOG_RAW(" chip_id:                            %08x \n chip_code_partition_start_addr :    %08x \n chip_code_partition_size :          %08x \n chip_authcode_partition_addr :      %08x \n",
        p_fw_info->chip_id, p_fw_info->chip_code_partition_start_addr, p_fw_info->chip_code_partition_size, p_fw_info->chip_authcode_partition_addr);

    JYMC_LOG_RAW(" [BIN_INFO]\n bin_info.size :                     %08x \n bin_info.allow_write_counts :       %08x \n bin_info.remain_counts :            %08x \n",
        p_fw_info->st_bin_info.size, p_fw_info->st_bin_info.allow_write_counts, p_fw_info->st_bin_info.remain_counts);
    JYMC_LOG_RAW(" bin_info.ver[20] :                  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x \n",
        p_fw_info->st_bin_info.ver[0],  p_fw_info->st_bin_info.ver[1],  p_fw_info->st_bin_info.ver[2],  p_fw_info->st_bin_info.ver[3],  p_fw_info->st_bin_info.ver[4],
        p_fw_info->st_bin_info.ver[5],  p_fw_info->st_bin_info.ver[6],  p_fw_info->st_bin_info.ver[7],  p_fw_info->st_bin_info.ver[8],  p_fw_info->st_bin_info.ver[9],
        p_fw_info->st_bin_info.ver[10], p_fw_info->st_bin_info.ver[11], p_fw_info->st_bin_info.ver[12], p_fw_info->st_bin_info.ver[13], p_fw_info->st_bin_info.ver[14],
        p_fw_info->st_bin_info.ver[15], p_fw_info->st_bin_info.ver[16], p_fw_info->st_bin_info.ver[17], p_fw_info->st_bin_info.ver[18], p_fw_info->st_bin_info.ver[19]);
    JYMC_LOG_RAW(" bin_info.reserved :                 0x%04x  \n bin_info.bin_crc16 :                %08x \n",
        p_fw_info->st_bin_info.reserved, p_fw_info->st_bin_info.bin_crc16);

    JYMC_LOG_RAW(" [TRIPLE_INFO]\n");
    JYMC_LOG_RAW(" triple_info.size :                  %08x \n triple_info.allow_write_counts :    %08x \n triple_info.remain_counts :         %08x \n triple_info.reserved :              %08x \n triple_info.triple_crc16 :          %08x \n",
        p_fw_info->st_triple_info.size, p_fw_info->st_triple_info.allow_write_counts, p_fw_info->st_triple_info.remain_counts, p_fw_info->st_triple_info.reserved, p_fw_info->st_triple_info.triple_crc16);

    JYMC_LOG_RAW(" [ROLLCODE_INFO]\n roll_code_info.start_value :        %08x \n roll_code_info.end_value :          %08x \n roll_code_info.allow_write_counts : %08x \n roll_code_info.remain_counts :      %08x \n reserved2 :                         %08x \n fw_info_crc16 :                     %08x \n",
        p_fw_info->st_roll_code_info.start_value, p_fw_info->st_roll_code_info.end_value, p_fw_info->st_roll_code_info.allow_write_counts, p_fw_info->st_roll_code_info.remain_counts, \
        p_fw_info->reserved2, p_fw_info->fw_info_crc16);
#endif
}

/**
 * @brief dump sys_info
 *
 * dump sys_info for debug
 *
 * @param p_sys_info Pointer to sys_info
 *
 * @return void
 */
void ftd_mw_sys_info_manager_sys_info_dump(SYS_INFO* p_sys_info)
{
#if (LOG_TYPE >= LOG_LEVEL_DEBUG)
    JYMC_LOG_DEBUG("\n [SYS_INFO] dump");
    
    // Dump WRITER_INFO
    JYMC_LOG_RAW(" [WRITER_INFO]\n");
    JYMC_LOG_RAW(" writer_hw_ver:  ");
    for (uint8_t i = 0; i < WRITER_HW_VER_LEN; i++)
    {
        JYMC_LOG_RAW(" %02x", p_sys_info->st_writer_info.writer_hw_ver[i]);
    }
    JYMC_LOG_RAW("\n");
    
    JYMC_LOG_RAW(" writer_sw_ver:  ");
    for (uint8_t i = 0; i < WRITER_SW_VER_LEN; i++)
    {
        JYMC_LOG_RAW(" %02x", p_sys_info->st_writer_info.writer_sw_ver[i]);
    }
    JYMC_LOG_RAW("\n");
    
    JYMC_LOG_RAW(" writer_sn:      ");
    for (uint8_t i = 0; i < WRITER_SN_LEN; i++)
    {
        JYMC_LOG_RAW(" %02x", p_sys_info->st_writer_info.writer_sn[i]);
    }
    JYMC_LOG_RAW("\n\n");
    
    // Dump FW_INFO array
    for (uint8_t j = 0; j < SUPPORT_FW_NUM; j++)
    {
        ftd_mw_sys_info_manager_fw_info_dump(&p_sys_info->st_fw_info[j]);
    }
    
    // Dump SYS_INFO members
    JYMC_LOG_RAW(" support_fw_counts: %d \n reserved1:      0x%02x \n sys_info_crc16: %08x \n",
        p_sys_info->support_fw_counts, p_sys_info->reserved1, p_sys_info->sys_info_crc16);
#endif
}
