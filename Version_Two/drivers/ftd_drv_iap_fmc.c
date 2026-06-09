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
    FTD_LOG_DEBUG("ERASE addr=0x%08X\r\n", addr);
    FMC_Erase(addr);
    SYS_LockReg();
}

void ftd_drv_iap_fmc_write_4bit(uint32_t u32Addr, uint32_t u32Data)
{
    SYS_UnlockReg();
    FTD_LOG_DEBUG("WRITE addr=0x%08X val=0x%08X\r\n", u32Addr, u32Data);
    FMC_Write(u32Addr, u32Data);
    SYS_LockReg();

}

uint32_t ftd_drv_iap_fmc_read_4bit(uint32_t u32Addr)
{
    SYS_UnlockReg();
    uint32_t u32Data = FMC_Read(u32Addr);
    SYS_LockReg();
    FTD_LOG_DEBUG("READ addr=0x%08X val=0x%08X\r\n", u32Addr, u32Data);
    return u32Data;
}

uint8_t ftd_drv_fmc_read_data(uint32_t base_addr, uint32_t offset, uint8_t* buf, uint32_t len)
{
    SYS_UnlockReg();
    FMC_Open();

    uint32_t addr = base_addr + offset;
    uint32_t i = 0;

    // Handle unaligned start address (process byte by byte until aligned)
    while (i < len && (addr & 0x3))
    {
        uint32_t word_addr = addr & ~0x3;
        uint32_t byte_offset = addr & 0x3;
        uint32_t data = FMC_Read(word_addr);
        buf[i] = (data >> (byte_offset * 8)) & 0xFF;
        addr++;
        i++;
    }

    // Read 32-bit words (4 bytes at a time)
    while (i + 4 <= len)
    {
        uint32_t data = FMC_Read(addr);
        buf[i++] = data & 0xFF;
        buf[i++] = (data >> 8) & 0xFF;
        buf[i++] = (data >> 16) & 0xFF;
        buf[i++] = (data >> 24) & 0xFF;
        addr += 4;
    }

    // Handle remaining bytes
    while (i < len)
    {
        uint32_t word_addr = addr & ~0x3;
        uint32_t byte_offset = addr & 0x3;
        uint32_t data = FMC_Read(word_addr);
        buf[i] = (data >> (byte_offset * 8)) & 0xFF;
        addr++;
        i++;
    }

    FMC_Close();
    SYS_LockReg();
    return 0;
}
