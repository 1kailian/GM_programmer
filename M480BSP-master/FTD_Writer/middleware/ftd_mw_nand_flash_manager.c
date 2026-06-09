#include "ftd_mw_nand_flash_manager.h"
#include "ftd_mw_log_manager.h"

//Based on the system delay function "us delay", the generated "ms delay" function
void ftd_mw_w25n01kv_delay_ms(uint16_t ms)
{
    CLK_SysTickDelay(ms * 1000);
}
/**
* @brief ：void ftd_mw_w25n01kv_delay_ms(uint16_t ms);
* function：delay
* parameter：ms
* return：none
*/
void (*ftd_mw_w25n01kv_delay)(unsigned short ms) = (void (*)(unsigned short)) & ftd_mw_w25n01kv_delay_ms;

/**
* @brief ：unsigned char ftd_mw_w25n01kv_rw(unsigned char dat);
* function：SPI read and write 1 byte
* parameter：dat data to be written
* return：data read out
*/
unsigned char (*ftd_mw_w25n01kv_rw)(unsigned char dat) = &ftd_drv_nand_flash_qspi_byte;

#define SPI_CS SPI_CS_FUN

/**
* @brief ：void ftd_mw_w25n01kv_enable(unsigned char s);
* @function：SPI chip select
* @parameter：s chip select status, 0 not selected, non-zero selected
* @return：none
*/
void ftd_mw_w25n01kv_enable(unsigned char s)
{
    ftd_drv_nand_flash_qspi_cs(s);
}


unsigned char ftd_mw_w25n01kv_reset(void)
{
    ftd_mw_w25n01kv_enable(1);
    ftd_mw_w25n01kv_rw(FTD_MW_W25N01KV_CMD_DEVICE_RESET);
    ftd_mw_w25n01kv_enable(0);
    ftd_mw_w25n01kv_delay(1);//manual says reset time about 5-500us, here delay 1ms
    if (ftd_mw_w25n01kv_wait_busy(100))
    {
        return 1;//reset timeout waiting
    }
    return 0;
}

unsigned char ftd_mw_w25n01kv_init(void)
{
    unsigned int id = 0;

    ftd_drv_nand_flash_qspi_init();
    ftd_mw_w25n01kv_enable(1);
    ftd_mw_w25n01kv_rw(FTD_MW_W25N01KV_CMD_JEDEC_ID);
    id |= ftd_mw_w25n01kv_rw(0x00) << 24;//first byte ignored
    id |= ftd_mw_w25n01kv_rw(0x00) << 16;//second byte is 0xEF
    id |= ftd_mw_w25n01kv_rw(0x00) << 8;//third byte is 0xAA
    id |= ftd_mw_w25n01kv_rw(0x00);//fourth byte is 0x21
    ftd_mw_w25n01kv_enable(0);
    FTD_LOG_INFO("Flash ID: 0x%08X", id);
    if ((id & FTD_MW_W25N01KV_FLASH_ID) != FTD_MW_W25N01KV_FLASH_ID)
    {
        // chip model is wrong, or chip is damaged?
        return 1;
    }

    if (ftd_mw_w25n01kv_read_status_register(FTD_MW_W25N01KV_CMD_STATUS_REG3_ADDR) & FTD_MW_W25N01KV_SR_BBM_LUT_FULL)
    {
        // bad block mapping table is full, continue using it cannot guarantee the correctness of data
        return 2;
    }

    // Set chip configuration
    id = ftd_mw_w25n01kv_read_status_register(FTD_MW_W25N01KV_CMD_STATUS_REG1_ADDR);
    id = 0;//disable all write protection
    ftd_mw_w25n01kv_write_status_register(FTD_MW_W25N01KV_CMD_STATUS_REG1_ADDR, id & 0x000000FF);

    id = ftd_mw_w25n01kv_read_status_register(FTD_MW_W25N01KV_CMD_STATUS_REG2_ADDR);
    id |= (0x10 | 0x80);//enable ECC, disable read buffer
    ftd_mw_w25n01kv_write_status_register(FTD_MW_W25N01KV_CMD_STATUS_REG2_ADDR, id & 0x000000FF);

    return 0;
}

// wait for chip operation to complete, time unit ms
unsigned char ftd_mw_w25n01kv_wait_busy(unsigned short int timeout)
{
    while (timeout--)
    {
        if ((ftd_mw_w25n01kv_read_status_register(FTD_MW_W25N01KV_CMD_STATUS_REG3_ADDR) & FTD_MW_W25N01KV_SR_BUSY) != FTD_MW_W25N01KV_SR_BUSY)
        {
            return 0;//wait end
        }
        ftd_mw_w25n01kv_delay(1);
    }
    return 1;//wait timeout
}

