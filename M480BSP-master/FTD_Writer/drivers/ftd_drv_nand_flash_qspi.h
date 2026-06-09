/****************************************************************************
@FILENAME:  ftd_drv_nand_flash_qspi.h
@FUNCTION:  NAND Flash QSPI Hardware Driver Interface
@AUTHOR:    yanxijiang
@DATE:      2025.12.23
@COPYRIGHT: FTD co.ltd
****************************************************************************/

#ifndef __FTD_DRV_NAND_FLASH_QSPI_H__
#define __FTD_DRV_NAND_FLASH_QSPI_H__

#include "NuMicro.h"
#include <stdint.h>
#include <stdbool.h>

/* QSPI Port Configuration */
#define FTD_DRV_QSPI_PORT           QSPI0
#define FTD_DRV_QSPI_MODE           QSPI_MODE_0
#define FTD_DRV_QSPI_DATA_WIDTH     8
#define FTD_DRV_QSPI_CLK_FREQ       10000000    /* 10 MHz */
#define FTD_DRV_QSPI_MODULE_CLK     QSPI0_MODULE

/* CS Control Macros (Use Nuvoton's built-in macros) */
#define FTD_DRV_QSPI_CS_ENABLE()    QSPI_SET_SS_LOW(FTD_DRV_QSPI_PORT)
#define FTD_DRV_QSPI_CS_DISABLE()   QSPI_SET_SS_HIGH(FTD_DRV_QSPI_PORT)

/**
 * @brief Initialize QSPI hardware interface
 *
 * Configures:
 * - QSPI0 module clock
 * - QSPI0 as Master mode
 * - QSPI mode 0, 8-bit width
 * - CS control via Nuvoton's QSPI_SET_SS_LOW/HIGH macros
 * - Clock frequency: 10 MHz
 *
 * @return void
 */
void ftd_drv_nand_flash_qspi_init(void);

/**
 * @brief Control QSPI Chip Select pin
 *
 * @param en: 1 = CS Enable (Active Low), 0 = CS Disable (Idle High)
 *
 * @note CS is active low, so en=1 pulls CS low
 * @return void
 */
void ftd_drv_nand_flash_qspi_cs(uint8_t en);

/**
 * @brief Write and read a single byte via QSPI (Full-duplex)
 *
 * This is the fundamental QSPI operation. Sends one byte and
 * simultaneously receives one byte.
 *
 * @param tx_data: Byte to transmit (0xFF for read-only operation)
 *
 * @return Received byte from QSPI
 */
uint8_t ftd_drv_nand_flash_qspi_byte(uint8_t tx_data);

/**
 * @brief Write multiple bytes via QSPI
 *
 * Sends data bytes and discards received data.
 * Used for command and address transmission.
 *
 * @param data: Pointer to data buffer to send
 * @param len: Number of bytes to send
 *
 * @return void
 */
void ftd_drv_nand_flash_qspi_write(const uint8_t* data, uint32_t len);

/**
 * @brief Read multiple bytes via QSPI
 *
 * Sends dummy bytes (0xFF) and captures received data.
 * Used for reading data from NAND Flash.
 *
 * @param data: Pointer to buffer for received data
 * @param len: Number of bytes to read
 *
 * @return void
 */
void ftd_drv_nand_flash_qspi_read(uint8_t* data, uint32_t len);

/**
 * @brief Write and read multiple bytes via QSPI (Full-duplex)
 *
 * Simultaneously transmits and receives data.
 * Useful for memory-to-memory transfers.
 *
 * @param tx_data: Pointer to transmit buffer
 * @param rx_data: Pointer to receive buffer
 * @param len: Number of bytes to transfer
 *
 * @return void
 */
void ftd_drv_nand_flash_qspi_xfer(const uint8_t* tx_data, uint8_t* rx_data, uint32_t len);

/**
 * @brief Wait for QSPI to complete transmission
 *
 * Waits until QSPI hardware finishes transmitting current data.
 * Blocking operation.
 *
 * @return void
 */
static inline void ftd_drv_nand_flash_qspi_wait_busy(void)
{
    while (QSPI_IS_BUSY(FTD_DRV_QSPI_PORT));
}

/**
 * @brief Clear QSPI RX FIFO
 *
 * Clears any residual data in the RX FIFO buffer.
 * Useful after status reads or unused operations.
 *
 * @return void
 */
static inline void ftd_drv_nand_flash_qspi_clear_rxfifo(void)
{
    QSPI_ClearRxFIFO(FTD_DRV_QSPI_PORT);
}

/**
 * @brief Check if RX FIFO is empty
 *
 * @return 1 if RX FIFO is empty, 0 otherwise
 */
static inline uint8_t ftd_drv_nand_flash_qspi_rx_empty(void)
{
    return QSPI_GET_RX_FIFO_EMPTY_FLAG(FTD_DRV_QSPI_PORT);
}

/**
 * @brief Check if TX FIFO is full
 *
 * @return 1 if TX FIFO is full, 0 otherwise
 */
static inline uint8_t ftd_drv_nand_flash_qspi_tx_full(void)
{
    return QSPI_GET_TX_FIFO_FULL_FLAG(FTD_DRV_QSPI_PORT);
}

#endif

