#ifndef __FTD_MW_RING_BUFFER_H__
#define __FTD_MW_RING_BUFFER_H__

#include "NuMicro.h"
#include <string.h>
#include "ftd_data_model.h"

// Remain counts structure for single FW_INFO
typedef struct {
    uint32_t bin_remain_counts;      // BIN_INFO's remain_counts
    uint32_t triple_remain_counts;   // TRIPLE_INFO's remain_counts
    uint32_t rollcode_remain_counts; // ROLLCODE_INFO's remain_counts
} FTD_MW_RING_BUFFER_FW_INFO;


// Structure for storing remain_counts
typedef struct {
    uint32_t version;             // Version number
    FTD_MW_RING_BUFFER_FW_INFO fw_info[SUPPORT_FW_NUM]; // Supported FW_INFO array
    uint8_t reserved[2];         // Reserved field for future extension
    uint16_t crc16;              // Data check CRC16
} FTD_MW_RING_BUFFER_REMAIN_COUNTS;

// ========== User Configuration ==========
#define FTD_MW_RING_BUFFER_SECTOR_SIZE         4096            // 4KB
#define FTD_MW_RING_BUFFER_DATA_SIZE           (sizeof(FTD_MW_RING_BUFFER_REMAIN_COUNTS))              // 44 bytes per write (4-byte version + 40-byte data)
#define FTD_MW_RING_BUFFER_SLOTS_PER_SECTOR    (FTD_MW_RING_BUFFER_SECTOR_SIZE / FTD_MW_RING_BUFFER_DATA_SIZE)  // 93 times

// DataFlash configuration (set according to ICP tool)
#define FTD_MW_RING_BUFFER_DATAFLASH_BASE      0x00004000      // DataFlash start address
#define FTD_MW_RING_BUFFER_SECTOR_COUNT        1               // Only 1 sector

// Write interval: actual write only after N calls to Write
#define FTD_MW_RING_BUFFER_WRITE_INTERVAL      50              // Write once every 50 updates

// ========== API Functions ==========
void ftd_mw_ring_buffer_init(void);                                    // Initialize
void ftd_mw_ring_buffer_update(FTD_MW_RING_BUFFER_REMAIN_COUNTS* pData); // Update data (may not write)
void ftd_mw_ring_buffer_force_write(void);                             // Force immediate write
uint8_t ftd_mw_ring_buffer_read(FTD_MW_RING_BUFFER_REMAIN_COUNTS* pData); // Read latest data, return 1=success, 0=fail
void ftd_mw_ring_buffer_update_remain_counts(SYS_INFO* p_sys_info);    // Update remain_counts from SYS_INFO
void ftd_mw_ring_buffer_force_update_remain_counts(SYS_INFO* p_sys_info); // Update remain_counts from SYS_INFO (direct write)

// ========== Write Protection Interface ==========
void ftd_mw_ring_buffer_enable_write_protect(void);                   // Enable write protection
void ftd_mw_ring_buffer_disable_write_protect(void);                  // Disable write protection
uint8_t ftd_mw_ring_buffer_is_write_protected(void);                   // Get write protection status


int8_t ftd_mw_ring_buffer_sync_to_sys_info(void);                   // Sync ring buffer to sys_info
#endif
