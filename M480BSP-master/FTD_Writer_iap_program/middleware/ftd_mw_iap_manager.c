#include "ftd_mw_iap_manager.h"
#include "ftd_mw_log_manager.h"
#include "ftd_drv_iap_fmc.h"
#include "ftd_utils.h"
#include "string.h"




uint32_t time_cnt = 0;
IAP_FMC_T g_iap_fmc;

void ftd_mw_iap_manager_fmc_data_init(void)
{
    g_iap_fmc.start_base = APP_2_START_BASE;
    g_iap_fmc.current_offset = 0;
    g_iap_fmc.page_offset = 0;
    g_iap_fmc.page_dirty = 0;
    g_iap_fmc.crc16 = 0xFFFF;

    memset(g_iap_fmc.page_buf, 0xFF, FLASH_PAGE_SIZE);
    g_iap_fmc.cur_page_addr = APP_2_START_BASE;
}


void ftd_mw_iap_manager_init(void)
{
    ftd_drv_iap_fmc_init();

    ftd_mw_iap_manager_fmc_data_init();
}


void ftd_mw_iap_manager_deinit(void)
{
    ftd_drv_iap_fmc_deinit();

    ftd_mw_iap_manager_fmc_data_init();
}

uint32_t ftd_mw_iap_manager_get_time_ms(void)
{
    return time_cnt;
}

void ftd_mw_iap_manager_set_time_ms(uint32_t time)
{
    time_cnt = time;
}

void ftd_mw_iap_manager_set_upgrade_flag(uint8_t flag)
{
    g_iap_fmc.upgrade_flag = flag;
}

uint8_t ftd_mw_iap_manager_get_upgrade_flag(void)
{
    return g_iap_fmc.upgrade_flag;
}

/* Write data to Flash with verify + rollback retry */
int ftd_mw_iap_manager_write(uint32_t total_len, uint8_t* data, uint32_t len)
{

    uint32_t copy_len, rdata;

    if (total_len > APP_2_SIZE)
        return -1;
    while (len > 0)
    {
        copy_len = FLASH_PAGE_SIZE - g_iap_fmc.page_offset;
        if (copy_len > len) copy_len = len;

        memcpy(&g_iap_fmc.page_buf[g_iap_fmc.page_offset], data, copy_len);

        g_iap_fmc.page_offset += copy_len;
        data += copy_len;
        len -= copy_len;
        g_iap_fmc.page_dirty = 1;

        if (g_iap_fmc.page_offset >= FLASH_PAGE_SIZE)
        {
            if (g_iap_fmc.cur_page_addr + FLASH_PAGE_SIZE > APP_2_END_BASE) // overflow
            {
                return -3;
            }
            for (int retry = 0; retry < ROLLBACK_REWRITE_COUNT; retry++)
            {
                ftd_drv_iap_fmc_erase(g_iap_fmc.cur_page_addr);

                int ok = 1;
                for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i += 4)
                {
                    uint32_t w = *(uint32_t*)&g_iap_fmc.page_buf[i];
                    ftd_drv_iap_fmc_write_4bit(g_iap_fmc.cur_page_addr + i, w);

                    rdata = ftd_drv_iap_fmc_read_4bit(g_iap_fmc.cur_page_addr + i);
                    if (rdata != w)
                    {
                        ok = 0;
                        break;
                    }
                }
                if (ok)
                {
                    g_iap_fmc.crc16 = crc16_ccitt_update(g_iap_fmc.crc16, g_iap_fmc.page_buf, FLASH_PAGE_SIZE);
                    break;          /* success */
                }
                if (retry == ROLLBACK_REWRITE_COUNT - 1)
                {
                    return -2; /* fail */
                }
            }

            g_iap_fmc.cur_page_addr += FLASH_PAGE_SIZE;
            g_iap_fmc.page_offset = 0;
            g_iap_fmc.page_dirty = 0;

            memset(g_iap_fmc.page_buf, 0xFF, FLASH_PAGE_SIZE);
        }
    }
    return 0;
}


/* Finish the IAP process */
int ftd_mw_iap_manager_finish(void)
{
    if (g_iap_fmc.page_dirty && g_iap_fmc.page_offset > 0)
    {
        memset(&g_iap_fmc.page_buf[g_iap_fmc.page_offset], 0xFF,
            FLASH_PAGE_SIZE - g_iap_fmc.page_offset);
        if (g_iap_fmc.cur_page_addr + FLASH_PAGE_SIZE > APP_2_END_BASE) // overflow
            return -3;

        for (int retry = 0; retry < ROLLBACK_REWRITE_COUNT; retry++)
        {
            ftd_drv_iap_fmc_erase(g_iap_fmc.cur_page_addr);

            int ok = 1;
            for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i += 4)
            {
                uint32_t w = *(uint32_t*)&g_iap_fmc.page_buf[i];
                ftd_drv_iap_fmc_write_4bit(g_iap_fmc.cur_page_addr + i, w);

                uint32_t rdata = ftd_drv_iap_fmc_read_4bit(g_iap_fmc.cur_page_addr + i);
                if (rdata != w)
                {
                    ok = 0;
                    break;
                }
            }

            if (ok)
            {
                g_iap_fmc.crc16 = crc16_ccitt_update(g_iap_fmc.crc16, g_iap_fmc.page_buf, g_iap_fmc.page_offset);
                break;          /* success */
            }
            if (retry == ROLLBACK_REWRITE_COUNT - 1) return -2; /* fail */
        }
        g_iap_fmc.page_dirty = 0;
    }


    return 0;
}

