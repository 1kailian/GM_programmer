/****************************************************************************
@FILENAME:  ftd_drv_flash_access.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.16
@COPYRIGHT: FTD co.ltd
****************************************************************************/

#include "ftd_drv_flash_access.h"
#include "ftd_mw_log_manager.h"
#include <stdio.h>
#include "NuMicro.h"

static uint32_t flash_page_number = 16384; //(4*1024*1024/PAGESIZE);//25Q32

PARTITION_ADDR_INFO st_partition_table[PARTITION_MAX] =
{
    {PARTITION_RESERVED_START_ADDR, PARTITION_RESERVED_SIZE},

    {PARTITION_WRITER_MANAGER_START_ADDR, PARTITION_WRITER_MANAGER_SIZE},

    {PARTITION_FW1_INFO_START_ADDR, PARTITION_FW1_INFO_SIZE},
    {PARTITION_FW1_BIN_START_ADDR, PARTITION_FW1_BIN_SIZE},
    {PARTITION_FW1_TRIPLE_START_ADDR, PARTITION_FW1_TRIPLE_SIZE},

    {PARTITION_FW2_INFO_START_ADDR, PARTITION_FW2_INFO_SIZE},
    {PARTITION_FW2_BIN_START_ADDR, PARTITION_FW2_BIN_SIZE},
    {PARTITION_FW2_TRIPLE_START_ADDR, PARTITION_FW2_TRIPLE_SIZE},

    {PARTITION_FW3_INFO_START_ADDR, PARTITION_FW3_INFO_SIZE},
    {PARTITION_FW3_BIN_START_ADDR, PARTITION_FW3_BIN_SIZE},
    {PARTITION_FW3_TRIPLE_START_ADDR, PARTITION_FW3_TRIPLE_SIZE},

    {PARTITION_CACHE_START_ADDR, PARTITION_CACHE_SIZE},
};


/*==============================Flash Basic Operation Functions Start ===================================*/
static uint16_t SpiFlash_ReadMidDid(void)
{
    uint8_t u8RxData[6], u8IDCnt = 0;

    // /CS: active
    SPI_SET_SS_LOW(SPI_FLASH_PORT);

    // send Command: 0x90, Read Manufacturer/Device ID
    SPI_WRITE_TX(SPI_FLASH_PORT, 0x90);

    // send 24-bit '0', dummy
    SPI_WRITE_TX(SPI_FLASH_PORT, 0x00);
    SPI_WRITE_TX(SPI_FLASH_PORT, 0x00);
    SPI_WRITE_TX(SPI_FLASH_PORT, 0x00);

    // receive 16-bit
    SPI_WRITE_TX(SPI_FLASH_PORT, 0x00);
    SPI_WRITE_TX(SPI_FLASH_PORT, 0x00);

    // wait tx finish
    while(SPI_IS_BUSY(SPI_FLASH_PORT));

    // /CS: de-active
    SPI_SET_SS_HIGH(SPI_FLASH_PORT);

    while(!SPI_GET_RX_FIFO_EMPTY_FLAG(SPI_FLASH_PORT))
        u8RxData[u8IDCnt ++] = SPI_READ_RX(SPI_FLASH_PORT);

    return ( (u8RxData[4]<<8) | u8RxData[5] );
}

static uint8_t SpiFlash_ReadStatusReg(void)
{
    // /CS: active
    SPI_SET_SS_LOW(SPI_FLASH_PORT);

    // send Command: 0x05, Read status register
    SPI_WRITE_TX(SPI_FLASH_PORT, 0x05);

    // read status
    SPI_WRITE_TX(SPI_FLASH_PORT, 0x00);

    // wait tx finish
    while(SPI_IS_BUSY(SPI_FLASH_PORT));

    // /CS: de-active
    SPI_SET_SS_HIGH(SPI_FLASH_PORT);

    // skip first rx data
    SPI_READ_RX(SPI_FLASH_PORT);

    return (SPI_READ_RX(SPI_FLASH_PORT) & 0xff);
}

// static void SpiFlash_WriteStatusReg(uint8_t u8Value)
// {
//     // /CS: active
//     SPI_SET_SS_LOW(SPI_FLASH_PORT);

//     // send Command: 0x06, Write enable
//     SPI_WRITE_TX(SPI_FLASH_PORT, 0x06);