void ftd_mw_w25n01kv_write_enable(void)
{
    ftd_mw_w25n01kv_enable(1);
    ftd_mw_w25n01kv_rw(FTD_MW_W25N01KV_CMD_WRITE_ENABLE);
    ftd_mw_w25n01kv_enable(0);
}

void ftd_mw_w25n01kv_write_disable(void)
{
    ftd_mw_w25n01kv_enable(1);
    ftd_mw_w25n01kv_rw(FTD_MW_W25N01KV_CMD_WRITE_DISABLE);
    ftd_mw_w25n01kv_enable(0);
}

unsigned char ftd_mw_w25n01kv_read_status_register(unsigned char RegAddr)
{
    unsigned char tmp;

    ftd_mw_w25n01kv_enable(1);
    ftd_mw_w25n01kv_rw(FTD_MW_W25N01KV_CMD_READ_STATUS_REGISTER);
    ftd_mw_w25n01kv_rw(RegAddr);
    tmp = ftd_mw_w25n01kv_rw(0x00);
    ftd_mw_w25n01kv_enable(0);

    return tmp;
}

void ftd_mw_w25n01kv_write_status_register(unsigned char RegAddr, unsigned char Value)
{
    ftd_mw_w25n01kv_enable(1);
    ftd_mw_w25n01kv_rw(FTD_MW_W25N01KV_CMD_WRITE_STATUS_REGISTER);
    ftd_mw_w25n01kv_rw(RegAddr);
    ftd_mw_w25n01kv_rw(Value);
    ftd_mw_w25n01kv_enable(0);
}

// read a page, return value is ECC check result
unsigned char ftd_mw_w25n01kv_read_page(unsigned short int page, unsigned char* buf)
{
    unsigned short int i = 0;

    // input the page address to read
    ftd_mw_w25n01kv_enable(1);
    ftd_mw_w25n01kv_rw(FTD_MW_W25N01KV_CMD_PAGE_DATA_READ);
    ftd_mw_w25n01kv_rw(0x00);
    ftd_mw_w25n01kv_rw((unsigned char)(page >> 8));
    ftd_mw_w25n01kv_rw((unsigned char)(page & 0x00FF));
    ftd_mw_w25n01kv_enable(0);
    ftd_mw_w25n01kv_delay(1);

    // start reading data
    ftd_mw_w25n01kv_enable(1);
    ftd_mw_w25n01kv_rw(FTD_MW_W25N01KV_CMD_READ);
    ftd_mw_w25n01kv_rw(0x00);
    ftd_mw_w25n01kv_rw(0x00);
    ftd_mw_w25n01kv_rw(0x00);
    // three empty bytes followed by data reception
    for (i = 0; i < 2048; i++)
    {
        *buf = ftd_mw_w25n01kv_rw(0x00);
        buf++;
    }
    ftd_mw_w25n01kv_enable(0);

    return ftd_mw_w25n01kv_read_status_register(FTD_MW_W25N01KV_CMD_STATUS_REG3_ADDR) & 0x30;
}

// write a page, return value is 0 success, 1 fail
unsigned char ftd_mw_w25n01kv_write_page(unsigned short int page, unsigned char* buf)
{
    unsigned short int i = 0;

    ftd_mw_w25n01kv_write_enable();
    ftd_mw_w25n01kv_delay(1);

    // load data into Flash
    ftd_mw_w25n01kv_enable(1);
    ftd_mw_w25n01kv_rw(FTD_MW_W25N01KV_CMD_PROGRAM_DATA_LOAD);
    ftd_mw_w25n01kv_rw(0x00);//fill byte
    ftd_mw_w25n01kv_rw(0x00);//fill byte
    for (i = 0; i < 2048; i++)
    {
        ftd_mw_w25n01kv_rw(*buf);
        buf++;
    }
    ftd_mw_w25n01kv_enable(0);
    ftd_mw_w25n01kv_delay(1);

    // execute write operation
    ftd_mw_w25n01kv_enable(1);
    ftd_mw_w25n01kv_rw(FTD_MW_W25N01KV_CMD_PROGRAM_EXECUTE);
    ftd_mw_w25n01kv_rw(0x00);
    ftd_mw_w25n01kv_rw((unsigned char)(page >> 8));
    ftd_mw_w25n01kv_rw((unsigned char)(page & 0x00FF));
    ftd_mw_w25n01kv_enable(0);

    ftd_mw_w25n01kv_wait_busy(100);//等待写入完成

    // check if write is successful
    if (ftd_mw_w25n01kv_read_status_register(FTD_MW_W25N01KV_CMD_STATUS_REG3_ADDR) & FTD_MW_W25N01KV_SR_WRITE_FAIL)
    {
        return 1;
    }
    return 0;
}

