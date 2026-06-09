#include "ftd_mw_ring_buffer.h"
#include "ftd_mw_log_manager.h"
#include "ftd_drv_iap_fmc.h"
#include "ftd_utils.h"
#include "ftd_mw_sys_info_manager.h"
#include "ftd_drv_bod.h"

// ========== Internal Structure ==========
typedef struct {
    // Flash management
    uint32_t current_slot;          // Current slot (0-101)
    uint32_t write_count;           // Total write count (version number)
    uint32_t last_valid_addr;       // Last valid data address

    // Write interval control
    uint32_t pending_count;          // Pending write count
    FTD_MW_RING_BUFFER_REMAIN_COUNTS pending_data; // Pending data to write

    // Bad block flag
    uint8_t  sector_bad;            // Whether sector is bad

    // Write protection flag
    uint8_t  write_protected;        // 1=write/erase disabled, 0=allowed
} ftd_mw_ring_buffer_ctx_t;

static ftd_mw_ring_buffer_ctx_t g_ftd_mw_ring_buffer_ctx;

// ========== FMC Low-level Operations (with write protection check) ==========

// Erase a sector (with bad block detection and write protection)
static uint8_t ftd_mw_ring_buffer_erase_sector(uint32_t addr)
{
    // Write protection check
    if (g_ftd_mw_ring_buffer_ctx.write_protected) {
        return 0;  // Write protection enabled, erase forbidden
    }

    // Address must be 4KB aligned
    if (addr & 0xFFF) return 0;

    // Sync latest data to system info before erase to prevent data loss
    if (g_ftd_mw_ring_buffer_ctx.write_count > 0) {
        ftd_mw_ring_buffer_sync_to_sys_info();
    }

    // Temporarily unlock write protection register
    SYS_UnlockReg();

    FMC_Erase(addr);


    // Verify erase success (check if first word is 0xFFFFFFFF)
    if (FMC_Read(addr) != 0xFFFFFFFF) {
        g_ftd_mw_ring_buffer_ctx.sector_bad = 1;
        return 0;
    }

    return 1;  // Success
}

// Write a word (with write protection check)
static uint8_t ftd_mw_ring_buffer_write_word(uint32_t addr, uint32_t data)
{
    // Write protection check
    if (g_ftd_mw_ring_buffer_ctx.write_protected) {
        return 0;  // Write protection enabled, write forbidden
    }

    if (addr & 0x3) return 0;


    ftd_drv_iap_fmc_write_4bit(addr, data);
    return 1;
}

// Write data (with write protection check)
static uint8_t ftd_mw_ring_buffer_write_data(uint32_t addr, uint8_t* pData)
{
    // Write protection check (quick return)
    if (g_ftd_mw_ring_buffer_ctx.write_protected) {
        return 0;
    }

    uint32_t i;
    uint32_t* pSrc = (uint32_t*)pData;
    uint32_t word_count = FTD_MW_RING_BUFFER_DATA_SIZE / 4;  // 10 words

    for (i = 0; i < word_count; i++) {
        if (!ftd_mw_ring_buffer_write_word(addr + i * 4, pSrc[i])) {
            return 0;
        }
    }

    return 1;
}

// ========== Internal Functions ==========