//     // wait tx finish
//     while(SPI_IS_BUSY(SPI_FLASH_PORT));

//     // /CS: de-active
//     SPI_SET_SS_HIGH(SPI_FLASH_PORT);

//     ///////////////////////////////////////

//     // /CS: active
//     SPI_SET_SS_LOW(SPI_FLASH_PORT);

//     // send Command: 0x01, Write status register
//     SPI_WRITE_TX(SPI_FLASH_PORT, 0x01);

//     // write status
//     SPI_WRITE_TX(SPI_FLASH_PORT, u8Value);

//     // wait tx finish
//     while(SPI_IS_BUSY(SPI_FLASH_PORT));

//     // /CS: de-active
//     SPI_SET_SS_HIGH(SPI_FLASH_PORT);
// }

static void SpiFlash_WaitReady(void)
{
    uint8_t ReturnValue;
    do
    {
        ReturnValue = SpiFlash_ReadStatusReg();
        ReturnValue = ReturnValue & 1;
    }
    while(ReturnValue!=0);   // check the BUSY bit
}

//确保StartAddress 确保页面首地址 length 不大于PAGESIZE/256,返回实际写入的Byte数
static uint8_t SpiFlash_NormalPageProgram(uint32_t StartAddress, uint32_t length, uint8_t *u8DataBuffer)
{
    uint32_t writtenLen = 0;
    if(StartAddress%PAGESIZE !=0 )//不是页面首地址
    {
        JYMC_LOG_INFO("Error: StartAddress is not page head address\n");
        return 0;
    }
    if(length > PAGESIZE)
    {
        JYMC_LOG_INFO("Error: length is too long\n");
        return 0;
    }
    if(StartAddress+length > FLASH_SIZE)
    {
        JYMC_LOG_INFO("Error: StartAddress/length is too long\n");
        return 0;
    }
// JYMC_LOG_INFO(" SpiFlash_NormalPageProgram 111(StartAddress:0x%x length:%d)",StartAddress,length);
    // /CS: active
    SPI_SET_SS_LOW(SPI_FLASH_PORT);
    // send Command: 0x06, Write enable
    SPI_WRITE_TX(SPI_FLASH_PORT, 0x06);
    // wait tx finish
    while(SPI_IS_BUSY(SPI_FLASH_PORT));
    // /CS: de-active
    SPI_SET_SS_HIGH(SPI_FLASH_PORT);
    // /CS: active
    SPI_SET_SS_LOW(SPI_FLASH_PORT);
    // send Command: 0x02, Page program
    SPI_WRITE_TX(SPI_FLASH_PORT, 0x02);
    // send 24-bit start address
    SPI_WRITE_TX(SPI_FLASH_PORT, (StartAddress>>16) & 0xFF);
    SPI_WRITE_TX(SPI_FLASH_PORT, (StartAddress>>8)  & 0xFF);
    SPI_WRITE_TX(SPI_FLASH_PORT, StartAddress       & 0xFF);

    // write data
    for(writtenLen = 0; writtenLen < length; writtenLen++)
    {
        // 等待直到FIFO有空间
        while(SPI_GET_TX_FIFO_FULL_FLAG(SPI_FLASH_PORT))
        {
            // 等待FIFO有空间
        }
        
        SPI_WRITE_TX(SPI_FLASH_PORT, u8DataBuffer[writtenLen]);
        
        // 每写入几个字节后等待一下，避免FIFO溢出
        if((writtenLen & 0x7) == 0x7) // 每8个字节检查一次
        {
            while(SPI_IS_BUSY(SPI_FLASH_PORT));
        }
    }

    // wait tx finish
    while(SPI_IS_BUSY(SPI_FLASH_PORT));
    // /CS: de-active
    SPI_SET_SS_HIGH(SPI_FLASH_PORT);
    SPI_ClearRxFIFO(SPI_FLASH_PORT);

    return length;  
}

