#ifndef __FTD_MW_FW_MANAGER_H__
#define __FTD_MW_FW_MANAGER_H__ 

#include "ftd_data_model.h"
#include <stdint.h>

int8_t ftd_mw_fw_manager_read_fw_info(uint8_t fw_region, FW_INFO *p_st_fw_info);
int8_t ftd_mw_fw_manager_deploy(FW_INFO *p_st_fw_info);

#endif 
