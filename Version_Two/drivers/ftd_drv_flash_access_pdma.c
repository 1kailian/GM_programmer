/****************************************************************************
@FILENAME:  ftd_drv_flash_access_pdma.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.16
@COPYRIGHT: FTD co.ltd
****************************************************************************/

#include "ftd_drv_flash_access_pdma.h"
#include "ftd_mw_log_manager.h"
#include <stdio.h>
#include "NuMicro.h"


static uint32_t flash_page_number = 16384; //(4*1024*1024/PAGESIZE);//25Q32

PARTITION_ADDR_INFO st_partition_table[PARTITION_MAX] =
{
    {PARTITION_RESERVED_START_ADDR,         PARTITION_RESERVED_SIZE},

    {PARTITION_WRITER_MANAGER_START_ADDR,   PARTITION_WRITER_MANAGER_SIZE},

    {PARTITION_FW1_INFO_START_ADDR,         PARTITION_FW1_INFO_SIZE},
    {PARTITION_FW1_BIN_START_ADDR,          PARTITION_FW1_BIN_SIZE},
    {PARTITION_FW1_TRIPLE_START_ADDR,       PARTITION_FW1_TRIPLE_SIZE},

    {PARTITION_FW2_INFO_START_ADDR,         PARTITION_FW2_INFO_SIZE},
    {PARTITION_FW2_BIN_START_ADDR,          PARTITION_FW2_BIN_SIZE},
    {PARTITION_FW2_TRIPLE_START_ADDR,       PARTITION_FW2_TRIPLE_SIZE},

    {PARTITION_FW3_INFO_START_ADDR,         PARTITION_FW3_INFO_SIZE},
    {PARTITION_FW3_BIN_START_ADDR,          PARTITION_FW3_BIN_SIZE},
    {PARTITION_FW3_TRIPLE_START_ADDR,       PARTITION_FW3_TRIPLE_SIZE},

    {PARTITION_CACHE_START_ADDR,            PARTITION_CACHE_SIZE},
};

static void ftd_drv_flash_access_init(void);

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
    while (SPI_IS_BUSY(SPI_FLASH_PORT));

    // /CS: de-active
    SPI_SET_SS_HIGH(SPI_FLASH_PORT);

    while (!SPI_GET_RX_FIFO_EMPTY_FLAG(SPI_FLASH_PORT))
        u8RxData[u8IDCnt++] = SPI_READ_RX(SPI_FLASH_PORT);

    return ((u8RxData[4] << 8) | u8RxData[5]);
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
    while (SPI_IS_BUSY(SPI_FLASH_PORT));

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
    } while (ReturnValue != 0);   // check the BUSY bit
}