static int8_t SpiFlash_NormalRead(uint32_t StartAddress, uint32_t length, uint8_t *u8DataBuffer)
{
    uint32_t i;
    if(StartAddress+length > FLASH_SIZE)
    {
        JYMC_LOG_INFO("Error: StartAddress/length error in SpiFlash_NormalRead\n");
        return -1;
    }
    // /CS: active
    SPI_SET_SS_LOW(SPI_FLASH_PORT);

    // send Command: 0x03, Read data
    SPI_WRITE_TX(SPI_FLASH_PORT, 0x03);

    // send 24-bit start address
    SPI_WRITE_TX(SPI_FLASH_PORT, (StartAddress>>16) & 0xFF);
    SPI_WRITE_TX(SPI_FLASH_PORT, (StartAddress>>8)  & 0xFF);
    SPI_WRITE_TX(SPI_FLASH_PORT, StartAddress       & 0xFF);

    while(SPI_IS_BUSY(SPI_FLASH_PORT));
    // clear RX buffer
    SPI_ClearRxFIFO(SPI_FLASH_PORT);

    // read data
    for(i = 0; i < length; i++)
    {
        SPI_WRITE_TX(SPI_FLASH_PORT, 0x00);
        while(SPI_IS_BUSY(SPI_FLASH_PORT));
        u8DataBuffer[i] = SPI_READ_RX(SPI_FLASH_PORT);
    }
    // wait tx finish
    while(SPI_IS_BUSY(SPI_FLASH_PORT));

    // /CS: de-active
    SPI_SET_SS_HIGH(SPI_FLASH_PORT);

    return 0;
}

//StartAddress 必须是Sector size/ERASE_SIZE_MIN整数倍,这样也可以避免 在一个scector内重复擦除。
static void SpiFlash_erase_sector(uint32_t StartAddress)
{
    JYMC_LOG_INFO(" SpiFlash_erase_sector StartAddress(0x%x)",StartAddress);
    if((StartAddress%ERASE_SIZE_MIN) != 0)
    {
        // JYMC_LOG_INFO(" ERASE StartAddress error!!");
        return;
    }
    // /CS: active
    SPI_SET_SS_LOW(SPI_FLASH_PORT);

    // send Command: 0x06, Write enable
    SPI_WRITE_TX(SPI_FLASH_PORT, 0x06);

    // wait tx finish
    while(SPI_IS_BUSY(SPI_FLASH_PORT));

    // /CS: de-active
    SPI_SET_SS_HIGH(SPI_FLASH_PORT);

    // /CS: active
    SPI_SET_SS_LOW(SPI_FLASH_PORT);

    // send Command: 0x20, sector/4K Erase
    SPI_WRITE_TX(SPI_FLASH_PORT, 0x20);
    SPI_WRITE_TX(SPI_FLASH_PORT, (StartAddress>>16) & 0xFF);
    SPI_WRITE_TX(SPI_FLASH_PORT, (StartAddress>>8)  & 0xFF);
    SPI_WRITE_TX(SPI_FLASH_PORT, StartAddress       & 0xFF);
    // wait tx finish
    while(SPI_IS_BUSY(SPI_FLASH_PORT));

    // /CS: de-active
    SPI_SET_SS_HIGH(SPI_FLASH_PORT);

    SPI_ClearRxFIFO(SPI_FLASH_PORT);
}
/*==============================Flash Basic Operation Functions End ===================================*/

uint32_t ftd_drv_flash_access_get_partition_start_addr(PARTITION_NAME e_partition)
{
    if(e_partition >= PARTITION_MAX)
        return 0;
        
    return st_partition_table[e_partition].start_addr;
}

uint32_t ftd_drv_flash_access_get_partition_size(PARTITION_NAME e_partition)
{
    if(e_partition >= PARTITION_MAX)
        return 0;
        
    return st_partition_table[e_partition].size;
}

static int8_t ftd_drv_flash_access_erase(uint32_t StartAddress, uint32_t length)
{
    uint16_t erase_sector_cnt = 0;
    uint16_t i = 0;

    if(StartAddress % ERASE_SIZE_MIN)
    {
        JYMC_LOG_INFO(" Error: length is not a multiple of ERASE_SIZE_MIN\n");
        return -1;
    }

    erase_sector_cnt = length / ERASE_SIZE_MIN;
    if(length < ERASE_SIZE_MIN)
    {
        erase_sector_cnt = 1;
    }
    
    for(i = 0; i < erase_sector_cnt; i++)
    {
        SpiFlash_WaitReady();
        SpiFlash_erase_sector(StartAddress + i * ERASE_SIZE_MIN);
    }

    return 0;
}

