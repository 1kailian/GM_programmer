/****************************************************************************
@FILENAME:  ftd_mw_fw_manager.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.16
@COPYRIGHT: FTD co.ltd
****************************************************************************/

#include "ftd_mw_fw_manager.h"
#include "ftd_mw_log_manager.h"
#include "ftd_mw_sys_info_manager.h"
#include "ftd_drv_flash_access.h"
#include "ftd_utils.h"
#include "NuMicro.h"
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#if (!defined BUFF_DYNAMIC_ALLOC)
// use static buffer to store the fw data, cause malloc can not apply 4KB buffer
static uint8_t g_fw_buff[PAGESIZE * 16]; // 4KB = 256B * 16
#endif

static PARTITION_NAME ftd_mw_fw_manager_fw_region_to_partition(uint8_t fw_region)
{
    if(fw_region == 0)
    {
        return PARTITION_FW1_INFO;
    }
    else if(fw_region == 1)
    {
        return PARTITION_FW2_INFO;
    }
    else if(fw_region == 2)
    {
        return PARTITION_FW3_INFO;
    }
    else
    {
        return PARTITION_FW3_INFO;
    }
}

static int8_t ftd_mw_fw_manager_read(uint8_t *buff, uint32_t total_length, uint32_t cur_length, uint32_t had_read_length)
{
    int8_t ret = 0;

    // Validate input parameters to ensure the read operation is within bounds.
    if( (total_length > PARTITION_CACHE_SIZE) || ((had_read_length + cur_length) > total_length) )
        return -1;

    // Perform the read operation on the flash memory cache partition.
    ret = ftd_drv_flash_operation(PARTITION_CACHE, OPERATION_READ, had_read_length, cur_length, buff);
    return ret;
}

int8_t ftd_mw_fw_manager_read_fw_info(uint8_t fw_region, FW_INFO *p_st_fw_info)
{
    PARTITION_NAME en_partition;
    en_partition = ftd_mw_fw_manager_fw_region_to_partition(fw_region);

    return ftd_drv_flash_operation(en_partition, OPERATION_READ, 0, sizeof(FW_INFO), (uint8_t *)p_st_fw_info);
}