// Scan sector to find latest data
static void ftd_mw_ring_buffer_scan_and_recover(void)
{
    uint32_t j;
    uint32_t addr;
    uint32_t latest_version = 0;
    uint32_t latest_addr = 0;
    uint32_t latest_slot = 0;

    // Process only one sector
    if (!g_ftd_mw_ring_buffer_ctx.sector_bad) {
        uint32_t sector_addr = FTD_MW_RING_BUFFER_DATAFLASH_BASE;

        // Temporarily unlock write protection register
        SYS_UnlockReg();

        for (j = 0; j < FTD_MW_RING_BUFFER_SLOTS_PER_SECTOR; j++) {
            addr = sector_addr + j * FTD_MW_RING_BUFFER_DATA_SIZE;
            uint32_t version = ftd_drv_iap_fmc_read_4bit(addr);
            FTD_LOG_DEBUG("version: %x, addr: %x, slot: %d\n", version, addr, j);
            if (version != 0xFFFFFFFF && version != 0)
            {
                // Read complete data for verification
                FTD_MW_RING_BUFFER_REMAIN_COUNTS data;
                uint32_t* pData = (uint32_t*)&data;

                // Read complete structure data
                for (int i = 0; i < sizeof(FTD_MW_RING_BUFFER_REMAIN_COUNTS) / 4; i++) {
                    pData[i] = ftd_drv_iap_fmc_read_4bit(addr + i * 4);
                }

                // Print read data
                FTD_LOG_DEBUG("Read data: version=0x%x\n", data.version);
                for (int i = 0; i < SUPPORT_FW_NUM; i++) {
                    FTD_LOG_DEBUG("FW[%d]: bin_remain=0x%x, triple_remain=0x%x, rollcode_remain=0x%x\n",
                        i, data.fw_info[i].bin_remain_counts,
                        data.fw_info[i].triple_remain_counts,
                        data.fw_info[i].rollcode_remain_counts);
                }
                FTD_LOG_DEBUG("CRC16: 0x%x data.version :0x%x, version: 0x%x\n", data.crc16, data.version, version);

                // Verify version number consistency
                if (data.version == version) {
                    // Verify CRC16
                    uint16_t calculated_crc = ftd_utils_hw_crc16((uint8_t*)&data, sizeof(FTD_MW_RING_BUFFER_REMAIN_COUNTS) - sizeof(uint16_t));
                    FTD_LOG_DEBUG("CRC16: 0x%x calculated_crc: 0x%x\n", data.crc16, calculated_crc);
                    if (calculated_crc == data.crc16) {
                        if (version > latest_version) {
                            FTD_LOG_DEBUG("Found newer version: 0x%x\n", version);
                            latest_version = version;
                            latest_addr = addr;
                            latest_slot = j;
                        }
                    }
                }
            }
            else if (version == 0xFFFFFFFF) {
                break;  // Encountered empty slot, rest are empty
            }
        }

        // Relock write protection register
        SYS_LockReg();
    }

    if (latest_version > 0) {
        // Restore state
        FTD_LOG_DEBUG("[Ring Buffer] Restoring state from slot %d\n", latest_slot);
        g_ftd_mw_ring_buffer_ctx.current_slot = latest_slot + 1;
        g_ftd_mw_ring_buffer_ctx.write_count = latest_version;
        g_ftd_mw_ring_buffer_ctx.last_valid_addr = latest_addr;

        // If current sector is full, start from beginning (ring overwrite)
        if (g_ftd_mw_ring_buffer_ctx.current_slot >= FTD_MW_RING_BUFFER_SLOTS_PER_SECTOR) {
            // Erase entire sector
            uint32_t sector_addr = FTD_MW_RING_BUFFER_DATAFLASH_BASE;
            if (ftd_mw_ring_buffer_erase_sector(sector_addr)) {
                g_ftd_mw_ring_buffer_ctx.current_slot = 0;
            }
        }
    }
    else {
        // First run, erase sector
        if (!g_ftd_mw_ring_buffer_ctx.sector_bad) {
            uint32_t sector_addr = FTD_MW_RING_BUFFER_DATAFLASH_BASE;
            ftd_mw_ring_buffer_erase_sector(sector_addr);
        }
        g_ftd_mw_ring_buffer_ctx.current_slot = 0;
        g_ftd_mw_ring_buffer_ctx.write_count = 0;
    }
}

// Actually write to Flash (with write protection check)
static void ftd_mw_ring_buffer_real_write(void)
{
    // Write protection check
    if (g_ftd_mw_ring_buffer_ctx.write_protected) {
        return;  // Write protection enabled, skip writing
    }

    // Check if sector is bad
    if (g_ftd_mw_ring_buffer_ctx.sector_bad) {
        return;  // Sector bad, cannot write
    }

    uint32_t write_addr;
    uint8_t write_buf[FTD_MW_RING_BUFFER_DATA_SIZE];

    // Prepare data: version + actual data
    g_ftd_mw_ring_buffer_ctx.write_count++;
    g_ftd_mw_ring_buffer_ctx.pending_data.version = g_ftd_mw_ring_buffer_ctx.write_count;

    // Recalculate CRC16 (including version number)
    uint16_t crc = ftd_utils_hw_crc16((uint8_t*)&g_ftd_mw_ring_buffer_ctx.pending_data, sizeof(FTD_MW_RING_BUFFER_REMAIN_COUNTS) - sizeof(uint16_t));
    g_ftd_mw_ring_buffer_ctx.pending_data.crc16 = crc;

    memset(write_buf, 0, FTD_MW_RING_BUFFER_DATA_SIZE); // Clear buffer first
    memcpy(write_buf, &g_ftd_mw_ring_buffer_ctx.pending_data,
        sizeof(FTD_MW_RING_BUFFER_REMAIN_COUNTS));

    // Check if sector needs to be erased
    if (g_ftd_mw_ring_buffer_ctx.current_slot >= FTD_MW_RING_BUFFER_SLOTS_PER_SECTOR) {
        // Current sector is full, erase and start from beginning
        uint32_t sector_addr = FTD_MW_RING_BUFFER_DATAFLASH_BASE;
        if (!ftd_mw_ring_buffer_erase_sector(sector_addr)) {
            return;  // Erase failed, mark as bad block and no longer use
        }
        g_ftd_mw_ring_buffer_ctx.current_slot = 0;
    }

    // Calculate write address
    write_addr = FTD_MW_RING_BUFFER_DATAFLASH_BASE +
        g_ftd_mw_ring_buffer_ctx.current_slot * FTD_MW_RING_BUFFER_DATA_SIZE;

    // Write data
    if (ftd_mw_ring_buffer_write_data(write_addr, write_buf)) {
        FTD_LOG_DEBUG("Write data to ring buffer success");
        g_ftd_mw_ring_buffer_ctx.last_valid_addr = write_addr;
        g_ftd_mw_ring_buffer_ctx.current_slot++;
        g_ftd_mw_ring_buffer_ctx.pending_count = 0;  // Clear pending count
    }
}