static int8_t ftd_drv_flash_access_read(uint32_t StartAddress, uint32_t length, uint8_t *u8DataBuffer)
{
    return SpiFlash_NormalRead(StartAddress, length, u8DataBuffer);
}

//u32FlashAddress 起始地址必须是 page开头位置，即PAGESIZE * n
static int8_t ftd_drv_flash_access_write(uint32_t u32FlashAddress, uint32_t writeLen, uint8_t *pu8Data)
{
    uint32_t u32Page = 0;
    uint32_t u32WittenLength = 0;
    uint32_t u32PageNumber = writeLen/PAGESIZE;
    uint32_t u32Remainder = writeLen%PAGESIZE;

    if( ( u32PageNumber > flash_page_number) || ((u32FlashAddress + writeLen) >= FLASH_SIZE ) \
        || ((u32FlashAddress%PAGESIZE) != 0) ) 
        return -1;

    SpiFlash_WaitReady();
    for(u32Page = 0; u32Page < u32PageNumber; u32Page++)
    {
        u32WittenLength = u32Page*PAGESIZE;
        SpiFlash_erase_sector(u32FlashAddress+u32WittenLength);//SpiFlash_erase_sector
        SpiFlash_WaitReady();
        SpiFlash_NormalPageProgram(u32FlashAddress+u32WittenLength, PAGESIZE ,&pu8Data[u32WittenLength]);
        SpiFlash_WaitReady();
    }

    if(u32Remainder != 0)
    {
        u32WittenLength = u32PageNumber*PAGESIZE; //上一步中最后没有翻页
        SpiFlash_erase_sector(u32FlashAddress+u32WittenLength);//SpiFlash_erase_sector
        SpiFlash_WaitReady();
        SpiFlash_NormalPageProgram(u32FlashAddress+u32WittenLength, u32Remainder ,&pu8Data[u32WittenLength]);
        SpiFlash_WaitReady();    
    }   

    return 0;
}


static void ftd_drv_flash_access_init(void)
{
    uint16_t u16ID;
    /* Unlock protected registers */
    SYS_UnlockReg();
    CLK_EnableModuleClock(SPI_FLASH_MODLUE_CLK);

    /* Enable SPI0 clock pin PA2 schmitt trigger,clk pin Msk */
    // PA->SMTEN |= GPIO_SMTEN_SMTEN2_Msk;
    // /* Enable SPI0 I/O high slew rate, clk pin bit set 1 */
    // GPIO_SetSlewCtl(PA, 0x8, GPIO_SLEWCTL_HIGH);
    /* Enable SPI1 clock pin PB3 schmitt trigger,clk pin Msk */
    // PB->SMTEN |= GPIO_SMTEN_SMTEN3_Msk;
    // /* Enable SPI1 I/O high slew rate, clk pin bit set 1 */
    // GPIO_SetSlewCtl(PB, 0xF, GPIO_SLEWCTL_HIGH);
    SPI_FLASH_PIN_SET
    /* Configure SPI_FLASH_PORT as a master, MSB first, 8-bit transaction, SPI Mode-0 timing, clock is 20MHz */
    SPI_Open(SPI_FLASH_PORT, SPI_MASTER, SPI_MODE_0, 8, 20000000);
    /* Disable auto SS function, control SS signal manually. */
    SPI_DisableAutoSS(SPI_FLASH_PORT);
    /* Lock protected registers */
    SYS_LockReg();    

    u16ID = SpiFlash_ReadMidDid();
    if (u16ID == 0xEF13)
    {
       JYMC_LOG_INFO("Flash found: W25Q80 ...\n");
       flash_page_number = 4096;
    }
    else if (u16ID == 0xEF14)
    {
        JYMC_LOG_INFO("Flash found: W25Q16 ...\n");
        flash_page_number = 8192;
    }
    else if (u16ID == 0xEF15)
    {
        JYMC_LOG_INFO("Flash found: W25Q32 ...\n");
        flash_page_number = 16384;
    }
    else if (u16ID == 0xEF16)
    {
        JYMC_LOG_INFO("Flash found: W25Q64 ...\n");
        flash_page_number = 32768;
    }
    else if (u16ID == 0xEF17)
    {
        JYMC_LOG_INFO("Flash found: W25Q128 ...\n");
        flash_page_number = 65536;
    }
    else
    {
        JYMC_LOG_INFO("Wrong ID, 0x%x\n", u16ID);
        flash_page_number = 0;
    }
}