// erase block, 0 success, 1 fail
unsigned char ftd_mw_w25n01kv_block_erase(unsigned short int BlockNum)
{
    ftd_mw_w25n01kv_write_enable();
    ftd_mw_w25n01kv_enable(1);
    ftd_mw_w25n01kv_rw(FTD_MW_W25N01KV_CMD_BLOCK_ERASE);
    ftd_mw_w25n01kv_rw((unsigned char)(0x00));
    ftd_mw_w25n01kv_rw((unsigned char)(BlockNum >> 8));
    ftd_mw_w25n01kv_rw((unsigned char)(BlockNum & 0x00FF));
    ftd_mw_w25n01kv_enable(0);
    if (ftd_mw_w25n01kv_wait_busy(100))
    {
        // wait for erase to complete, manual says maximum not exceeding 10ms
        return 1;
    }

    // check if erase is successful
    if (ftd_mw_w25n01kv_read_status_register(FTD_MW_W25N01KV_CMD_STATUS_REG3_ADDR) & FTD_MW_W25N01KV_SR_ERASE_FAIL)
    {
        return 1;
    }
    return 0;
}

// block scan, can be used to check if erase is successful or to scan for factory bad block marks
unsigned char ftd_mw_w25n01kv_block_scan(unsigned short int BlockNum)
{
    int i = 0;

    // convert block address to page address
    BlockNum <<= 6;

    ftd_mw_w25n01kv_enable(1);
    ftd_mw_w25n01kv_rw(FTD_MW_W25N01KV_CMD_PAGE_DATA_READ);
    ftd_mw_w25n01kv_rw(0x00);
    ftd_mw_w25n01kv_rw((unsigned char)(BlockNum >> 8));
    ftd_mw_w25n01kv_rw((unsigned char)(BlockNum & 0x00FF));
    ftd_mw_w25n01kv_enable(0);

    ftd_mw_w25n01kv_delay(1);
    // start reading
    ftd_mw_w25n01kv_enable(1);
    ftd_mw_w25n01kv_rw(FTD_MW_W25N01KV_CMD_READ);
    ftd_mw_w25n01kv_rw(0x00);
    ftd_mw_w25n01kv_rw(0x00);
    ftd_mw_w25n01kv_rw(0x00);
    // three empty bytes followed by data reception
    for (i = 0; i < 128 * 1024; i++)
    {
        if (ftd_mw_w25n01kv_rw(0x00) != 0xFF)
        {
            ftd_mw_w25n01kv_enable(0);
            return 1;
        }
    }
    ftd_mw_w25n01kv_enable(0);

    return 0;
}

// special used to find the bad block marks at the time of factory, the manual says that the first byte of the first page of the bad block is marked as non-0xFF, and the rest does not need to continue scanning
unsigned char ftd_mw_w25n01kv_bad_block_scan(unsigned short int BlockNum)
{
    // convert block address to page address
    BlockNum <<= 6;

    ftd_mw_w25n01kv_enable(1);
    ftd_mw_w25n01kv_rw(FTD_MW_W25N01KV_CMD_PAGE_DATA_READ);
    ftd_mw_w25n01kv_rw(0x00);
    ftd_mw_w25n01kv_rw((unsigned char)(BlockNum >> 8));
    ftd_mw_w25n01kv_rw((unsigned char)(BlockNum & 0x00FF));
    ftd_mw_w25n01kv_enable(0);

    ftd_mw_w25n01kv_delay(1);
    // start reading
    ftd_mw_w25n01kv_enable(1);
    ftd_mw_w25n01kv_rw(FTD_MW_W25N01KV_CMD_READ);
    ftd_mw_w25n01kv_rw(0x00);
    ftd_mw_w25n01kv_rw(0x00);
    ftd_mw_w25n01kv_rw(0x00);
    // three empty bytes followed by data reception
    // after three empty bytes, start receiving data
    if (ftd_mw_w25n01kv_rw(0x00) != 0xFF)
    {
        ftd_mw_w25n01kv_enable(0);
        return 1;
    }
    ftd_mw_w25n01kv_enable(0);

    return 0;
}