// ========== Public API ==========



// Update data (may not write to Flash)
void ftd_mw_ring_buffer_update(FTD_MW_RING_BUFFER_REMAIN_COUNTS* pData)
{
    static uint8_t first  = 1; // 1 indicates first call
    
    // Save to pending buffer
    g_ftd_mw_ring_buffer_ctx.pending_data = *pData;
    g_ftd_mw_ring_buffer_ctx.pending_count++;

    // Write directly on first call to ensure timely saving
    if (first) {
        if (!g_ftd_mw_ring_buffer_ctx.write_protected) {
            ftd_mw_ring_buffer_real_write();
        }
        first = 0; // Mark as not first call
    }
    // For subsequent calls, write only when interval is reached and not write protected
    else if (g_ftd_mw_ring_buffer_ctx.pending_count >= FTD_MW_RING_BUFFER_WRITE_INTERVAL &&
             !g_ftd_mw_ring_buffer_ctx.write_protected) {
        ftd_mw_ring_buffer_real_write();
    }
}

// Force immediate write
void ftd_mw_ring_buffer_force_write(void)
{
    if (g_ftd_mw_ring_buffer_ctx.pending_count > 0 &&
        !g_ftd_mw_ring_buffer_ctx.write_protected) {
        ftd_mw_ring_buffer_real_write();
    }
}

// Read latest data
uint8_t ftd_mw_ring_buffer_read(FTD_MW_RING_BUFFER_REMAIN_COUNTS* pData)
{
    if (g_ftd_mw_ring_buffer_ctx.write_count == 0) {
        return 0;  // No data
    }

    // Read from last valid address
    uint8_t read_buf[FTD_MW_RING_BUFFER_DATA_SIZE];
    uint32_t* pDst = (uint32_t*)read_buf;

    for (int i = 0; i < FTD_MW_RING_BUFFER_DATA_SIZE / 4; i++) {
        pDst[i] = ftd_drv_iap_fmc_read_4bit(g_ftd_mw_ring_buffer_ctx.last_valid_addr + i * 4);
    }

    // Verify version number
    FTD_MW_RING_BUFFER_REMAIN_COUNTS* pReadData = (FTD_MW_RING_BUFFER_REMAIN_COUNTS*)read_buf;
    if (pReadData->version != g_ftd_mw_ring_buffer_ctx.write_count) {
        return 0;  // Data corrupted
    }

    // Return actual data
    memcpy(pData, read_buf, sizeof(FTD_MW_RING_BUFFER_REMAIN_COUNTS));
    return 1;
}

// ========== Write Protection Interface Implementation ==========

// Enable write protection
void ftd_mw_ring_buffer_enable_write_protect(void)
{
    g_ftd_mw_ring_buffer_ctx.write_protected = 1;
    // Clear pending count to avoid auto-write after recovery
    g_ftd_mw_ring_buffer_ctx.pending_count = 0;
}

// Disable write protection
void ftd_mw_ring_buffer_disable_write_protect(void)
{
    g_ftd_mw_ring_buffer_ctx.write_protected = 0;
}

