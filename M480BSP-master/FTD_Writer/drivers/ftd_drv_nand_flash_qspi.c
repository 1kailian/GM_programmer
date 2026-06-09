/****************************************************************************
@FILENAME:  ftd_drv_nand_flash_qspi.c
@FUNCTION:  NAND Flash QSPI Hardware Driver Implementation
@AUTHOR:    chenkailian
@DATE:      2025.1.3
@COPYRIGHT: FTD co.ltd
****************************************************************************/

#include "ftd_drv_nand_flash_qspi.h"
#include "ftd_mw_log_manager.h"

/**
 * @brief Initialize QSPI hardware interface
 */
void ftd_drv_nand_flash_qspi_init(void)
{
    static uint8_t s_qspi_init_flag = 0;

    if (s_qspi_init_flag == 0)
    {
        /* Unlock protected registers */
        SYS_UnlockReg();

        /* Enable QSPI0 module clock */
        CLK_EnableModuleClock(FTD_DRV_QSPI_MODULE_CLK);

        /* Configure QSPI0 as Master mode */
        /* Parameters: Port, Mode (Master/Slave), QSPI Mode, Data Width, Clock Frequency */
        QSPI_Open(FTD_DRV_QSPI_PORT, QSPI_MASTER, FTD_DRV_QSPI_MODE,
            FTD_DRV_QSPI_DATA_WIDTH, FTD_DRV_QSPI_CLK_FREQ);

        /* CS idle state = High (Inactive) */
        FTD_DRV_QSPI_CS_DISABLE();

        /* Lock protected registers */
        SYS_LockReg();

        s_qspi_init_flag = 1;

        FTD_LOG_INFO("QSPI0 initialized: Master mode, Mode 0, 8-bit, %d Hz\n",
            FTD_DRV_QSPI_CLK_FREQ);
    }
}

/**
 * @brief Control QSPI Chip Select pin
 */
void ftd_drv_nand_flash_qspi_cs(uint8_t en)
{
    if (en)
    {
        /* CS Enable: Pull CS low (Active Low) */
        FTD_DRV_QSPI_CS_ENABLE();
    }
    else
    {
        /* CS Disable: Pull CS high (Idle High) */
        FTD_DRV_QSPI_CS_DISABLE();
    }
}

/**
 * @brief Write and read a single byte via QSPI (Full-duplex)
 *
 * This is the fundamental QSPI operation following Nuvoton's API:
 * 1. Write byte to TX buffer
 * 2. Wait for transmission to complete
 * 3. Read byte from RX buffer
 */
uint8_t ftd_drv_nand_flash_qspi_byte(uint8_t tx_data)
{
    /* Write transmit data to QSPI */
    QSPI_WRITE_TX(FTD_DRV_QSPI_PORT, tx_data);

    /* Wait for transmission to complete */
    ftd_drv_nand_flash_qspi_wait_busy();

    /* Read received data from QSPI */
    return (uint8_t)QSPI_READ_RX(FTD_DRV_QSPI_PORT);
}

/**
 * @brief Write multiple bytes via QSPI
 *
 * Following Nuvoton's sample code pattern:
 * - Check TX FIFO full flag before writing
 * - Wait for transmission completion
 * - Clear RX FIFO to discard received data
 */
void ftd_drv_nand_flash_qspi_write(const uint8_t* data, uint32_t len)
{
    uint32_t i = 0;

    if (data == NULL || len == 0)
        return;

    /* Write data bytes to TX buffer */
    while (i < len)
    {
        /* Check if TX FIFO is not full */
        if (!ftd_drv_nand_flash_qspi_tx_full())
        {
            QSPI_WRITE_TX(FTD_DRV_QSPI_PORT, data[i++]);
        }
    }

    /* Wait for all transmissions to complete */
    ftd_drv_nand_flash_qspi_wait_busy();

    /* Clear RX FIFO since we don't use the received data */
    ftd_drv_nand_flash_qspi_clear_rxfifo();
}

/**
 * @brief Read multiple bytes via QSPI
 *
 * Following Nuvoton's sample code pattern:
 * - Send dummy bytes (0xFF) to generate clock for reading
 * - Wait for each transmission
 * - Read received data byte by byte
 */
void ftd_drv_nand_flash_qspi_read(uint8_t* data, uint32_t len)
{
    uint32_t i = 0;

    if (data == NULL || len == 0)
        return;

    /* Send dummy bytes and read data */
    for (i = 0; i < len; i++)
    {
        /* Write dummy byte (0xFF) to generate clock */
        QSPI_WRITE_TX(FTD_DRV_QSPI_PORT, 0xFF);

        /* Wait for transmission to complete */
        ftd_drv_nand_flash_qspi_wait_busy();

        /* Read received data */
        data[i] = QSPI_READ_RX(FTD_DRV_QSPI_PORT);
    }
}

/**
 * @brief Write and read multiple bytes via QSPI (Full-duplex)
 *
 * Simultaneous transmit and receive operation.
 * Useful for memory-to-memory transfers or commands with response.
 */
void ftd_drv_nand_flash_qspi_xfer(const uint8_t* tx_data, uint8_t* rx_data, uint32_t len)
{
    uint32_t i = 0;

    if (tx_data == NULL || rx_data == NULL || len == 0)
        return;

    /* Send data and receive simultaneously */
    for (i = 0; i < len; i++)
    {
        /* Write transmit byte */
        QSPI_WRITE_TX(FTD_DRV_QSPI_PORT, tx_data[i]);

        /* Wait for transmission to complete */
        ftd_drv_nand_flash_qspi_wait_busy();

        /* Read received byte */
        rx_data[i] = QSPI_READ_RX(FTD_DRV_QSPI_PORT);
    }
}