// LBA(Logical Block Address) is bad block address, PBA(Physical Block Address) is the block to be replaced
// return 0 normal, 1 bad block table full
unsigned char ftd_mw_w25n01kv_bad_block_mapping(unsigned short int LBA, unsigned short int PBA)
{
    if (ftd_mw_w25n01kv_read_status_register(FTD_MW_W25N01KV_CMD_STATUS_REG3_ADDR) & FTD_MW_W25N01KV_SR_BBM_LUT_FULL)
    {
        return 1;
    }

    ftd_mw_w25n01kv_enable(1);
    ftd_mw_w25n01kv_rw(FTD_MW_W25N01KV_CMD_BAD_BLOCK_MANAGEMENT);
    ftd_mw_w25n01kv_rw((unsigned char)(LBA >> 8));
    ftd_mw_w25n01kv_rw((unsigned char)(LBA & 0x00FF));
    ftd_mw_w25n01kv_rw((unsigned char)(PBA >> 8));
    ftd_mw_w25n01kv_rw((unsigned char)(PBA & 0x00FF));
    ftd_mw_w25n01kv_enable(0);

    return 0;
}

// Bad Block Management Look Up Table
// BBMLUT needs 80 bytes of storage (actually should be 40 unsigned short data)
// format is LBA0,PBA0,LBA1,PBA1......LBA19,PBA19
void ftd_mw_w25n01kv_read_bbm_lut(unsigned short int* BBMLUT)
{
    int i = 0;

    ftd_mw_w25n01kv_enable(1);
    ftd_mw_w25n01kv_rw(FTD_MW_W25N01KV_CMD_READ_BBM_LUT);
    ftd_mw_w25n01kv_rw(0x00);
    for (i = 0; i < 80; i++)
    {
        *BBMLUT = ftd_mw_w25n01kv_rw(0x00) << 8;
        BBMLUT++;
        *BBMLUT |= ftd_mw_w25n01kv_rw(0x00);
    }
    ftd_mw_w25n01kv_enable(0);
}

// read out the last ECC failure page address
unsigned short int ftd_mw_w25n01kv_last_ecc_fail_page_addr(void)
{
    unsigned short int PageAddr = 0;

    ftd_mw_w25n01kv_enable(1);
    ftd_mw_w25n01kv_rw(FTD_MW_W25N01KV_CMD_LAST_ECC_FAILURE_PAGE);
    ftd_mw_w25n01kv_rw(0x00);
    PageAddr |= ftd_mw_w25n01kv_rw(0x00) << 8;
    PageAddr |= ftd_mw_w25n01kv_rw(0x00);
    ftd_mw_w25n01kv_enable(0);

    return PageAddr;
}

//It is recommended to perform a bad block scan before its official use.
// use before formal use, do not write any data to Flash before use, otherwise it may cover the bad block information at the time of factory
void ftd_mw_w25n01kv_bad_block_manage(void)
{
    unsigned short int i = 0, n = 0;
    unsigned short int BBT[40] = { 0 };//Bad Block Table

    JYMC_LOG_INFO("Start bad block scan \r\n");
    for (i = 0; i < 1024; i++)
    {
        JYMC_LOG_INFO("Scanning block %d \r\n", i);
        if (ftd_mw_w25n01kv_bad_block_scan(i))// quick scan, only scan the first byte of each page
        {
            JYMC_LOG_INFO("Block %d is bad \r\n", i);
            // automatically fill in the mapping table or fill manually?

            if (ftd_mw_w25n01kv_read_status_register(FTD_MW_W25N01KV_CMD_STATUS_REG3_ADDR) & FTD_MW_W25N01KV_SR_BBM_LUT_FULL)
            {
                JYMC_LOG_INFO("Bad block mapping table is full, cannot map \r\n");
            }
            else
            {
                ftd_mw_w25n01kv_read_bbm_lut(BBT);
                // A total of 24 blocks ranging from 1000 to 1023 are used for replacement, 
                // and the BBMLUT can store a total of 20 mapping relationships.
                // 24 blocks from 1000 to 1023 are used for replacement, BBMLUT can only save 20 mapping relationships
                // 4 extra blocks are considered to account for the possibility of bad blocks among these 20 blocks
                for (n = 0; n < 20; n++)
                {
                    if (BBT[n * 2] == 0x0000 && BBT[n * 2 + 1] == 0x0000)
                    {
                        n += 1000;
                        ftd_mw_w25n01kv_bad_block_mapping(i, n);
                        JYMC_LOG_INFO("Block %d is mapped to block %d \r\n", i, n);
                        break;
                    }
                }
            }
        }
    }
    JYMC_LOG_INFO("Bad block scan completed \r\n");
}