// Get write protection status
uint8_t ftd_mw_ring_buffer_is_write_protected(void)
{
    return g_ftd_mw_ring_buffer_ctx.write_protected;
}

// Update remain_counts from SYS_INFO
void ftd_mw_ring_buffer_update_remain_counts(SYS_INFO* p_sys_info)
{
    if (p_sys_info == NULL) {
        return;
    }

    // Prepare data
    FTD_MW_RING_BUFFER_REMAIN_COUNTS remain_counts_data;
    memset(&remain_counts_data, 0, sizeof(FTD_MW_RING_BUFFER_REMAIN_COUNTS));

    // Extract remain_counts from each FW_INFO
    for (uint8_t i = 0; i < p_sys_info->support_fw_counts && i < SUPPORT_FW_NUM; i++) {
        remain_counts_data.fw_info[i].bin_remain_counts = p_sys_info->st_fw_info[i].st_bin_info.remain_counts;
        remain_counts_data.fw_info[i].triple_remain_counts = p_sys_info->st_fw_info[i].st_triple_info.remain_counts;
        remain_counts_data.fw_info[i].rollcode_remain_counts = p_sys_info->st_fw_info[i].st_roll_code_info.remain_counts;
    }

    // Calculate CRC16 (excluding crc16 field itself)
    uint16_t crc = ftd_utils_hw_crc16((uint8_t*)&remain_counts_data, sizeof(FTD_MW_RING_BUFFER_REMAIN_COUNTS) - sizeof(uint16_t));
    remain_counts_data.crc16 = crc;

    // Update to ring buffer
    ftd_mw_ring_buffer_update(&remain_counts_data);
}

// Update remain_counts from SYS_INFO (direct write)
void ftd_mw_ring_buffer_force_update_remain_counts(SYS_INFO* p_sys_info)
{
    if (p_sys_info == NULL) {
        return;
    }

    // Prepare data
    FTD_MW_RING_BUFFER_REMAIN_COUNTS remain_counts_data;
    memset(&remain_counts_data, 0, sizeof(FTD_MW_RING_BUFFER_REMAIN_COUNTS));

    // Extract remain_counts from each FW_INFO
    for (uint8_t i = 0; i < p_sys_info->support_fw_counts && i < SUPPORT_FW_NUM; i++) {
        remain_counts_data.fw_info[i].bin_remain_counts = p_sys_info->st_fw_info[i].st_bin_info.remain_counts;
        remain_counts_data.fw_info[i].triple_remain_counts = p_sys_info->st_fw_info[i].st_triple_info.remain_counts;
        remain_counts_data.fw_info[i].rollcode_remain_counts = p_sys_info->st_fw_info[i].st_roll_code_info.remain_counts;
    }

    // Pre-calculate version number (consistent with real_write)
    uint32_t next_version = g_ftd_mw_ring_buffer_ctx.write_count + 1;

    // Print data to be written
    FTD_LOG_DEBUG("Force update data: version=0x%x\n", next_version);
    for (int i = 0; i < SUPPORT_FW_NUM; i++) {
        FTD_LOG_DEBUG("FW[%d]: bin_remain=0x%x, triple_remain=0x%x, rollcode_remain=0x%x\n",
            i, remain_counts_data.fw_info[i].bin_remain_counts,
            remain_counts_data.fw_info[i].triple_remain_counts,
            remain_counts_data.fw_info[i].rollcode_remain_counts);
    }
    FTD_LOG_DEBUG("CRC16 will be calculated in real_write\n");

    // Save to pending buffer
    g_ftd_mw_ring_buffer_ctx.pending_count++;
    g_ftd_mw_ring_buffer_ctx.pending_data = remain_counts_data;

    ftd_mw_ring_buffer_force_write();
}


