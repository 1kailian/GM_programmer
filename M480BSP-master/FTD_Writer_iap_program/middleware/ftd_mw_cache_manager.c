/****************************************************************************
@FILENAME:  ftd_mw_cache_manager.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.16
@COPYRIGHT: FTD co.ltd
****************************************************************************/
#include "ftd_drv_flash_access.h"
#include "ftd_mw_cache_manager.h"
#include "ftd_mw_log_manager.h"
#include "ftd_mw_sys_info_manager.h"
#include "ftd_utils.h"
#include "NuMicro.h"
#include <string.h>

// #include <stdio.h>
// #include <stdbool.h>


/**
 * @brief Reads data from the cache partition in flash memory.
 * @param buff Pointer to the buffer where the read data will be stored. 
 *             Must not be NULL.
 * @param total_length Total size of the data to be read. Represents the 
 *                     maximum size of the cache partition.
 * @param cur_length The length of data to read in the current operation.
 * @param had_read_length The offset (in bytes) from the start of the cache 
 *                        partition where the read operation begins.
 * @return int8_t Returns 0 on success, -1 if the input parameters are invalid 
 *                or if the read operation exceeds the partition size.
 */
static int8_t ftd_mw_cache_manager_read(uint8_t *buff, uint32_t total_length, uint32_t cur_length, uint32_t had_read_length)
{
    int8_t ret = 0;

    // Validate input parameters to ensure the read operation is within bounds.
    if( (total_length > PARTITION_CACHE_SIZE) || ((had_read_length + cur_length) > total_length) )
        return -1;

    // Perform the read operation on the flash memory cache partition.
    ret = ftd_drv_flash_operation(PARTITION_CACHE, OPERATION_READ, had_read_length, cur_length, buff);
    return ret;
}

/**
 * @brief Writes data to the cache partition in flash memory.
 * @param buff Pointer to the buffer containing the data to be written.
 * @param total_length Total length of the data to be written across all segments.
 * @param cur_length Length of the current segment of data to be written.
 * @param written_length Total length of data already written in previous segments.
 * @return int8_t 
 *         - Returns 0 on success.
 *         - Returns -1 if the input parameters are invalid (e.g., total_length exceeds partition size
 *           or the sum of written_length and cur_length exceeds total_length).
 *         - Returns the error code from the flash operation if an error occurs during erase or write.
 */
int8_t ftd_mw_cache_manager_write(uint8_t *buff, uint32_t total_length, uint32_t cur_length, uint32_t written_length)
{
    static int8_t cache_partition_erased = 0;
    int8_t ret = 0;

    // Validate input parameters to ensure they are within acceptable limits.
    if( (total_length > PARTITION_CACHE_SIZE) || ((written_length + cur_length) > total_length) )
        return -1;

    // Erase the cache partition if it hasn't been erased yet.
    if(cache_partition_erased == 0)
    {
        ret = ftd_drv_flash_operation(PARTITION_CACHE, OPERATION_ERASE, 0, 
                                        PARTITION_CACHE_SIZE, NULL); // Erase the entire cache partition.
        if (ret < 0)
            return ret;
        cache_partition_erased = 1; // Mark the partition as erased.
    }

    // Write the current segment of data to the cache partition.
    ret = ftd_drv_flash_operation(PARTITION_CACHE, OPERATION_WRITE, written_length, 
                                    cur_length, buff);

    // Reset the erase flag if the entire data has been written.
    if((written_length + cur_length) >= total_length) // All data segments have been written.
        cache_partition_erased = 0; 
    
    return ret;
}


/**
 * @brief Retrieves firmware information from the cache manager.
 *
 * This function reads system information from the cache, validates its CRC16 checksum,
 * finds the deployed firmware information, validates its CRC16 checksum, and copies
 * the valid firmware information to the output parameter.
 *
 * @param[out] p_st_fw_info Pointer to the FW_INFO structure where the retrieved
 *                          firmware information will be stored. Must not be NULL.
 *
 * @return int8_t
 *         - 0  Success
 *         - -1 Failure (invalid parameters, CRC check failed, or no valid firmware found)
 *
 * @note This function assumes that the cache partition has been properly initialized
 *       and contains valid system information with correct CRC checksums.
 * *     (0-FW1, 1-FW2, 2-FW3, etc.)
 */
int8_t ftd_mw_cache_manager_get_fw_info(FW_INFO *p_st_fw_info)
{
    int8_t          ret = 0;
    int8_t          fw_index = -1; // 0-FW1 1-FW2 2-FW3
    SYS_INFO        st_sys_info;
    uint8_t         read_buffer[sizeof(FW_INFO)] = { 0 };
    FW_INFO         st_fw_info;

    /* read FW_INFO from cache */
    ret = ftd_mw_cache_manager_read(read_buffer, sizeof(FW_INFO), sizeof(FW_INFO), 0);
    if(ret < 0)
        return -1;

    #if (LOG_TYPE >= LOG_LEVEL_DEBUG)
    JYMC_LOG_DEBUG("GET CACHE FW INFO ret:%d len[0x%02x] : ", ret, sizeof(FW_INFO));
    FTD_LOG_DEBUG_BUFF(read_buffer, sizeof(read_buffer), sizeof(FW_INFO));
    #endif

    memcpy(&st_fw_info, read_buffer, sizeof(FW_INFO));

    ftd_mw_sys_info_manager_get(&st_sys_info);

    /* check st_fw_info crc */
    //uint16_t fw_info_crc16 = ftd_utils_hw_crc16((uint8_t*)&st_fw_info, sizeof(st_fw_info) - sizeof(st_fw_info.fw_info_crc16));
    uint16_t fw_info_crc16 = crc16_ccitt_update(0xFFFF, (uint8_t*)&st_fw_info, sizeof(st_fw_info) - sizeof(st_fw_info.fw_info_crc16));
    st_fw_info.fw_info_crc16 = ((st_fw_info.fw_info_crc16 & 0x00FF) << 8) | ((st_fw_info.fw_info_crc16 & 0xFF00) >> 8);
    if (fw_info_crc16 != st_fw_info.fw_info_crc16 )
    {
        JYMC_LOG_INFO(" sys_info_crc16 check error! calc:0x%04x  buff:0x%04x\n", fw_info_crc16, st_fw_info.fw_info_crc16);
        return -2;
    }

    /* find the valid deploy fw info */
    //for(fw_index = 0; fw_index < st_sys_info.support_fw_counts; fw_index++)
    //{
    //    if(st_sys_info.st_fw_info[fw_index].deploy_flag != 0 )
    //        break;
    //}

    /* check fw index is valid? */
    fw_index = st_fw_info.fw_region - 1;
    if( (fw_index >= st_sys_info.support_fw_counts) || (fw_index < 0) )
        return -3;

    /* endian reverse */
    ftd_mw_sys_info_manager_reverse_endian_fw_info(&st_fw_info);

    /* save into sys_info, deploy_flag is 0, fw still unavailable */
    memcpy((uint8_t*)&st_sys_info.st_fw_info[fw_index], (uint8_t*)&st_fw_info, sizeof(st_fw_info));
    st_sys_info.st_fw_info[fw_index].deploy_flag = 0;
    ftd_mw_sys_info_manager_set(&st_sys_info);
    if (ftd_mw_sys_info_manager_save_nv())
        return -4;

    *p_st_fw_info = st_fw_info;

    return 0;
}