static uint8_t SpiFlash_NormalPageProgram(uint32_t StartAddress, uint32_t length, uint8_t* u8DataBuffer)
{
    uint32_t writtenLen = 0;
    if (StartAddress % PAGESIZE != 0)
    {
        FTD_LOG_INFO("Error: StartAddress is not page head address\n");
        return 0;
    }
    if (length > PAGESIZE)
    {
        FTD_LOG_INFO("Error: length is too long\n");
        return 0;
    }
    if (StartAddress + length > FLASH_SIZE)
    {
        FTD_LOG_INFO("Error: StartAddress/length is too long\n");
        return 0;
    }
    // FTD_LOG_INFO(" SpiFlash_NormalPageProgram 111(StartAddress:0x%x length:%d)",StartAddress,length);
    // /CS: active
    SPI_SET_SS_LOW(SPI_FLASH_PORT);
    // send Command: 0x06, Write enable
    SPI_WRITE_TX(SPI_FLASH_PORT, 0x06);
    // wait tx finish
    while (SPI_IS_BUSY(SPI_FLASH_PORT));
    // /CS: de-active
    SPI_SET_SS_HIGH(SPI_FLASH_PORT);
    // /CS: active
    SPI_SET_SS_LOW(SPI_FLASH_PORT);
    // send Command: 0x02, Page program
    SPI_WRITE_TX(SPI_FLASH_PORT, 0x02);
    // send 24-bit start address
    SPI_WRITE_TX(SPI_FLASH_PORT, (StartAddress >> 16) & 0xFF);
    SPI_WRITE_TX(SPI_FLASH_PORT, (StartAddress >> 8) & 0xFF);
    SPI_WRITE_TX(SPI_FLASH_PORT, StartAddress & 0xFF);

    // write data
    for (writtenLen = 0; writtenLen < length; writtenLen++)
    {
        // 
        while (SPI_GET_TX_FIFO_FULL_FLAG(SPI_FLASH_PORT))
        {
        }

        SPI_WRITE_TX(SPI_FLASH_PORT, u8DataBuffer[writtenLen]);

        if ((writtenLen & 0x7) == 0x7)
        {
            while (SPI_IS_BUSY(SPI_FLASH_PORT));
        }
    }

    // wait tx finish
    while (SPI_IS_BUSY(SPI_FLASH_PORT));
    // /CS: de-active
    SPI_SET_SS_HIGH(SPI_FLASH_PORT);
    SPI_ClearRxFIFO(SPI_FLASH_PORT);

    return length;
}

static int8_t SpiFlash_NormalRead(uint32_t StartAddress, uint32_t length, uint8_t* u8DataBuffer)
{
    uint32_t i;
    if (StartAddress + length > FLASH_SIZE)
    {
        FTD_LOG_INFO("Error: StartAddress/length error in SpiFlash_NormalRead\n");
        return -1;
    }
    // /CS: active
    SPI_SET_SS_LOW(SPI_FLASH_PORT);

    // send Command: 0x03, Read data
    SPI_WRITE_TX(SPI_FLASH_PORT, 0x03);

    // send 24-bit start address
    SPI_WRITE_TX(SPI_FLASH_PORT, (StartAddress >> 16) & 0xFF);
    SPI_WRITE_TX(SPI_FLASH_PORT, (StartAddress >> 8) & 0xFF);
    SPI_WRITE_TX(SPI_FLASH_PORT, StartAddress & 0xFF);

    while (SPI_IS_BUSY(SPI_FLASH_PORT));
    // clear RX buffer
    SPI_ClearRxFIFO(SPI_FLASH_PORT);

    // read data
    for (i = 0; i < length; i++)
    {
        SPI_WRITE_TX(SPI_FLASH_PORT, 0x00);
        while (SPI_IS_BUSY(SPI_FLASH_PORT));
        u8DataBuffer[i] = SPI_READ_RX(SPI_FLASH_PORT);
    }
    // wait tx finish
    while (SPI_IS_BUSY(SPI_FLASH_PORT));

    // /CS: de-active
    SPI_SET_SS_HIGH(SPI_FLASH_PORT);

    return 0;
}

/**
 * @brief Erase a single sector in Flash memory
 *
 * This function erases one sector (ERASE_SIZE_MIN size, typically 4KB) in the Flash memory
 * starting from the specified address. The address must be aligned to the sector boundary.
 * It sends the sector erase command (0x20) along with the 24-bit address to the Flash chip.
 * Before erasing, it sends the write enable command (0x06) to allow the erase operation.
 *
 * @param StartAddress      The starting address of the sector to erase (must be sector-aligned)
 *
 * @return None
 */