// Sync ring buffer data to system info
int8_t ftd_mw_ring_buffer_sync_to_sys_info(void)
{
    // Read latest data from ring buffer
    FTD_MW_RING_BUFFER_REMAIN_COUNTS ring_buffer_data;
    if (!ftd_mw_ring_buffer_read(&ring_buffer_data)) {
        FTD_LOG_WARN("Failed to read ring buffer data for sync");
        return -1;
    }

    // Get current system info
    SYS_INFO sys_info;
    ftd_mw_sys_info_manager_get(&sys_info);

    // Check if data is different
    uint8_t need_sync = 0;
    uint8_t data_different[SUPPORT_FW_NUM] = {0};
    for (uint8_t i = 0; i < SUPPORT_FW_NUM; i++) {
        if (ring_buffer_data.fw_info[i].bin_remain_counts != sys_info.st_fw_info[i].st_bin_info.remain_counts ||
            ring_buffer_data.fw_info[i].triple_remain_counts != sys_info.st_fw_info[i].st_triple_info.remain_counts ||
            ring_buffer_data.fw_info[i].rollcode_remain_counts != sys_info.st_fw_info[i].st_roll_code_info.remain_counts) {
            data_different[i] = 1;
            need_sync = 1;
        }
    }

    if (need_sync) {
        FTD_LOG_INFO("Ring buffer data differs from system info, syncing...");
        
        // Update remain_counts in system info
        for (uint8_t i = 0; i < SUPPORT_FW_NUM; i++) {
            if(!data_different[i])
            {
                continue;
            }
            if (ring_buffer_data.fw_info[i].bin_remain_counts >= FTD_MW_RING_BUFFER_WRITE_INTERVAL) {
                sys_info.st_fw_info[i].st_bin_info.remain_counts = ring_buffer_data.fw_info[i].bin_remain_counts - FTD_MW_RING_BUFFER_WRITE_INTERVAL;
            } else {
                sys_info.st_fw_info[i].st_bin_info.remain_counts = 0;
            }
            if (ring_buffer_data.fw_info[i].triple_remain_counts >= FTD_MW_RING_BUFFER_WRITE_INTERVAL) {
                sys_info.st_fw_info[i].st_triple_info.remain_counts = ring_buffer_data.fw_info[i].triple_remain_counts - FTD_MW_RING_BUFFER_WRITE_INTERVAL;
            } else {
                sys_info.st_fw_info[i].st_triple_info.remain_counts = 0;
            }
            if (ring_buffer_data.fw_info[i].rollcode_remain_counts >= FTD_MW_RING_BUFFER_WRITE_INTERVAL) {
                sys_info.st_fw_info[i].st_roll_code_info.remain_counts = ring_buffer_data.fw_info[i].rollcode_remain_counts - FTD_MW_RING_BUFFER_WRITE_INTERVAL;
            } else {
                sys_info.st_fw_info[i].st_roll_code_info.remain_counts = 0;
            }
        }
        
        // Update system info
        ftd_mw_sys_info_manager_set(&sys_info);
        
        // Save to Flash
        int8_t ret = ftd_mw_sys_info_manager_save_nv();
        if (ret == 0) {
            FTD_LOG_INFO("Sync to system info successful");
            
            // Write updated system info back to ring buffer to ensure ring buffer data is also latest
            ftd_mw_ring_buffer_force_update_remain_counts(&sys_info);
            FTD_LOG_INFO("Ring buffer updated with latest data");
        } else {
            FTD_LOG_ERROR("Failed to save system info: %d", ret);
        }
        return ret;
    } else {
        FTD_LOG_INFO("Ring buffer data matches system info, no sync needed");
        return 0;
    }
}


// Initialize
void ftd_mw_ring_buffer_init(void)
{
    FTD_LOG_DEBUG("sizeof(FTD_MW_RING_BUFFER_REMAIN_COUNTS) = %d\n", sizeof(FTD_MW_RING_BUFFER_REMAIN_COUNTS));
    // Clear context
    memset(&g_ftd_mw_ring_buffer_ctx, 0, sizeof(g_ftd_mw_ring_buffer_ctx));
    ftd_drv_bod_set_callback(ftd_mw_ring_buffer_enable_write_protect);
    // Allow write by default
    g_ftd_mw_ring_buffer_ctx.write_protected = 0;

    // Scan and recover
    ftd_mw_ring_buffer_scan_and_recover();

    // Output scan result via serial
    if (g_ftd_mw_ring_buffer_ctx.write_count > 0) {
        FTD_LOG_INFO("[Ring Buffer] Found latest data: version=%d, slot=%d, addr=0x%08X\n",
            g_ftd_mw_ring_buffer_ctx.write_count,
            g_ftd_mw_ring_buffer_ctx.current_slot - 1,
            g_ftd_mw_ring_buffer_ctx.last_valid_addr);
    }
    else {
        FTD_LOG_DEBUG("[Ring Buffer] No data found, initializing...\n");
    }

    ftd_mw_ring_buffer_sync_to_sys_info();

    // If there is pending data to write, write immediately
    if (g_ftd_mw_ring_buffer_ctx.pending_count > 0 &&
        !g_ftd_mw_ring_buffer_ctx.write_protected) {
        ftd_mw_ring_buffer_real_write();
    }


}