#include "ftd_drv_ring_buffer.h"
#include "ftd_mw_log_manager.h"
#include "ftd_drv_iap_fmc.h"
#include "ftd_utils.h"

// ========== 内部结构 ==========
typedef struct {
    // Flash 管理
    uint32_t current_slot;          // 当前 slot (0-101)
    uint32_t write_count;           // 总写入次数（版本号）
    uint32_t last_valid_addr;       // 最后有效数据地址

    // 写入间隔控制
    uint32_t pending_count;          // 待写入次数
    FTD_DRV_RING_BUFFER_REMAIN_COUNTS pending_data; // 待写入的数据

    // 坏块标记
    uint8_t  sector_bad;            // sector 是否损坏

    // 写保护标志
    uint8_t  write_protected;        // 1=禁止写入/擦除, 0=允许
} ftd_drv_ring_buffer_ctx_t;

static ftd_drv_ring_buffer_ctx_t g_ftd_drv_ring_buffer_ctx;

// ========== FMC 底层操作（带写保护检查）==========

// 擦除一个 sector（带坏块检测和写保护）
static uint8_t ftd_drv_ring_buffer_erase_sector(uint32_t addr)
{
    // 写保护检查
    if (g_ftd_drv_ring_buffer_ctx.write_protected) {
        return 0;  // 写保护生效，禁止擦除
    }

    // 地址必须 4KB 对齐
    if (addr & 0xFFF) return 0;

    // 临时解锁写保护寄存器
    SYS_UnlockReg();

    FMC_Erase(addr);


    // 验证是否擦除成功（检查第一个字是否为 0xFFFFFFFF）
    if (FMC_Read(addr) != 0xFFFFFFFF) {
        g_ftd_drv_ring_buffer_ctx.sector_bad = 1;
        return 0;
    }

    return 1;  // 成功
}

// 写入一个字（带写保护检查）
static uint8_t ftd_drv_ring_buffer_write_word(uint32_t addr, uint32_t data)
{
    // 写保护检查
    if (g_ftd_drv_ring_buffer_ctx.write_protected) {
        return 0;  // 写保护生效，禁止写入
    }

    if (addr & 0x3) return 0;


    ftd_drv_iap_fmc_write_4bit(addr, data);
    return 1;
}

// 写入数据（带写保护检查）
static uint8_t ftd_drv_ring_buffer_write_data(uint32_t addr, uint8_t* pData)
{
    // 写保护检查（快速返回）
    if (g_ftd_drv_ring_buffer_ctx.write_protected) {
        return 0;
    }

    uint32_t i;
    uint32_t* pSrc = (uint32_t*)pData;
    uint32_t word_count = FTD_DRV_RING_BUFFER_DATA_SIZE / 4;  // 10 个字

    for (i = 0; i < word_count; i++) {
        if (!ftd_drv_ring_buffer_write_word(addr + i * 4, pSrc[i])) {
            return 0;
        }
    }

    return 1;
}

// ========== 内部函数 ==========