static void SpiFlash_erase_sector(uint32_t StartAddress)
{
    if ((StartAddress % ERASE_SIZE_MIN) != 0)
    {
        // FTD_LOG_INFO(" ERASE StartAddress error!!");
        return;
    }
    FTD_LOG_TRACE(" SpiFlash_erase_sector StartAddress(0x%x)", StartAddress);
    // /CS: active
    SPI_SET_SS_LOW(SPI_FLASH_PORT);

    // send Command: 0x06, Write enable
    SPI_WRITE_TX(SPI_FLASH_PORT, 0x06);

    // wait tx finish
    while (SPI_IS_BUSY(SPI_FLASH_PORT));

    // /CS: de-active
    SPI_SET_SS_HIGH(SPI_FLASH_PORT);

    // /CS: active
    SPI_SET_SS_LOW(SPI_FLASH_PORT);

    // send Command: 0x20, sector/4K Erase
    SPI_WRITE_TX(SPI_FLASH_PORT, 0x20);
    SPI_WRITE_TX(SPI_FLASH_PORT, (StartAddress >> 16) & 0xFF);
    SPI_WRITE_TX(SPI_FLASH_PORT, (StartAddress >> 8) & 0xFF);
    SPI_WRITE_TX(SPI_FLASH_PORT, StartAddress & 0xFF);
    // wait tx finish
    while (SPI_IS_BUSY(SPI_FLASH_PORT));

    // /CS: de-active
    SPI_SET_SS_HIGH(SPI_FLASH_PORT);

    SPI_ClearRxFIFO(SPI_FLASH_PORT);
}

static void SpiFlash_erase_block_64K(uint32_t StartAddress)
{
    if ((StartAddress % ERASE_SIZE_MIN) != 0)
    {
        // FTD_LOG_INFO(" ERASE StartAddress error!!");
        return;
    }
    FTD_LOG_TRACE(" SpiFlash_erase_sector StartAddress(0x%x)", StartAddress);
    // /CS: active
    SPI_SET_SS_LOW(SPI_FLASH_PORT);

    // send Command: 0x06, Write enable
    SPI_WRITE_TX(SPI_FLASH_PORT, 0x06);

    // wait tx finish
    while (SPI_IS_BUSY(SPI_FLASH_PORT));

    // /CS: de-active
    SPI_SET_SS_HIGH(SPI_FLASH_PORT);

    // /CS: active
    SPI_SET_SS_LOW(SPI_FLASH_PORT);

    // send Command: 0xD8, 64K Erase
    SPI_WRITE_TX(SPI_FLASH_PORT, 0xD8);
    SPI_WRITE_TX(SPI_FLASH_PORT, (StartAddress >> 16) & 0xFF);
    SPI_WRITE_TX(SPI_FLASH_PORT, (StartAddress >> 8) & 0xFF);
    SPI_WRITE_TX(SPI_FLASH_PORT, StartAddress & 0xFF);
    // wait tx finish
    while (SPI_IS_BUSY(SPI_FLASH_PORT));

    // /CS: de-active
    SPI_SET_SS_HIGH(SPI_FLASH_PORT);

    SPI_ClearRxFIFO(SPI_FLASH_PORT);
}

/*==============================Flash Basic Operation Functions End ===================================*/

uint32_t ftd_drv_flash_access_get_partition_start_addr(PARTITION_NAME e_partition)
{
    if (e_partition >= PARTITION_MAX)
        return 0;

    return st_partition_table[e_partition].start_addr;
}

uint32_t ftd_drv_flash_access_get_partition_size(PARTITION_NAME e_partition)
{
    if (e_partition >= PARTITION_MAX)
        return 0;

    return st_partition_table[e_partition].size;
}

/**
 * @brief Erase Flash memory sectors with optimized 64K block erase
 *
 * This function erases Flash memory starting from the specified address.
 * Erase strategy:
 * 1. If total erase length >= 128KB (2x64K), use optimized mode:
 *    - First use 4K sector erase to align address to 64K boundary
 *    - Then use 64K block erase for the aligned middle portion
 *    - Finally use 4K sector erase for remaining bytes
 * 2. If total erase length < 128KB, use 4K sector erase only
 *
 * @param StartAddress      The starting address of the memory area to erase (must be 4K sector-aligned)
 * @param length            Number of bytes to erase
 *
 * @return
 *     - 0: Erase operation successful
 *     - -1: Error occurred (start address not aligned to ERASE_SIZE_MIN)
 *     - -2: Error occurred (length is 0)
 */