bool ftd_mw_iap_manager_check_addr_valid(uint32_t app_base)
{
    uint32_t sp = *(uint32_t*)(app_base);
    uint32_t pc = *(uint32_t*)(app_base + 4);

    /* MSP must be in SRAM */
    if ((sp & 0x2FFE0000) != 0x20000000)
        return false;

    /* PC must be in Flash */
    if (app_base == APP_1_START_BASE)
    {
        if (pc < APP_1_START_BASE || pc >= APP_1_END_BASE)
            return false;
    }
    else if (app_base == APP_2_START_BASE)
    {
        if (pc < APP_2_START_BASE || pc >= APP_2_END_BASE)
            return false;
    }

    return true;
}

uint16_t ftd_mw_iap_manager_get_crc16(void)
{
    return g_iap_fmc.crc16;
}

void ftd_mw_iap_manager_set_crc16(uint16_t crc16)
{
    g_iap_fmc.crc16 = crc16;
}

void ftd_mw_iap_manager_flag_load(IAP_FLAG_T* flag)
{
    uint32_t* p = (uint32_t*)flag;
    uint32_t words = sizeof(IAP_FLAG_T) / 4;
    for (uint32_t i = 0; i < words; i++)
    {
        p[i] = ftd_drv_iap_fmc_read_4bit(
            IAP_FLAG_START_BASE + i * 4);
    }
}


int ftd_mw_iap_manager_flag_save(IAP_FLAG_T flag)
{
    IAP_FLAG_T old;
    ftd_mw_iap_manager_flag_load(&old);

    if (memcmp(&old, &flag, sizeof(IAP_FLAG_T)) == 0)
        return 0;

    for (int retry = 0; retry < ROLLBACK_REWRITE_COUNT; retry++)
    {
        int ok = 1;
        ftd_drv_iap_fmc_erase(IAP_FLAG_START_BASE);

        for (uint32_t i = 0; i < sizeof(IAP_FLAG_T) / 4; i++)
        {
            uint32_t w = ((uint32_t*)&flag)[i];
            ftd_drv_iap_fmc_write_4bit(IAP_FLAG_START_BASE + i * 4, w);

            if (ftd_drv_iap_fmc_read_4bit(IAP_FLAG_START_BASE + i * 4) != w)
            {
                ok = 0;
                break;
            }
        }

        if (ok)
        {
            return 0;
        }

    }

    /* rollback */
    ftd_drv_iap_fmc_erase(IAP_FLAG_START_BASE);
    for (uint32_t i = 0; i < sizeof(IAP_FLAG_T) / 4; i++)
    {
        uint32_t w = ((uint32_t*)&old)[i];
        ftd_drv_iap_fmc_write_4bit(IAP_FLAG_START_BASE + i * 4, w);
    }
    return -2;
}

void ftd_mw_iap_manager_erase_app_2(void)
{
    for (uint32_t i = APP_2_START_BASE; i < APP_2_END_BASE; i += FLASH_PAGE_SIZE)
        ftd_drv_iap_fmc_erase(i);
}

void ftd_mw_iap_manager_reset(void)
{
    __disable_irq();

    SYS_UnlockReg();
    FMC_Open();

    FMC_SetVectorPageAddr(IAP_PROGRAM_START_BASE);
    FMC_Close();
    SYS_LockReg();
    NVIC_SystemReset();
    while (1);

}

typedef void (FUNC_PTR)(void);

/*
 *  Set stack base address to SP register.
 */
#ifdef __ARMCC_VERSION                 /* for Keil compiler */
void __set_SP(uint32_t _sp)
{
    __set_MSP(_sp);
}
#endif

void ftd_mw_iap_manager_jum_to_app(uint32_t app_start_addr)
{
    SYS_UnlockReg();
    JYMC_LOG_INFO("jump to app addr: %08X\r\n", app_start_addr);
    FUNC_PTR* func;                 /* function pointer */
    /*
*  The reset handler address of an executable image is located at offset 0x4.
*  Thus, this sample get reset handler address of LDROM code from FMC_LDROM_BASE + 0x4.
*/
    FMC_SetVectorPageAddr(app_start_addr);
    func = (FUNC_PTR*)*(uint32_t*)(app_start_addr + 4);
    /*
     *  The stack base address of an executable image is located at offset 0x0.
     *  Thus, this sample get stack base address of LDROM code from FMC_LDROM_BASE + 0x0.
     */
#if defined (__GNUC__) && !defined(__ARMCC_VERSION)  /* for GNU C compiler */
    u32Data = *(uint32_t*)app_start_addr;
    asm("msr msp, %0" : : "r" (u32Data));
#else
    __set_SP(*(uint32_t*)app_start_addr);
#endif
    /*
     *  Branch to the LDROM code's reset handler in way of function call.
     */
    SYS_LockReg();
    ftd_mw_iap_manager_deinit();
    func();
}