int8_t ftd_mw_fw_manager_deploy(FW_INFO *p_st_fw_info)
{
    int8_t ret = 0;
    uint8_t *p_buff;
    uint32_t buff_size = 0;                         // The size of the buffer to read and prog.
    uint32_t prog_size = 0;                         // The size of the data to be programmed.
    uint16_t crc16_result = 0xFFFF;
    uint32_t cache_read_length = 0;

    uint32_t flash_fw_info_addr         = 0;
    uint32_t flash_fw_info_size         = 0;
    uint32_t flash_fw_bin_addr          = 0;
    uint32_t flash_fw_bin_size          = 0;
    uint32_t flash_fw_triple_addr       = 0;
    uint32_t flash_fw_triple_size       = 0;
    PARTITION_NAME en_flash_fw_info_name    = PARTITION_FW1_INFO;
    PARTITION_NAME en_flash_fw_bin_name     = PARTITION_FW1_BIN;
    PARTITION_NAME en_flash_fw_triple_name  = PARTITION_FW1_TRIPLE;

    #if (defined BUFF_DYNAMIC_ALLOC)
    buff_size = PAGESIZE * 16; //4096 Byte, 1 sector
    p_buff = (uint8_t*)malloc(buff_size);
    #else
    buff_size = sizeof(g_fw_buff);
    p_buff = (uint8_t*)g_fw_buff;
    #endif
    
    #if (defined BUFF_DYNAMIC_ALLOC)
    if(p_buff == NULL)
    {
        free(p_buff);
        FTD_LOG_WARN("malloc p_buff failed!");
        return -1;
    }
    #endif

    // 1.get flash fw partition name and start address
    if(p_st_fw_info->fw_region == 1)
    {
        en_flash_fw_info_name   = PARTITION_FW1_INFO;
        en_flash_fw_bin_name    = PARTITION_FW1_BIN;
        en_flash_fw_triple_name = PARTITION_FW1_TRIPLE;
        flash_fw_info_addr      = PARTITION_FW1_INFO_START_ADDR;
        flash_fw_info_size      = PARTITION_FW1_INFO_SIZE;
        flash_fw_bin_addr       = PARTITION_FW1_BIN_START_ADDR;
        flash_fw_bin_size       = PARTITION_FW1_BIN_SIZE;
        flash_fw_triple_addr    = PARTITION_FW1_TRIPLE_START_ADDR;
        flash_fw_triple_size    = PARTITION_FW1_TRIPLE_SIZE;
    }
    else if (p_st_fw_info->fw_region == 2)
    {
        en_flash_fw_info_name   = PARTITION_FW2_INFO;
        en_flash_fw_bin_name    = PARTITION_FW2_BIN;
        en_flash_fw_triple_name = PARTITION_FW2_TRIPLE;
        flash_fw_info_addr      = PARTITION_FW2_INFO_START_ADDR;
        flash_fw_info_size      = PARTITION_FW2_INFO_SIZE;
        flash_fw_bin_addr       = PARTITION_FW2_BIN_START_ADDR;
        flash_fw_bin_size       = PARTITION_FW2_BIN_SIZE;
        flash_fw_triple_addr    = PARTITION_FW2_TRIPLE_START_ADDR;
        flash_fw_triple_size    = PARTITION_FW2_TRIPLE_SIZE;
    }
    else if (p_st_fw_info->fw_region == 3)
    {
        en_flash_fw_info_name   = PARTITION_FW3_INFO;
        en_flash_fw_bin_name    = PARTITION_FW3_BIN;
        en_flash_fw_triple_name = PARTITION_FW3_TRIPLE;
        flash_fw_info_addr      = PARTITION_FW3_INFO_START_ADDR;
        flash_fw_info_size      = PARTITION_FW3_INFO_SIZE;
        flash_fw_bin_addr       = PARTITION_FW3_BIN_START_ADDR;
        flash_fw_bin_size       = PARTITION_FW3_BIN_SIZE;
        flash_fw_triple_addr    = PARTITION_FW3_TRIPLE_START_ADDR;
        flash_fw_triple_size    = PARTITION_FW3_TRIPLE_SIZE;
    }
    else
    {
        #if (defined BUFF_DYNAMIC_ALLOC)
        free(p_buff);
        #endif
        return -10;
    }
    
    // 2.erase fw partition
    ret  = ftd_drv_flash_operation(en_flash_fw_info_name, OPERATION_ERASE, flash_fw_info_addr, flash_fw_info_size, NULL);
    FTD_LOG_DEBUG("erase fw info partition[%d] ret = %d", en_flash_fw_info_name, ret);
    ret |= ftd_drv_flash_operation(en_flash_fw_bin_name, OPERATION_ERASE, flash_fw_bin_addr, flash_fw_bin_size, NULL);
    FTD_LOG_DEBUG("erase fw bin partition[%d] ret = %d", en_flash_fw_bin_name, ret);
    if ((0x00 != p_st_fw_info->st_triple_info.size) && (0x00 != p_st_fw_info->st_triple_info.allow_write_counts))
    {
        ret |= ftd_drv_flash_operation(en_flash_fw_triple_name, OPERATION_ERASE, flash_fw_triple_addr, flash_fw_triple_size, NULL);
        FTD_LOG_DEBUG("erase fw triple partition[%d] ret = %d", en_flash_fw_triple_name, ret);
    }
    if(ret != 0)
    {
        #if (defined BUFF_DYNAMIC_ALLOC)
        free(p_buff);
        #endif
        return -20;
    }

    // 3.write fw_info to partition
    ret = ftd_drv_flash_operation(en_flash_fw_info_name, OPERATION_WRITE, flash_fw_info_addr, sizeof(FW_INFO), (uint8_t*)p_st_fw_info);
    FTD_LOG_DEBUG("write fw info partition[%d] ret = %d", en_flash_fw_info_name, ret);
    if(ret != 0)
    {
        #if (defined BUFF_DYNAMIC_ALLOC)
        free(p_buff);
        #endif
        return -30;
    }

    // 4.copy fw_bin to partition
    for (cache_read_length = 0; cache_read_length < p_st_fw_info->st_bin_info.size; cache_read_length += buff_size)
    {
        FTD_LOG_DEBUG("cache_read_length = 0x%08x", cache_read_length);

        if (ftd_drv_flash_operation(PARTITION_CACHE, OPERATION_READ, sizeof(FW_INFO) + cache_read_length, buff_size, p_buff) == 0) //success
        {
            // if read length over bin_size, only copy part of buffer
            if ((cache_read_length + buff_size) > p_st_fw_info->st_bin_info.size)
            {
                prog_size = p_st_fw_info->st_bin_info.size % buff_size;
                FTD_LOG_DEBUG("prog_size = 0x%08x", prog_size);
            }
            else
            {
                prog_size = buff_size;
            }

            // for debug only
            #if 0
            FTD_LOG_RAW("READ CACHE BIN[read_len:0x%08x bin_size:0x%08x]:\n", cache_read_length, p_st_fw_info->st_bin_info.size);
            FTD_LOG_DEBUG_BUFF(p_buff, buff_size, prog_size);
            #endif

            if (ftd_drv_flash_operation(en_flash_fw_bin_name, OPERATION_WRITE, cache_read_length, prog_size, p_buff) == 0) //success
            {
                // for debug only
                #if 0
                FTD_LOG_DEBUG("CHK READ %d %d offset:0x%08x size:0x%08x buff_addr:0x%08x",
                    en_flash_fw_bin_name, OPERATION_READ, cache_read_length, prog_size, p_buff);
                if (ftd_drv_flash_operation(en_flash_fw_bin_name, OPERATION_READ, cache_read_length, prog_size, p_buff) == 0) //success
                {
                    FTD_LOG_RAW("READ BIN[0x%08x]:\n", prog_size);
                    FTD_LOG_DEBUG_BUFF(p_buff, buff_size, prog_size);
                }
                #endif
            }
            else
            {
                #if (defined BUFF_DYNAMIC_ALLOC)
                free(p_buff);
                #endif
                return -40;
            }
        }
        else
        {
            FTD_LOG_INFO("CACHE READ FAIL length 0x%08x ttl_size 0x%08x", cache_read_length, p_st_fw_info->st_bin_info.size);
            #if (defined BUFF_DYNAMIC_ALLOC)
            free(p_buff);
            #endif
            return -41;
        }
    }

    // 5.copy fw_triple to partition
    if ((0x00 != p_st_fw_info->st_triple_info.size) && (0x00 != p_st_fw_info->st_triple_info.allow_write_counts))
    {
        for(cache_read_length = 0; cache_read_length < p_st_fw_info->st_triple_info.size; cache_read_length += buff_size)
        {
            if (ftd_drv_flash_operation(PARTITION_CACHE, OPERATION_READ, sizeof(FW_INFO) + p_st_fw_info->st_bin_info.size + cache_read_length, buff_size, p_buff) == 0) //success
            {
                FTD_LOG_DEBUG("triple read addr:0x%08x cache_read_length = 0x%08x", (sizeof(FW_INFO) + p_st_fw_info->st_bin_info.size + cache_read_length), cache_read_length);
                // if read length over triple_size, only copy part of buffer
                if ((cache_read_length + buff_size) > p_st_fw_info->st_triple_info.size)
                {
                    prog_size = p_st_fw_info->st_triple_info.size % buff_size;
                    FTD_LOG_DEBUG("prog_size = 0x%08x", prog_size);
                }
                else
                {
                    prog_size = buff_size;
                }

                // for debug only
                #if 0
                FTD_LOG_RAW("READ CACHE TRIPLE[read_len:0x%08x triple_size:0x%08x]:\n", cache_read_length, p_st_fw_info->st_triple_info.size);
                FTD_LOG_DEBUG_BUFF(p_buff, buff_size, prog_size);
                #endif

                if (ftd_drv_flash_operation(en_flash_fw_triple_name, OPERATION_WRITE, cache_read_length, prog_size, p_buff) == 0) //success
                {
                    // for debug only
                    #if 0
                    FTD_LOG_DEBUG("CHK READ %d %d offset:0x%08x size:0x%08x buff_addr:0x%08x",
                        en_flash_fw_triple_name, OPERATION_READ, cache_read_length, prog_size, p_buff);
                    if (ftd_drv_flash_operation(en_flash_fw_triple_name, OPERATION_READ, cache_read_length, prog_size, p_buff) == 0) //success
                    {
                        FTD_LOG_RAW("READ TRIPLE[0x%08x]:\n", prog_size);
                        FTD_LOG_DEBUG_BUFF(p_buff, buff_size, prog_size);
                    }
                    #endif
                }
                else
                {
                    #if (defined BUFF_DYNAMIC_ALLOC)
                    free(p_buff);
                    #endif
                    return -50;
                }
            }
            else
            {
                #if (defined BUFF_DYNAMIC_ALLOC)
                free(p_buff);
                #endif
                return -51;
            }
        }
    }

    // 6.check fw_bin crc16
    //ftd_utils_hw_crc16_open();
    for (cache_read_length = 0; cache_read_length < p_st_fw_info->st_bin_info.size; cache_read_length += buff_size)
    {
        FTD_LOG_TRACE("CHK READ %d %d offset:0x%08x size:0x%08x buff_addr:0x%08x",
            en_flash_fw_bin_name, OPERATION_READ, cache_read_length, buff_size, p_buff);
        if(ftd_drv_flash_operation(en_flash_fw_bin_name, OPERATION_READ, cache_read_length, buff_size, p_buff) == 0) //success
        {
            //ftd_utils_hw_crc16_update(p_buff, buff_size);

            if (p_st_fw_info->st_bin_info.size < buff_size)
                crc16_result = crc16_ccitt_update(0xFFFF, (uint8_t*)p_buff, p_st_fw_info->st_bin_info.size);
            else
            {
                if ((cache_read_length + buff_size) > p_st_fw_info->st_bin_info.size)
                {
                    FTD_LOG_TRACE("the last sector");
                    crc16_result = crc16_ccitt_update(crc16_result, (uint8_t*)p_buff, (p_st_fw_info->st_bin_info.size - cache_read_length));
                }
                else
                {
                    FTD_LOG_TRACE("the other sector");
                    crc16_result = crc16_ccitt_update(crc16_result, (uint8_t*)p_buff, buff_size);
                }
            }

            FTD_LOG_TRACE("crc16_result: 0x%08x", crc16_result);
        }
        else
        {
            #if (defined BUFF_DYNAMIC_ALLOC)
            free(p_buff);
            #endif
            return -60;
        }
    }

    //crc16_result = ftd_utils_hw_crc16_get_result();
    if(crc16_result != p_st_fw_info->st_bin_info.bin_crc16) //fwx_bin crc16 error
    {
        #if (defined BUFF_DYNAMIC_ALLOC)
        free(p_buff);
        #endif
        FTD_LOG_ERROR(" fw%02d_bin crc16 check FAIL [0x%x][0x%x]", p_st_fw_info->fw_region, crc16_result, p_st_fw_info->st_bin_info.bin_crc16);
        return -61;
    }
    FTD_LOG_INFO(" fw%02d_bin crc16 check PASS [0x%x][0x%x]", p_st_fw_info->fw_region, crc16_result, p_st_fw_info->st_bin_info.bin_crc16);
    
    // 7.check triple crc16
    if ((0x00 != p_st_fw_info->st_triple_info.size) && (0x00 != p_st_fw_info->st_triple_info.allow_write_counts))
    {
        if ((0 != (p_st_fw_info->st_triple_info.size % TRIPLE_DATA_LEN))
            || (p_st_fw_info->st_triple_info.allow_write_counts != (p_st_fw_info->st_triple_info.size / TRIPLE_DATA_LEN)))
        {
            FTD_LOG_ERROR(" fw%02d_triple_info size error[%d] ttl_write_cnt[%d] triple_len[%d]", 
                p_st_fw_info->fw_region, p_st_fw_info->st_triple_info.size, p_st_fw_info->st_triple_info.allow_write_counts, TRIPLE_DATA_LEN);
            #if (defined BUFF_DYNAMIC_ALLOC)
            free(p_buff);
            #endif
            return -70;
        }
        //ftd_utils_hw_crc16_open();
        crc16_result = 0xFFFF;
        for(cache_read_length = 0; cache_read_length < p_st_fw_info->st_triple_info.size; cache_read_length += buff_size)
        {
            if(ftd_drv_flash_operation(en_flash_fw_triple_name, OPERATION_READ, cache_read_length, buff_size, p_buff) == 0) //success
            {
                //ftd_utils_hw_crc16_update(p_buff, buff_size);

                if (p_st_fw_info->st_triple_info.size < buff_size)
                    crc16_result = crc16_ccitt_update(crc16_result, (uint8_t*)p_buff, p_st_fw_info->st_triple_info.size);
                else
                {
                    if ((cache_read_length + buff_size) > p_st_fw_info->st_triple_info.size)
                    {
                        FTD_LOG_TRACE("the last sector");
                        crc16_result = crc16_ccitt_update(crc16_result, (uint8_t*)p_buff, (p_st_fw_info->st_triple_info.size - cache_read_length));
                    }
                    else
                    {
                        FTD_LOG_TRACE("the other sector");
                        crc16_result = crc16_ccitt_update(crc16_result, (uint8_t*)p_buff, buff_size);
                    }
                }

                FTD_LOG_TRACE("crc16_result: 0x%08x", crc16_result);
            }
            else
            {
                #if (defined BUFF_DYNAMIC_ALLOC)
                free(p_buff);
                #endif
                return -71;
            }
        }

        //crc16_result = ftd_utils_hw_crc16_get_result();
        if(crc16_result != p_st_fw_info->st_triple_info.triple_crc16) //fwx_triple crc16 error
        {
            #if (defined BUFF_DYNAMIC_ALLOC)
            free(p_buff);
            #endif
            FTD_LOG_ERROR(" fw%02d_triple crc16 check FAIL [0x%x][0x%x]", p_st_fw_info->fw_region, crc16_result, p_st_fw_info->st_triple_info.triple_crc16);
            return -72;
        }
        FTD_LOG_INFO(" fw%02d_triple crc16 check PASS [0x%x][0x%x]", p_st_fw_info->fw_region, crc16_result, p_st_fw_info->st_triple_info.triple_crc16);
    }

    // 8.write fw_info to partition
    p_st_fw_info->fw_active = 1;
    crc16_result = ftd_utils_hw_crc16((uint8_t *)p_st_fw_info, sizeof(FW_INFO));
    p_st_fw_info->fw_info_crc16 = crc16_result;
    if(ftd_drv_flash_operation(en_flash_fw_info_name, OPERATION_WRITE, 0, sizeof(FW_INFO), (uint8_t *)p_st_fw_info) < 0) //failed
    {
        #if (defined BUFF_DYNAMIC_ALLOC)
        free(p_buff);
        #endif
        return -80;
    }

    // 9. update sys_info
    SYS_INFO        st_sys_info;
    ftd_mw_sys_info_manager_get(&st_sys_info);
    st_sys_info.st_fw_info[p_st_fw_info->fw_region - 1].deploy_flag = 1;
    memcpy((uint8_t*)&st_sys_info.st_fw_info[p_st_fw_info->fw_region - 1], (uint8_t*)p_st_fw_info, sizeof(FW_INFO));
    ftd_mw_sys_info_manager_set(&st_sys_info);
    if (ftd_mw_sys_info_manager_save_nv())
        return -90;

    #if (defined BUFF_DYNAMIC_ALLOC)
    free(p_buff);
    #endif
    return 0;
}



