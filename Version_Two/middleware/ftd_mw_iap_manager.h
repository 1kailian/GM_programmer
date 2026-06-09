#ifndef FTD_MW_IAP_MANAGER_H
#define FTD_MW_IAP_MANAGER_H


#include "ftd_drv_iap_fmc.h"

//If the bootloader fails to implement the IAP function, then the IAP program will be stored here.
#define IAP_PROGRAM_START_BASE 0x00000
#define IAP_PROGRAM_END_BASE   0x05000 //iap progame size : 0x5000 (20kb)

#define APP_1_START_BASE 0x05000
#define APP_1_END_BASE   0x40000 //app1 size : 0x40000 (256kb)


#define APP_2_START_BASE 0x40000
#define APP_2_END_BASE   0x70000 //app2 size : 0x30000 (192kb)
#define APP_2_SIZE       (APP_2_END_BASE - APP_2_START_BASE)

#define IAP_FLAG_START_BASE 0x7F000
#define IAP_FLAG_END_BASE   0x80000 //iap flag size : 0x1000 (4kb)


#define TEST_BASE 0x70000

#define FLASH_PAGE_SIZE   4096   
#define ROLLBACK_REWRITE_COUNT  2


typedef struct
{
    uint32_t start_base;          // start address  
    uint32_t current_offset;      // current write offset
    uint8_t  page_buf[FLASH_PAGE_SIZE];
    uint32_t page_offset;         // current page filled byte count
    uint32_t cur_page_addr;       // current page Flash address
    uint8_t  page_dirty;          // is there any unwritten data
    uint32_t crc16;               // CRC16 value
} IAP_FMC_T;


#define IAP_MAGIC  0x4A594D43  // 'JYMC'
#define CHECK_LEN_BYTES   500U

#ifdef __ICCARM__
#pragma data_alignment=4
typedef struct
{
    uint32_t start_magic;  // fixed magic number, used to check if valid
    uint32_t upgrade;      // 0 = no upgrade, 1 = upgrading
    uint32_t app_select;    //1 = APP1  2 = APP2
} IAP_FLAG_T;
#else
__attribute__((aligned(4)))
typedef struct
{
    uint32_t start_magic;  // fixed magic number, used to check if valid
    uint32_t upgrade;      // 0 = no upgrade, 1 = upgrading
    uint32_t app_select;    //1 = APP1  2 = APP2

} IAP_FLAG_T;
#endif


void ftd_mw_iap_manager_init(void);
void ftd_mw_iap_manager_deinit(void);
int ftd_mw_iap_manager_write(uint32_t total_len, uint8_t* data, uint32_t len);
int ftd_mw_iap_manager_finish(void);
int ftd_mw_iap_check_addr_valid(uint32_t addr);
uint16_t ftd_mw_iap_manager_get_crc16(void);

void ftd_mw_iap_manager_flag_load(IAP_FLAG_T* flag);
int ftd_mw_iap_manager_flag_save(IAP_FLAG_T flag);

void ftd_mw_iap_manager_jum_to_app(uint32_t app_start_addr);

void ftd_mw_iap_manager_reset(void);

void ftd_mw_iap_manager_test(void);


#endif
