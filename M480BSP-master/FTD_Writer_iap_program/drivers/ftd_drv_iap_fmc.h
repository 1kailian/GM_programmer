#ifndef __FTD_DRV_IAP_FMC_H__
#define __FTD_DRV_IAP_FMC_H__

#include "NuMicro.h"

void ftd_drv_iap_fmc_init(void);
void ftd_drv_iap_fmc_deinit(void);
void ftd_drv_iap_fmc_erase(uint32_t addr);
void ftd_drv_iap_fmc_write_4bit(uint32_t u32Addr, uint32_t u32Data);
uint32_t ftd_drv_iap_fmc_read_4bit(uint32_t u32Addr);
void ftd_drv_iap_fmc_write_multiple(uint32_t addr, const uint8_t* buf, uint32_t len);
void ftd_drv_iap_fmc_read_multiple(uint32_t addr, uint8_t* buf, uint32_t len);


#endif
