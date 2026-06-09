#ifndef _FTD_MW_NAND_FLASH_MANAGER_H
#define _FTD_MW_NAND_FLASH_MANAGER_H

#include "stdio.h"
#include "ftd_drv_nand_flash_qspi.h"

/*
Some brief explanations about W25N01KV:
This Flash is a NAND Flash. Some instructions are different from those in the W25Q series (W25Q is a NOR Flash).
This product has an 2048-byte ID page, a 2048-byte parameter page, and 10 2048-byte OTP (One Time Program) pages.
The total chip capacity is 128MB, with 2048+64 bytes per page, a total of 65536 pages. It can perform full-page programming using a 2048-byte input buffer.
There is a bad block management function. If ECC is enabled, each page is 2048 bytes, with the last 64 bytes used to store check information; otherwise, each page is 2112 bytes.
The erase block size is 128KB, with a total of 1024 erasable blocks.
The maximum SPI clock rate is 104 MHz.
*/

#define FTD_MW_W25N01KV_FLASH_ID 0x00EFAE21 // EF represents the chip manufacturer Winbond, and AE represents the chip model W25N01KV.

// Status Register Indicators
#define FTD_MW_W25N01KV_SR_BUSY                         0x01// Chip busy, performing write or erase operations
#define FTD_MW_W25N01KV_SR_WEL                          0x02// Write enable flag
#define FTD_MW_W25N01KV_SR_ERASE_FAIL                   0x04// Erase failure
#define FTD_MW_W25N01KV_SR_WRITE_FAIL                   0x08// Programming (write) failure
#define FTD_MW_W25N01KV_SR_ECC_SUCCESS                  0x00// ECC check passed
#define FTD_MW_W25N01KV_SR_ECC_LESS_THAN_4BIT_ERR       0x10// ECC check has 1-4bit errors, corrected
#define FTD_MW_W25N01KV_SR_ECC_MORE_THAN_4BIT_ERR       0x20// ECC check has more than 4bit errors, cannot be corrected
#define FTD_MW_W25N01KV_SR_ECC_DATA_DAMAGED             0x30// Every page has more than 4bit errors, not recommended for continued use
#define FTD_MW_W25N01KV_SR_BBM_LUT_FULL                 0x40// Bad block management table is full

// Bad Block Link Status
#define FLASH_LINK_STATUS_MASK                          0xC000// Bad block link status bit mask
#define FLASH_LINK_AVAILABLE                            0x0000// Link can be used
#define FLASH_LINK_VALID                                0x8000// The link is currently in use and valid
#define FLASH_LINK_NOT_VALID                            0xC000// The link has been used but is now invalid
#define FLASH_LINK_NOT_APPLICABLE                       0x4000// The link cannot be used

// Commands
#define FTD_MW_W25N01KV_CMD_DEVICE_RESET              0xFF
#define FTD_MW_W25N01KV_CMD_JEDEC_ID                  0x9F
#define FTD_MW_W25N01KV_CMD_READ_STATUS_REGISTER      0x0F
#define FTD_MW_W25N01KV_CMD_WRITE_STATUS_REGISTER     0x1F
#define FTD_MW_W25N01KV_CMD_STATUS_REG1_ADDR          0xA0
#define FTD_MW_W25N01KV_CMD_STATUS_REG2_ADDR          0xB0
#define FTD_MW_W25N01KV_CMD_STATUS_REG3_ADDR          0xC0
#define FTD_MW_W25N01KV_CMD_WRITE_ENABLE              0x06
#define FTD_MW_W25N01KV_CMD_WRITE_DISABLE             0x04
#define FTD_MW_W25N01KV_CMD_BAD_BLOCK_MANAGEMENT      0xA1
#define FTD_MW_W25N01KV_CMD_READ_BBM_LUT              0xA5
#define FTD_MW_W25N01KV_CMD_LAST_ECC_FAILURE_PAGE     0xA9
#define FTD_MW_W25N01KV_CMD_BLOCK_ERASE               0xD8
#define FTD_MW_W25N01KV_CMD_PROGRAM_DATA_LOAD         0x02
#define FTD_MW_W25N01KV_CMD_RANDOM_PROGRAM_DATA_LOAD  0x84
#define FTD_MW_W25N01KV_CMD_QUAD_PROGRAM_DATA_LOAD    0x32
#define FTD_MW_W25N01KV_CMD_RANDOM_QUAD_PROGRAM_LOAD  0x34
#define FTD_MW_W25N01KV_CMD_PROGRAM_EXECUTE           0x10
#define FTD_MW_W25N01KV_CMD_PAGE_DATA_READ            0x13
#define FTD_MW_W25N01KV_CMD_READ                      0x03
#define FTD_MW_W25N01KV_CMD_FAST_READ                 0x0B
#define FTD_MW_W25N01KV_CMD_FAST_READ_WITH_4BYTE_ADDR 0x0C
#define FTD_MW_W25N01KV_CMD_FAST_READ_DUAL_OUTPUT     0x3B
#define FTD_MW_W25N01KV_CMD_FAST_READ_DUAL_OUTPUT_4B  0x3C
#define FTD_MW_W25N01KV_CMD_FAST_READ_QUAD_OUTPUT     0x6B
#define FTD_MW_W25N01KV_CMD_FAST_READ_QUAD_OUTPUT_4B  0x6C
#define FTD_MW_W25N01KV_CMD_FAST_READ_DUAL_IO         0xBB
#define FTD_MW_W25N01KV_CMD_FAST_READ_DUAL_IO_4B      0xBC
#define FTD_MW_W25N01KV_CMD_FAST_READ_QUAD_IO         0xEB
#define FTD_MW_W25N01KV_CMD_FAST_READ_QUAD_IO_4B      0xEC

unsigned char ftd_mw_w25n01kv_reset(void);
unsigned char ftd_mw_w25n01kv_init(void);
unsigned char ftd_mw_w25n01kv_wait_busy(unsigned short int timeout);
void ftd_mw_w25n01kv_write_enable(void);
void ftd_mw_w25n01kv_write_disable(void);
unsigned char ftd_mw_w25n01kv_read_status_register(unsigned char RegAddr);
void ftd_mw_w25n01kv_write_status_register(unsigned char RegAddr, unsigned char Value);
unsigned char ftd_mw_w25n01kv_read_page(unsigned short int page, unsigned char* buf);
unsigned char ftd_mw_w25n01kv_write_page(unsigned short int page, unsigned char* buf);
unsigned char ftd_mw_w25n01kv_block_erase(unsigned short int BlockNum);
unsigned char ftd_mw_w25n01kv_block_scan(unsigned short int BlockNum);
unsigned char ftd_mw_w25n01kv_bad_block_scan(unsigned short int BlockNum);
unsigned char ftd_mw_w25n01kv_bad_block_mapping(unsigned short int LBA, unsigned short int PBA);
void ftd_mw_w25n01kv_read_bbm_lut(unsigned short int* BBMLUT);
unsigned short int ftd_mw_w25n01kv_last_ecc_fail_page_addr(void);
void ftd_mw_w25n01kv_bad_block_manage(void);
void ftd_mw_w25n01kv_test(void);

#endif