static int8_t ftd_drv_flash_access_erase(uint32_t StartAddress, uint32_t length)
{
    uint32_t current_addr = StartAddress;
    uint32_t end_addr = StartAddress + length;
    uint32_t i = 0;
    const uint32_t ERASE_SIZE_64K = 0x10000; // 64KB
    const uint32_t ERASE_THRESHOLD_USE_64K = ERASE_SIZE_64K * 2; // 128KB threshold

    // Check start address alignment to 4K sector
    if (StartAddress % ERASE_SIZE_MIN)
    {
        FTD_LOG_ERROR("Erase Error: StartAddress 0x%08X not aligned to 4K", StartAddress);
        return -1;
    }

    // Check length validity
    if (length == 0)
    {
        FTD_LOG_ERROR("Erase Error: length is 0");
        return -2;
    }

    // Round up length to 4K boundary if not aligned
    if (length % ERASE_SIZE_MIN)
    {
        length = ((length / ERASE_SIZE_MIN) + 1) * ERASE_SIZE_MIN;
        end_addr = StartAddress + length;
    }



    // Strategy: use 64K block erase only if total length >= 128KB
    if (length >= ERASE_THRESHOLD_USE_64K)
    {
        // Calculate the next 64K aligned address
        uint32_t next_64k_aligned = ((StartAddress + ERASE_SIZE_64K - 1) / ERASE_SIZE_64K) * ERASE_SIZE_64K;

        // If start address is already 64K aligned, next_64k_aligned equals StartAddress
        if (next_64k_aligned == StartAddress)
        {
            next_64k_aligned = StartAddress; // Already aligned
        }

        // Calculate the last 64K aligned address within the erase range
        uint32_t last_64k_aligned = (end_addr / ERASE_SIZE_64K) * ERASE_SIZE_64K;

        // Step 1: Use 4K erase to align to 64K boundary (head padding)
        if (current_addr < next_64k_aligned && next_64k_aligned <= end_addr)
        {
            uint32_t head_4k_count = (next_64k_aligned - current_addr) / ERASE_SIZE_MIN;
            for (i = 0; i < head_4k_count; i++)
            {
                SpiFlash_WaitReady();
                SpiFlash_erase_sector(current_addr + i * ERASE_SIZE_MIN);
            }
            current_addr = next_64k_aligned;
        }

        // Step 2: Use 64K block erase for the aligned middle portion
        if (current_addr < last_64k_aligned)
        {
            uint32_t block_64k_count = (last_64k_aligned - current_addr) / ERASE_SIZE_64K;
            for (i = 0; i < block_64k_count; i++)
            {
                SpiFlash_WaitReady();
                SpiFlash_erase_block_64K(current_addr + i * ERASE_SIZE_64K);
            }
            current_addr = last_64k_aligned;
        }

        // Step 3: Use 4K erase for remaining bytes (tail padding)
        if (current_addr < end_addr)
        {
            uint32_t tail_4k_count = (end_addr - current_addr) / ERASE_SIZE_MIN;
            for (i = 0; i < tail_4k_count; i++)
            {
                SpiFlash_WaitReady();
                SpiFlash_erase_sector(current_addr + i * ERASE_SIZE_MIN);
            }
        }
    }
    else
    {
        // Length < 128KB, use 4K sector erase only
        uint32_t sector_count = length / ERASE_SIZE_MIN;
        for (i = 0; i < sector_count; i++)
        {
            SpiFlash_WaitReady();
            SpiFlash_erase_sector(StartAddress + i * ERASE_SIZE_MIN);
        }
    }

    return 0;
}

