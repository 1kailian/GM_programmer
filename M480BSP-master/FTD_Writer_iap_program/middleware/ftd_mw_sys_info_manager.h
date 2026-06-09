#ifndef __FTD_MW_SYS_INFO_MANAGER_H__
#define __FTD_MW_SYS_INFO_MANAGER_H__ 

#include "ftd_data_model.h"
#include <stdint.h>

int8_t ftd_mw_sys_info_manager_save_nv(void);
void   ftd_mw_sys_info_manager_get(SYS_INFO *p_st_sys_info);
void   ftd_mw_sys_info_manager_set(SYS_INFO *p_st_sys_info);
int8_t ftd_mw_sys_info_manager_init(void);
void   ftd_mw_sys_info_manager_reverse_endian_writer_info(WRITER_INFO* p_writer_info);
void   ftd_mw_sys_info_manager_reverse_endian_fw_info(FW_INFO* p_fw_info);
void   ftd_mw_sys_info_manager_reverse_endian_sys_info(SYS_INFO* p_sys_info);
/************************************ for debug ************************************/
void   ftd_mw_sys_info_manager_writer_info_dump(WRITER_INFO* p_writer_info);
void   ftd_mw_sys_info_manager_fw_info_dump(FW_INFO* p_fw_info);
void   ftd_mw_sys_info_manager_sys_info_dump(SYS_INFO* p_sys_info);
#endif 