/**
 * @brief Flash operation function supporting erase, write and read operations
 * 
 * @param e_partition Partition name, specifies the flash partition to operate on
 * @param ope_type Operation type, including erase, write and read
 * @param addr_offset Offset address within the partition
 * @param length Length of data to operate on
 * @param pu8Data Pointer to data buffer, used for write or read operations
 * @return int8_t Operation result: 0 for success, negative values for failure 
 *         (-1: invalid partition, -2: length exceeds limit, -3: erase address not aligned,
 *          -4: write address not page-aligned, -5: underlying operation failed)
 */
int8_t ftd_drv_flash_operation (PARTITION_NAME e_partition, PATITION_OPERATION_TYPE ope_type, 
                                    uint32_t addr_offset, uint32_t length, uint8_t *pu8Data)
{
    static uint8_t init_flg = 0;
    int8_t ret = 0;
    uint32_t addr_temp = 0;

    if(init_flg == 0)
    {
        ftd_drv_flash_access_init();
        init_flg = 1;
    }

    if(e_partition > PARTITION_MAX)
    {
        ret = -1;
        return ret;
    }

    if(length > st_partition_table[e_partition].size)
    {
        ret = -2;
        return ret;
    }

    if(ope_type == OPERATION_ERASE)
    {
        addr_temp = addr_offset % ERASE_SIZE_MIN;
        if(addr_temp != 0)
        {
            ret = -3;
            return ret;
        }
    }

    if ( (ope_type == OPERATION_WRITE) && ( (addr_offset % PAGESIZE) != 0 ) )
    {
        ret = -4;
        return ret;
    }

    switch(ope_type)
    { 
        case OPERATION_ERASE: //if erase len < ERASE_SIZE_MIN,will be erase 1 sector
            ret = ftd_drv_flash_access_erase(st_partition_table[e_partition].start_addr+addr_offset, length);
            break;
        case OPERATION_WRITE:
            ret = ftd_drv_flash_access_write(st_partition_table[e_partition].start_addr+addr_offset, length, pu8Data);
            break;
        case OPERATION_READ:
            ret = ftd_drv_flash_access_read(st_partition_table[e_partition].start_addr+addr_offset, length, pu8Data);
            break;
        default:
            break;
    }

    if(ret < 0)
        return -5;
    else
        return 0;
}

#if 0
void ftd_flash_any_write_test(uint32_t offset, uint32_t len, uint8_t *pu8Data)
{ 
    ftd_flash_write(MCU_BIN_START_ADDR+offset, len, pu8Data);
}
void ftd_flash_any_read_test(uint32_t offset, uint32_t len, uint8_t *pu8Data)
{ 
    ftd_flash_read(MCU_BIN_START_ADDR+offset, len, pu8Data);
}
staic void flashtest(void)
{    
    uint8_t tesetbuff[1280];
    uint16_t i;
    ftd_flash_init();
    ftd_flash_deployment_data_read();
    JYMC_LOG_INFO(" Before: mcu_bin_addr = 0x%x \n", st_deployment_data.st_bin_info[0].start_addr);
    st_deployment_data.st_bin_info[0].start_addr = 0x123456;
    ftd_flash_deployment_data_write();
    ftd_flash_deployment_data_read();
    JYMC_LOG_INFO(" After: mcu_bin_addr = 0x%x \n", st_deployment_data.st_bin_info[0].start_addr);

    //任意地址读写测试
    for (i = 0; i < 1280; i++)
    {
        tesetbuff[i] = i%256;
    }
    JYMC_LOG_INFO(" start flashtest anyaddr test(0x%x-0x%x-0x%x)",tesetbuff[0],tesetbuff[257],tesetbuff[1190]);

    ftd_flash_any_write_test(PAGESIZE*2,1200,tesetbuff);
    JYMC_LOG_INFO(" start flashtest anyaddr test 111");
    for (i = 0; i < 1280; i++)
    {
        tesetbuff[i] = 0;
    }
    ftd_flash_any_read_test(PAGESIZE*2,1200,tesetbuff);
    JYMC_LOG_INFO(" READ2 flashtest anyaddr test(0x%x-0x%x-0x%x)",tesetbuff[0],tesetbuff[257],tesetbuff[1190]);
}

#endif