/**
 * @brief NAND Flash test function
 * Test: init, erase, write, read, verify
 */
void ftd_mw_w25n01kv_test(void)
{
    unsigned char ret = 0;
    unsigned short test_page = 100;
    unsigned short test_block = 50;
    unsigned char test_data[2048];
    unsigned char read_data[2048];
    unsigned short i = 0;

    JYMC_LOG_INFO("\n================== NAND Flash Test Start ==================\r\n");

    /* Test 1: Initialize */
    JYMC_LOG_INFO("[TEST 1] Initialization\r\n");
    ret = ftd_mw_w25n01kv_init();
    if (ret != 0)
    {
        JYMC_LOG_INFO("[FAIL] Initialization failed, error code: %d\r\n", ret);
        return;
    }
    JYMC_LOG_INFO("[PASS] Initialization success\r\n");

    //ftd_mw_w25n01kv_bad_block_manage();
    /* Test 2: Prepare test data */
    JYMC_LOG_INFO("[TEST 2] Prepare test data\r\n");
    for (i = 0; i < 2048; i++)
    {
        test_data[i] = (unsigned char)(i & 0xFF);
    }
    JYMC_LOG_INFO("[PASS] Test pattern created (0x00-0xFF cycling)\r\n");

    /* Test 3: Erase block */
    JYMC_LOG_INFO("[TEST 3] Erase block %d\r\n", test_block);
    ret = ftd_mw_w25n01kv_block_erase(test_block);
    if (ret != 0)
    {
        JYMC_LOG_INFO("[FAIL] Block erase failed\r\n");
        return;
    }
    JYMC_LOG_INFO("[PASS] Block erase success\r\n");

    /* Test 4: Write page */
    JYMC_LOG_INFO("[TEST 4] Write page %d (2048 bytes)\r\n", test_page);
    ret = ftd_mw_w25n01kv_write_page(test_page, test_data);
    if (ret != 0)
    {
        JYMC_LOG_INFO("[FAIL] Page write failed\r\n");
        return;
    }
    JYMC_LOG_INFO("[PASS] Page write success\r\n");

    /* Test 5: Read page */
    JYMC_LOG_INFO("[TEST 5] Read page %d\r\n", test_page);
    ret = ftd_mw_w25n01kv_read_page(test_page, read_data);
    if (ret & FTD_MW_W25N01KV_SR_ECC_MORE_THAN_4BIT_ERR)
    {
        JYMC_LOG_INFO("[FAIL] ECC error detected\r\n");
        return;
    }
    JYMC_LOG_INFO("[PASS] Page read success\r\n");

    /* Test 6: Verify data */
    JYMC_LOG_INFO("[TEST 6] Verify data integrity\r\n");
    unsigned char match_ok = 1;
    for (i = 0; i < 2048; i++)
    {
        if (read_data[i] != test_data[i])
        {
            JYMC_LOG_INFO("[FAIL] Mismatch at byte %d (expected 0x%02X, got 0x%02X)\r\n",
                i, test_data[i], read_data[i]);
            match_ok = 0;
            break;
        }
    }

    if (match_ok)
    {
        JYMC_LOG_INFO("[PASS] All 2048 bytes verified correctly\r\n");
    }
    else
    {
        JYMC_LOG_INFO("[FAIL] Data verification failed\r\n");
        return;
    }

    /* Test 7: Bad block check */
    JYMC_LOG_INFO("[TEST 7] Bad block check\r\n");
    ret = ftd_mw_w25n01kv_bad_block_scan(test_block);
    if (ret)
    {
        JYMC_LOG_INFO("[WARN] Block %d is marked as bad\r\n", test_block);
    }
    else
    {
        JYMC_LOG_INFO("[PASS] Block %d is good\r\n", test_block);
    }

    JYMC_LOG_INFO("\n================= NAND Flash Test Complete ==================\r\n");
    JYMC_LOG_INFO("[SUCCESS] All tests passed!\r\n");
    JYMC_LOG_INFO("==========================================================\r\n\n");
}
