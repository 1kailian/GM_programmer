#ifndef __FTD_MW_CACHE_MANAGER_H__
#define __FTD_MW_CACHE_MANAGER_H__ 

#include "ftd_data_model.h"
#include <stdint.h>

int8_t ftd_mw_cache_manager_read(uint8_t *buff, uint32_t total_length, uint32_t cur_length, uint32_t had_read_length);
int8_t ftd_mw_cache_manager_write(uint8_t *buff, uint32_t total_length, uint32_t cur_length, uint32_t written_length);
int8_t ftd_mw_cache_manager_get_fw_info(FW_INFO *p_st_fw_info);

#endif 