/**
 * @brief Read data from Flash memory using normal SPI read operation
 *
 * This function reads data from Flash memory using standard SPI read commands.
 * It calls the low-level SpiFlash_NormalRead function to perform the actual read operation.
 *
 * @param StartAddress      The starting address in Flash memory to read from
 * @param length            Number of bytes to read
 * @param u8DataBuffer      Pointer to the destination buffer where read data will be stored
 *
 * @return
 *     - 0: Read operation successful
 *     - -1: Error occurred during read operation (address/length out of range)
 */
static int8_t ftd_drv_flash_access_read(uint32_t StartAddress, uint32_t length, uint8_t* u8DataBuffer)
{
    return SpiFlash_NormalRead(StartAddress, length, u8DataBuffer);
}

/**
 * @brief Write data to Flash memory with page programming
 *
 * This function writes data to Flash memory using page programming operations. It handles
 * both full page writes and partial page writes. Before writing, it erases the corresponding
 * sectors. The write address must be page-aligned (multiple of PAGESIZE).
 *
 * @param u32FlashAddress   The starting address in Flash memory to write to (must be page-aligned)
 * @param writeLen          Number of bytes to write
 * @param pu8Data           Pointer to the source data buffer to be written
 *
 * @return
 *     - 0: Write operation successful
 *     - -1: Invalid parameters (address out of range, not page-aligned, or page count exceeds limit)
 */
static int8_t ftd_drv_flash_access_write(uint32_t u32FlashAddress, uint32_t writeLen, uint8_t* pu8Data)
{
    uint32_t u32Page = 0;
    uint32_t u32WittenLength = 0;
    uint32_t u32PageNumber = writeLen / PAGESIZE;
    uint32_t u32Remainder = writeLen % PAGESIZE;

    if ((u32PageNumber > flash_page_number) || ((u32FlashAddress + writeLen) >= FLASH_SIZE) \
        || ((u32FlashAddress % PAGESIZE) != 0))
        return -1;

    SpiFlash_WaitReady();
    for (u32Page = 0; u32Page < u32PageNumber; u32Page++)
    {
        u32WittenLength = u32Page * PAGESIZE;
        SpiFlash_erase_sector(u32FlashAddress + u32WittenLength);//SpiFlash_erase_sector
        SpiFlash_WaitReady();
        SpiFlash_NormalPageProgram(u32FlashAddress + u32WittenLength, PAGESIZE, &pu8Data[u32WittenLength]);
        SpiFlash_WaitReady();
    }

    if (u32Remainder != 0)
    {
        u32WittenLength = u32PageNumber * PAGESIZE; //��һ�������û�з�ҳ
        SpiFlash_erase_sector(u32FlashAddress + u32WittenLength);//SpiFlash_erase_sector
        SpiFlash_WaitReady();
        SpiFlash_NormalPageProgram(u32FlashAddress + u32WittenLength, u32Remainder, &pu8Data[u32WittenLength]);
        SpiFlash_WaitReady();
    }

    return 0;
}

/**
 * @brief Read data from Flash using PDMA (Peripheral Direct Memory Access)
 *
 * This function initiates a PDMA-based read operation from the Flash memory. It first sends
 * the read command and address over SPI, then configures and starts the PDMA channels for
 * efficient data transfer. The function sets up separate PDMA channels for SPI TX (to send
 * dummy clocks) and SPI RX (to receive data from Flash).
 *
 * @param u32FlashAddress       The starting address in Flash memory to read from
 * @param trans_len             Number of bytes to read
 * @param pu8Data               Pointer to the destination buffer where read data will be stored
 * @param dummy_data            Pointer to a dummy data buffer used for SPI TX generate clk during read operation
 *
 * @return None
 */
