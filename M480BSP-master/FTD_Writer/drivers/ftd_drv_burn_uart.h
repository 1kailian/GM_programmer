#ifndef __FTD_MW_BURN_UART_H__
#define __FTD_MW_BURN_UART_H__

#include <stdint.h>
#include <stdbool.h>
#include "NuMicro.h"

#define DEFAULT_BAUDRATE  (115200)
#define TRANSFER_BAUDRATE (460800)

// Constants
#define BURN_UART_CHANNELS     (4)
#define BURN_UART_DATA_MAX     (1024+20)
// PDMA Channel Definitions
#define UART1_RX_PDMA_CH       (2)
#define UART2_RX_PDMA_CH       (3)
#define UART3_RX_PDMA_CH       (4)
#define UART4_RX_PDMA_CH       (5)
#define UART1_TX_PDMA_CH       (6)
#define UART2_TX_PDMA_CH       (7)
#define UART3_TX_PDMA_CH       (8)
#define UART4_TX_PDMA_CH       (9)

// Function Declarations
/**
 * @brief Initialize all burn UART channels (UART1-UART4)
 * @note This function MUST be called once at system startup before using any UART functions.
 *       It initializes all UART peripherals and PDMA clock.
 *       Call this function only ONCE during system initialization.
 */
void ftd_drv_burn_uart_init(uint32_t baudrate);

void ftd_drv_burn_uart_deinit(void);

void ftd_drv_burn_uart_single_init(uint8_t channel, uint32_t baudrate);

void ftd_drv_burn_uart_single_deinit(uint8_t channel);

/**
 * Start UART transmission on the specified channel
 * @param channel UART channel number (0-3 corresponding to UART1-UART4)
 * @param buf Pointer to the data buffer containing the data to be transmitted
 * @param len Length of data to be transmitted in bytes
 * 
 * @note The function uses PDMA for efficient data transfer without CPU intervention.
 *       Use ftd_drv_burn_channel_is_tx_complete() to check transmission completion status.
 * 
 * @see ftd_drv_burn_channel_is_tx_complete()
 */
void ftd_drv_burn_uart_tx_start(uint8_t channel, uint8_t *buf, uint16_t len);

/**
 * Start UART reception on the specified channel
 * @param channel UART channel number (0-3 corresponding to UART1-UART4)
 * @param buf Pointer to the data buffer where received data will be stored
 * @param len Length of data to be received in bytes
 * 
 * @note The function uses PDMA for efficient data transfer without CPU intervention.
 *       Use ftd_drv_burn_channel_is_rx_complete() to check reception completion status.
 * 
 * @see ftd_drv_burn_channel_is_rx_complete()
 */
void ftd_drv_burn_uart_rx_start(uint8_t channel, uint8_t *buf, uint16_t len);

/**
 * Stop UART reception on the specified channel
 * @param channel UART channel number (0-3 corresponding to UART1-UART4)
 * @param buf Pointer to the data buffer where received data will be stored
 * @param len Length of data to be received in bytes
 *
 * @note The function to stop PDMA for efficient data transfer without CPU intervention.
 */
void ftd_drv_burn_uart_rx_stop(uint8_t channel);

/**
 * Check if the transmission on the specified UART channel is complete
 * 
 * @param channel UART channel number (0-3 corresponding to UART1-UART4)
 * @return true: transmission complete, false: transmission not complete or invalid channel
 */
bool ftd_drv_burn_channel_is_rx_complete(uint8_t channel);

/**
 * Check if the reception on the specified UART channel is complete
 * 
 * @param channel UART channel number (0-3 corresponding to UART1-UART4)
 * @return true: reception complete, false: reception not complete or invalid channel
 */
bool ftd_drv_burn_channel_is_tx_complete(uint8_t channel);

#endif