// 扫描 sector，找到最新数据
static void ftd_drv_ring_buffer_scan_and_recover(void)
{
    uint32_t j;
    uint32_t addr;
    uint32_t latest_version = 0;
    uint32_t latest_addr = 0;
    uint32_t latest_slot = 0;

    // 只处理一个 sector
    if (!g_ftd_drv_ring_buffer_ctx.sector_bad) {
        uint32_t sector_addr = FTD_DRV_RING_BUFFER_DATAFLASH_BASE;

        // 临时解锁写保护寄存器
        SYS_UnlockReg();

        for (j = 0; j < FTD_DRV_RING_BUFFER_SLOTS_PER_SECTOR; j++) {
            addr = sector_addr + j * FTD_DRV_RING_BUFFER_DATA_SIZE;
            uint32_t version = ftd_drv_iap_fmc_read_4bit(addr);
            FTD_LOG_DEBUG("version: %x, addr: %x, slot: %d\n", version, addr, j);
            if (version != 0xFFFFFFFF && version != 0)
            {
                // 读取完整数据进行验证
                FTD_DRV_RING_BUFFER_REMAIN_COUNTS data;
                uint32_t* pData = (uint32_t*)&data;

                // 读取完整的结构体数据
                for (int i = 0; i < sizeof(FTD_DRV_RING_BUFFER_REMAIN_COUNTS) / 4; i++) {
                    pData[i] = ftd_drv_iap_fmc_read_4bit(addr + i * 4);
                }

                // 打印读取的数据
                FTD_LOG_DEBUG("Read data: version=0x%x\n", data.version);
                for (int i = 0; i < SUPPORT_FW_NUM; i++) {
                    FTD_LOG_DEBUG("FW[%d]: bin_remain=0x%x, triple_remain=0x%x, rollcode_remain=0x%x\n",
                        i, data.fw_info[i].bin_remain_counts,
                        data.fw_info[i].triple_remain_counts,
                        data.fw_info[i].rollcode_remain_counts);
                }
                FTD_LOG_DEBUG("CRC16: 0x%x data.version :0x%x, version: 0x%x\n", data.crc16, data.version, version);

                // 验证版本号一致性
                if (data.version == version) {
                    // 验证CRC16
                    uint16_t calculated_crc = ftd_utils_hw_crc16((uint8_t*)&data, sizeof(FTD_DRV_RING_BUFFER_REMAIN_COUNTS) - sizeof(uint16_t));
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
                break;  // 遇到空 slot，后面都是空的
            }
        }

        // 重新锁定写保护寄存器
        SYS_LockReg();
    }

    if (latest_version > 0) {
        // 恢复状态
        FTD_LOG_DEBUG("[Ring Buffer] Restoring state from slot %d\n", latest_slot);
        g_ftd_drv_ring_buffer_ctx.current_slot = latest_slot + 1;
        g_ftd_drv_ring_buffer_ctx.write_count = latest_version;
        g_ftd_drv_ring_buffer_ctx.last_valid_addr = latest_addr;

        // 如果当前 sector 已写满，从头开始（环形覆盖）
        if (g_ftd_drv_ring_buffer_ctx.current_slot >= FTD_DRV_RING_BUFFER_SLOTS_PER_SECTOR) {
            // 擦除整个 sector
            uint32_t sector_addr = FTD_DRV_RING_BUFFER_DATAFLASH_BASE;
            if (ftd_drv_ring_buffer_erase_sector(sector_addr)) {
                g_ftd_drv_ring_buffer_ctx.current_slot = 0;
            }
        }
    }
    else {
        // 首次运行，擦除 sector
        if (!g_ftd_drv_ring_buffer_ctx.sector_bad) {
            uint32_t sector_addr = FTD_DRV_RING_BUFFER_DATAFLASH_BASE;
            ftd_drv_ring_buffer_erase_sector(sector_addr);
        }
        g_ftd_drv_ring_buffer_ctx.current_slot = 0;
        g_ftd_drv_ring_buffer_ctx.write_count = 0;
    }
}

// 真正写入 Flash（带写保护检查）
static void ftd_drv_ring_buffer_real_write(void)
{
    // 写保护检查
    if (g_ftd_drv_ring_buffer_ctx.write_protected) {
        return;  // 写保护生效，放弃写入
    }

    // 检查 sector 是否损坏
    if (g_ftd_drv_ring_buffer_ctx.sector_bad) {
        return;  // sector 损坏，无法写入
    }

    uint32_t write_addr;
    uint8_t write_buf[FTD_DRV_RING_BUFFER_DATA_SIZE];

    // 准备数据：版本号 + 实际数据
    g_ftd_drv_ring_buffer_ctx.write_count++;
    g_ftd_drv_ring_buffer_ctx.pending_data.version = g_ftd_drv_ring_buffer_ctx.write_count;

    // 重新计算CRC16（包含版本号）
    uint16_t crc = ftd_utils_hw_crc16((uint8_t*)&g_ftd_drv_ring_buffer_ctx.pending_data, sizeof(FTD_DRV_RING_BUFFER_REMAIN_COUNTS) - sizeof(uint16_t));
    g_ftd_drv_ring_buffer_ctx.pending_data.crc16 = crc;

    memset(write_buf, 0, FTD_DRV_RING_BUFFER_DATA_SIZE); // 先清空缓冲区
    memcpy(write_buf, &g_ftd_drv_ring_buffer_ctx.pending_data,
        sizeof(FTD_DRV_RING_BUFFER_REMAIN_COUNTS));

    // 检查是否需要擦除 sector
    if (g_ftd_drv_ring_buffer_ctx.current_slot >= FTD_DRV_RING_BUFFER_SLOTS_PER_SECTOR) {
        // 当前 sector 已满，擦除后从头开始
        uint32_t sector_addr = FTD_DRV_RING_BUFFER_DATAFLASH_BASE;
        if (!ftd_drv_ring_buffer_erase_sector(sector_addr)) {
            return;  // 擦除失败，标记为坏块后不再使用
        }
        g_ftd_drv_ring_buffer_ctx.current_slot = 0;
    }

    // 计算写入地址
    write_addr = FTD_DRV_RING_BUFFER_DATAFLASH_BASE +
        g_ftd_drv_ring_buffer_ctx.current_slot * FTD_DRV_RING_BUFFER_DATA_SIZE;

    // 写入数据
    if (ftd_drv_ring_buffer_write_data(write_addr, write_buf)) {
        FTD_LOG_DEBUG("Write data to ring buffer success");
        g_ftd_drv_ring_buffer_ctx.last_valid_addr = write_addr;
        g_ftd_drv_ring_buffer_ctx.current_slot++;
        g_ftd_drv_ring_buffer_ctx.pending_count = 0;  // 清除待写入计数
    }
}

// ========== 公共 API ==========

// 初始化
void ftd_drv_ring_buffer_init(void)
{
    FTD_LOG_DEBUG("sizeof(FTD_DRV_RING_BUFFER_REMAIN_COUNTS) = %d\n", sizeof(FTD_DRV_RING_BUFFER_REMAIN_COUNTS));
    // 清空上下文
    memset(&g_ftd_drv_ring_buffer_ctx, 0, sizeof(g_ftd_drv_ring_buffer_ctx));

    // 默认允许写入
    g_ftd_drv_ring_buffer_ctx.write_protected = 0;

    // 扫描并恢复
    ftd_drv_ring_buffer_scan_and_recover();

    // // 串口输出扫描结果
    // if (g_ftd_drv_ring_buffer_ctx.write_count > 0) {
    //     FTD_LOG_DEBUG("[Ring Buffer] Found latest data: version=%d, slot=%d, addr=0x%08X\n",
    //         g_ftd_drv_ring_buffer_ctx.write_count,
    //         g_ftd_drv_ring_buffer_ctx.current_slot - 1,
    //         g_ftd_drv_ring_buffer_ctx.last_valid_addr);
    // }
    // else {
    //     FTD_LOG_DEBUG("[Ring Buffer] No data found, initializing...\n");
    // }

    // 如果有待写入的 pending 数据，立即写入
    if (g_ftd_drv_ring_buffer_ctx.pending_count > 0 &&
        !g_ftd_drv_ring_buffer_ctx.write_protected) {
        ftd_drv_ring_buffer_real_write();
    }
}

// 更新数据（不一定会写入 Flash）
void ftd_drv_ring_buffer_update(FTD_DRV_RING_BUFFER_REMAIN_COUNTS* pData)
{
    // 保存到 pending 缓冲区
    g_ftd_drv_ring_buffer_ctx.pending_data = *pData;
    g_ftd_drv_ring_buffer_ctx.pending_count++;

    // 达到间隔次数，且未写保护，才真正写入
    if (g_ftd_drv_ring_buffer_ctx.pending_count >= FTD_DRV_RING_BUFFER_WRITE_INTERVAL &&
        !g_ftd_drv_ring_buffer_ctx.write_protected) {
        ftd_drv_ring_buffer_real_write();
    }
}

// 强制立即写入
void ftd_drv_ring_buffer_force_write(void)
{
    if (g_ftd_drv_ring_buffer_ctx.pending_count > 0 &&
        !g_ftd_drv_ring_buffer_ctx.write_protected) {
        ftd_drv_ring_buffer_real_write();
    }
}

// 读取最新数据
uint8_t ftd_drv_ring_buffer_read(FTD_DRV_RING_BUFFER_REMAIN_COUNTS* pData)
{
    if (g_ftd_drv_ring_buffer_ctx.write_count == 0) {
        return 0;  // 没有数据
    }

    // 从最后有效地址读取
    uint8_t read_buf[FTD_DRV_RING_BUFFER_DATA_SIZE];
    uint32_t* pDst = (uint32_t*)read_buf;

    for (int i = 0; i < FTD_DRV_RING_BUFFER_DATA_SIZE / 4; i++) {
        pDst[i] = ftd_drv_iap_fmc_read_4bit(g_ftd_drv_ring_buffer_ctx.last_valid_addr + i * 4);
    }

    // 验证版本号
    FTD_DRV_RING_BUFFER_REMAIN_COUNTS* pReadData = (FTD_DRV_RING_BUFFER_REMAIN_COUNTS*)read_buf;
    if (pReadData->version != g_ftd_drv_ring_buffer_ctx.write_count) {
        return 0;  // 数据损坏
    }

    // 返回实际数据
    memcpy(pData, read_buf, sizeof(FTD_DRV_RING_BUFFER_REMAIN_COUNTS));
    return 1;
}

// ========== 写保护接口实现 ==========

// 设置写保护
void ftd_drv_ring_buffer_set_write_protect(uint8_t enable)
{
    g_ftd_drv_ring_buffer_ctx.write_protected = enable ? 1 : 0;

    // 如果使能写保护时，清除 pending 计数，避免恢复后自动写入
    if (enable) {
        g_ftd_drv_ring_buffer_ctx.pending_count = 0;
    }
}

// 获取写保护状态
uint8_t ftd_drv_ring_buffer_is_write_protected(void)
{
    return g_ftd_drv_ring_buffer_ctx.write_protected;
}

// 从SYS_INFO更新remain_counts
void ftd_drv_ring_buffer_update_remain_counts(SYS_INFO* p_sys_info)
{
    if (p_sys_info == NULL) {
        return;
    }

    // 准备数据
    FTD_DRV_RING_BUFFER_REMAIN_COUNTS remain_counts_data;
    memset(&remain_counts_data, 0, sizeof(FTD_DRV_RING_BUFFER_REMAIN_COUNTS));

    // 提取每个FW_INFO中的remain_counts
    for (uint8_t i = 0; i < p_sys_info->support_fw_counts && i < SUPPORT_FW_NUM; i++) {
        remain_counts_data.fw_info[i].bin_remain_counts = p_sys_info->st_fw_info[i].st_bin_info.remain_counts;
        remain_counts_data.fw_info[i].triple_remain_counts = p_sys_info->st_fw_info[i].st_triple_info.remain_counts;
        remain_counts_data.fw_info[i].rollcode_remain_counts = p_sys_info->st_fw_info[i].st_roll_code_info.remain_counts;
    }

    // 计算CRC16（不包含crc16字段本身）
    uint16_t crc = ftd_utils_hw_crc16((uint8_t*)&remain_counts_data, sizeof(FTD_DRV_RING_BUFFER_REMAIN_COUNTS) - sizeof(uint16_t));
    remain_counts_data.crc16 = crc;

    // 更新到环形缓冲区
    ftd_drv_ring_buffer_update(&remain_counts_data);
}

// 从SYS_INFO更新remain_counts（直接写入）
void ftd_drv_ring_buffer_force_update_remain_counts(SYS_INFO* p_sys_info)
{
    if (p_sys_info == NULL) {
        return;
    }

    // 准备数据
    FTD_DRV_RING_BUFFER_REMAIN_COUNTS remain_counts_data;
    memset(&remain_counts_data, 0, sizeof(FTD_DRV_RING_BUFFER_REMAIN_COUNTS));

    // 提取每个FW_INFO中的remain_counts
    for (uint8_t i = 0; i < p_sys_info->support_fw_counts && i < SUPPORT_FW_NUM; i++) {
        remain_counts_data.fw_info[i].bin_remain_counts = p_sys_info->st_fw_info[i].st_bin_info.remain_counts;
        remain_counts_data.fw_info[i].triple_remain_counts = p_sys_info->st_fw_info[i].st_triple_info.remain_counts;
        remain_counts_data.fw_info[i].rollcode_remain_counts = p_sys_info->st_fw_info[i].st_roll_code_info.remain_counts;
    }

    // 打印准备写入的数据
    FTD_LOG_DEBUG("Force update data: version=0x%x\n", remain_counts_data.version);
    for (int i = 0; i < SUPPORT_FW_NUM; i++) {
        FTD_LOG_DEBUG("FW[%d]: bin_remain=0x%x, triple_remain=0x%x, rollcode_remain=0x%x\n",
            i, remain_counts_data.fw_info[i].bin_remain_counts,
            remain_counts_data.fw_info[i].triple_remain_counts,
            remain_counts_data.fw_info[i].rollcode_remain_counts);
    }


    // 保存到 pending 缓冲区
    g_ftd_drv_ring_buffer_ctx.pending_data = remain_counts_data;

    if (!g_ftd_drv_ring_buffer_ctx.write_protected) {
        ftd_drv_ring_buffer_real_write();
    }
}