static void ftd_drv_flash_access_read_pdma(uint32_t u32FlashAddress, uint16_t trans_len, uint8_t* pu8Data, uint8_t* dummy_data)
{
    // /CS: active
    SPI_SET_SS_LOW(SPI_FLASH_PORT);

    // send Command: 0x03, Read data
    SPI_WRITE_TX(SPI_FLASH_PORT, 0x03);

    // send 24-bit start address
    SPI_WRITE_TX(SPI_FLASH_PORT, (u32FlashAddress >> 16) & 0xFF);
    SPI_WRITE_TX(SPI_FLASH_PORT, (u32FlashAddress >> 8) & 0xFF);
    SPI_WRITE_TX(SPI_FLASH_PORT, u32FlashAddress & 0xFF);

    while (SPI_IS_BUSY(SPI_FLASH_PORT));
    // clear RX buffer
    SPI_ClearRxFIFO(SPI_FLASH_PORT);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /* Enable PDMA IRQ */
    NVIC_EnableIRQ(PDMA_IRQn);

    PDMA_CLR_TD_FLAG(PDMA, 1 << FLASH_TX_PDMA_CH);
    PDMA_Open(PDMA, 1 << FLASH_TX_PDMA_CH);
    PDMA_SetTransferMode(PDMA, FLASH_TX_PDMA_CH, FLASH_SPI_PDMA_TX, 0, 0);
    PDMA_SetTransferCnt(PDMA, FLASH_TX_PDMA_CH, PDMA_WIDTH_8, trans_len);
    PDMA_SetTransferAddr(PDMA, FLASH_TX_PDMA_CH,
        (uint32_t)dummy_data, PDMA_SAR_INC,
        (uint32_t)&SPI_FLASH_PORT->TX, PDMA_DAR_FIX);


    PDMA_CLR_TD_FLAG(PDMA, 1 << FLASH_RX_PDMA_CH);
    PDMA_Open(PDMA, 1 << FLASH_RX_PDMA_CH);
    PDMA_SetTransferMode(PDMA, FLASH_RX_PDMA_CH, FLASH_SPI_PDMA_RX, 0, 0);
    PDMA_SetTransferCnt(PDMA, FLASH_RX_PDMA_CH, PDMA_WIDTH_8, trans_len);
    PDMA_SetTransferAddr(PDMA, FLASH_RX_PDMA_CH,
        (uint32_t)&SPI_FLASH_PORT->RX, PDMA_SAR_FIX,
        (uint32_t)pu8Data, PDMA_DAR_INC);

    // if (trans_len >= 16 && (trans_len % 16) == 0)
    // {
    //     PDMA_SetBurstType(PDMA, FLASH_TX_PDMA_CH, PDMA_REQ_BURST, PDMA_BURST_16);
    //     PDMA_SetBurstType(PDMA, FLASH_RX_PDMA_CH, PDMA_REQ_BURST, PDMA_BURST_16);
    // } else
    {
        PDMA_SetBurstType(PDMA, FLASH_TX_PDMA_CH, PDMA_REQ_SINGLE, 0);
        PDMA_SetBurstType(PDMA, FLASH_RX_PDMA_CH, PDMA_REQ_SINGLE, 0);
    }

    SPI_TRIGGER_TX_PDMA(SPI_FLASH_PORT);
    SPI_TRIGGER_RX_PDMA(SPI_FLASH_PORT);
}

/**
 * @brief Check if PDMA flash read operation is complete
 *
 * This function checks the PDMA transfer status to determine if the flash read operation
 * via PDMA has completed. It monitors both TX and RX PDMA channels and performs cleanup
 * operations when the transfer is finished.
 *
 * @param None
 * @return bool - Returns true if PDMA transfer is complete, false otherwise
 */
