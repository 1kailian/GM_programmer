#include "ftd_drv_iap_fmc.h"
#include "ftd_mw_log_manager.h"



void ftd_drv_iap_fmc_init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();
    FMC_Open();

    FMC_ENABLE_AP_UPDATE();
    /* Lock protected registers */
    SYS_LockReg();
}

/* Deinitialize the IAP process */
void ftd_drv_iap_fmc_deinit(void)
{
    SYS_UnlockReg();
    FMC_DISABLE_AP_UPDATE();
    FMC_Close();
    /* Lock protected registers */
    SYS_LockReg();
}

void ftd_drv_iap_fmc_erase(uint32_t addr)
{
    SYS_UnlockReg();
    FMC_Erase(addr);
    SYS_LockReg();
}

void ftd_drv_iap_fmc_write_4bit(uint32_t u32Addr, uint32_t u32Data)
{
    SYS_UnlockReg();
    FMC_Write(u32Addr, u32Data);
    SYS_LockReg();

}

uint32_t ftd_drv_iap_fmc_read_4bit(uint32_t u32Addr)
{
    SYS_UnlockReg();
    uint32_t u32Data = FMC_Read(u32Addr);
    SYS_LockReg();
    return u32Data;
}
