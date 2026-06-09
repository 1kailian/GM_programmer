#ifndef __FTD_DRV_RING_BUFFER_H__
#define __FTD_DRV_RING_BUFFER_H__

#include "NuMicro.h"
#include <string.h>
#include "ftd_data_model.h"

// 单个FW_INFO的remain_counts结构体
typedef struct {
    uint32_t bin_remain_counts;      // BIN_INFO的remain_counts
    uint32_t triple_remain_counts;   // TRIPLE_INFO的remain_counts
    uint32_t rollcode_remain_counts; // ROLLCODE_INFO的remain_counts
} FTD_DRV_RING_BUFFER_FW_INFO;


// 用于存储remain_counts的结构体
typedef struct {
    uint32_t version;             // 版本号
    FTD_DRV_RING_BUFFER_FW_INFO fw_info[SUPPORT_FW_NUM]; // 支持的FW_INFO数组
    uint8_t reserved[2];         // 保留字段，用于未来扩展
    uint16_t crc16;              // 数据校验CRC16
} FTD_DRV_RING_BUFFER_REMAIN_COUNTS;

// ========== 用户配置 ==========
#define FTD_DRV_RING_BUFFER_SECTOR_SIZE         4096            // 4KB
#define FTD_DRV_RING_BUFFER_DATA_SIZE           (sizeof(FTD_DRV_RING_BUFFER_REMAIN_COUNTS))              // 每次写入 44 字节（4字节版本号 + 40字节数据）
#define FTD_DRV_RING_BUFFER_SLOTS_PER_SECTOR    (FTD_DRV_RING_BUFFER_SECTOR_SIZE / FTD_DRV_RING_BUFFER_DATA_SIZE)  // 93 次

// DataFlash 配置（根据 ICP 工具设置）
#define FTD_DRV_RING_BUFFER_DATAFLASH_BASE      0x00004000      // DataFlash 起始地址
#define FTD_DRV_RING_BUFFER_SECTOR_COUNT        1               // 只有 1 个 sector

// 写入间隔：每 N 次调用 Write 才真正写入一次
#define FTD_DRV_RING_BUFFER_WRITE_INTERVAL      50              // 50 次更新才写入一次

// ========== API 函数 ==========
void ftd_drv_ring_buffer_init(void);                                    // 初始化
void ftd_drv_ring_buffer_update(FTD_DRV_RING_BUFFER_REMAIN_COUNTS* pData); // 更新数据（不一定写入）
void ftd_drv_ring_buffer_force_write(void);                             // 强制立即写入
uint8_t ftd_drv_ring_buffer_read(FTD_DRV_RING_BUFFER_REMAIN_COUNTS* pData); // 读取最新数据，返回 1=成功,0=失败
void ftd_drv_ring_buffer_update_remain_counts(SYS_INFO* p_sys_info);    // 从SYS_INFO更新remain_counts
void ftd_drv_ring_buffer_force_update_remain_counts(SYS_INFO* p_sys_info); // 从SYS_INFO更新remain_counts（直接写入）

// ========== 写保护接口 ==========
void ftd_drv_ring_buffer_set_write_protect(uint8_t enable);             // 设置写保护（1=禁止写入/擦除, 0=允许）
uint8_t ftd_drv_ring_buffer_is_write_protected(void);                   // 获取写保护状态

#endif