bool ftd_drv_flash_access_pdma_read_is_complete(void)
{
    uint32_t pdma_status = PDMA_GET_TD_STS(PDMA);
    bool rx_complete = (pdma_status & (1 << FLASH_RX_PDMA_CH)) ? true : false;
    bool tx_complete = (pdma_status & (1 << FLASH_TX_PDMA_CH)) ? true : false;

    FTD_LOG_INFO("PDMA status: 0x%08X, RX:%d, TX:%d\n", pdma_status, rx_complete, tx_complete);

    if (rx_complete && tx_complete)
    {
        PDMA_CLR_TD_FLAG(PDMA, (1 << FLASH_RX_PDMA_CH) | (1 << FLASH_TX_PDMA_CH));

        SPI_DISABLE_RX_PDMA(SPI_FLASH_PORT);
        SPI_DISABLE_TX_PDMA(SPI_FLASH_PORT);

        while (SPI_IS_BUSY(SPI_FLASH_PORT));

        // /CS: de-active
        SPI_SET_SS_HIGH(SPI_FLASH_PORT);

        PDMA_Close(PDMA);

        FTD_LOG_INFO("PDMA transfer completed successfully\n");
        return true;
    }

    return false;
}


/**
 * @brief Initialize Flash access functionality
 *
 * This function initializes the SPI interface required for communication with the Flash chip,
 * including clock enabling, pin configuration, SPI mode setting, etc. It identifies the specific
 * Flash model by reading the manufacturer and device ID from the Flash, and sets the corresponding
 * page count parameter accordingly.
 *
 * @param None
 * @return None
 */
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
    { SPI_FLASH_CLK_PIN_SET }
    /* Configure SPI_FLASH_PORT as a master, MSB first, 8-bit transaction, SPI Mode-0 timing, clock is 20MHz */
    SPI_Open(SPI_FLASH_PORT, SPI_MASTER, SPI_MODE_0, 8, 20000000);
    /* Disable auto SS function, control SS signal manually. */
    SPI_DisableAutoSS(SPI_FLASH_PORT);
    /* Lock protected registers */
    SYS_LockReg();

    u16ID = SpiFlash_ReadMidDid();
    if (u16ID == 0xEF13)
    {
        FTD_LOG_INFO("Flash found: W25Q80 ...\n");
        flash_page_number = 4096;
    }
    else if (u16ID == 0xEF14)
    {
        FTD_LOG_INFO("Flash found: W25Q16 ...\n");
        flash_page_number = 8192;
    }
    else if (u16ID == 0xEF15)
    {
        FTD_LOG_INFO("Flash found: W25Q32 ...\n");
        flash_page_number = 16384;
    }
    else if (u16ID == 0xEF16)
    {
        FTD_LOG_INFO("Flash found: W25Q64 ...\n");
        flash_page_number = 32768;
    }
    else if (u16ID == 0xEF17)
    {
        FTD_LOG_INFO("Flash found: W25Q128 ...\n");
        flash_page_number = 65536;
    }
    else
    {
        FTD_LOG_INFO("Wrong ID, 0x%x\n", u16ID);
        flash_page_number = 0;
    }

    // dump all partition addr & size
#if 0 //(LOG_TYPE >= LOG_LEVEL_DEBUG)
    for (uint8_t i = 0; i < PARTITION_MAX; i++)
    {
        FTD_LOG_DEBUG("Partition_table[%02d]: start_addr=0x%08x, size=0x%08x\n", i, st_partition_table[i].start_addr, st_partition_table[i].size);
    }
#endif
}

/**
 * @brief Perform specified operations (erase, write, read, etc.) on Flash partitions
 *
 * @param e_partition       The partition name to operate on, must be within valid range
 * @param ope_type          Operation type, including erase, write, read, etc.
 * @param addr_offset       Offset address within the partition，addr_offset with erase operation must be aligned to minimum erase unit
 * @param length            Length of data to operate on
 * @param pu8Data           Pointer to data buffer, used for write or read operations
 * @param pdma_read_dummy_data Auxiliary data pointer for PDMA read operations, only used in PDMA reading
 *
 * @return
 *     - 0: Operation successful
 *     - -1: Partition index out of range
 *     - -2: Operation length exceeds partition size
 *     - -3: Erase operation address not aligned to minimum erase unit
 *     - -4: Write operation address not page-aligned
 *     - -5: Low-level operation returned error
 */
int8_t ftd_drv_flash_operation(PARTITION_NAME e_partition, PATITION_OPERATION_TYPE ope_type,
    uint32_t addr_offset, uint32_t length, uint8_t* pu8Data, uint8_t* pdma_read_dummy_data)
{
    static uint8_t init_flg = 0;
    int8_t ret = 0;
    uint32_t addr_temp = 0;

    if (init_flg == 0)
    {
        ftd_drv_flash_access_init();
        init_flg = 1;
    }

    if (e_partition > PARTITION_MAX)
    {
        FTD_LOG_ERROR("Flash Op: Invalid partition %d", e_partition);
        ret = -1;
        return ret;
    }

    // Check if operation exceeds partition boundary
    if (length > st_partition_table[e_partition].size)
    {
        FTD_LOG_ERROR("Flash Op: length 0x%08X exceeds partition size 0x%08X",
            length, st_partition_table[e_partition].size);
        ret = -2;
        return ret;
    }

    // Check if addr_offset + length exceeds partition boundary (prevent overflow)
    // if (addr_offset + length > st_partition_table[e_partition].size)
    // {
    //     FTD_LOG_ERROR("Flash Op: addr_offset(0x%08X) + length(0x%08X) = 0x%08X exceeds partition size 0x%08X",
    //         addr_offset, length, addr_offset + length, st_partition_table[e_partition].size);
    //     ret = -2;
    //     return ret;
    // }

    if (ope_type == OPERATION_ERASE)
    {
        addr_temp = addr_offset % ERASE_SIZE_MIN;
        if (addr_temp != 0)
        {
            ret = -3;
            return ret;
        }
    }

    if ((ope_type == OPERATION_WRITE) && ((addr_offset % PAGESIZE) != 0))
    {
        ret = -4;
        return ret;
    }

    switch (ope_type)
    {
        case OPERATION_ERASE: //if erase len < ERASE_SIZE_MIN,will be erase 1 sector
            ret = ftd_drv_flash_access_erase(st_partition_table[e_partition].start_addr + addr_offset, length);
            break;
        case OPERATION_WRITE:
            ret = ftd_drv_flash_access_write(st_partition_table[e_partition].start_addr + addr_offset, length, pu8Data);
            break;
        case OPERATION_READ:
            ret = ftd_drv_flash_access_read(st_partition_table[e_partition].start_addr + addr_offset, length, pu8Data);
            break;
        case OPERATION_READ_PDMA:
            ftd_drv_flash_access_read_pdma(st_partition_table[e_partition].start_addr + addr_offset, length, pu8Data, pdma_read_dummy_data);
            break;
        default:
            break;
    }

    if (ret < 0)
        return -5;
    else
        return 0;
}

void flashPdmaReadTest(void)
{
    uint16_t i;
    uint8_t pu8Data_tx[1024];
    uint8_t pu8Data_dummy[1024];
    uint8_t pu8Data_rx[1024];

    for (i = 0;i < 1024;i++)
    {
        pu8Data_tx[i] = i % 256;
    }

    FTD_LOG_INFO(" flash pdma read test start\n");
    ftd_drv_flash_operation(PARTITION_FW2_BIN, OPERATION_WRITE, 0, 1024, pu8Data_tx, NULL);
    FTD_LOG_INFO(" flash pdma read test write finish\n");

    for (i = 0;i < 512;i++)
    {
        ftd_drv_flash_operation(PARTITION_FW2_BIN, OPERATION_READ_PDMA, i * 2, 1024, pu8Data_rx, pu8Data_dummy);
        while (!ftd_drv_flash_access_pdma_read_is_complete());
        FTD_LOG_INFO(" read1 [0x%x][0x%x][0x%x][0x%x][0x%x]\n", pu8Data_rx[0], pu8Data_rx[1], pu8Data_rx[2], pu8Data_rx[3], pu8Data_rx[4]);
    }

}
