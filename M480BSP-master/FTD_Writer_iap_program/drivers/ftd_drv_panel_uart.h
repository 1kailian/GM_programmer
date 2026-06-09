#ifndef __FTD_DRV_PANEL_UART_H__
#define __FTD_DRV_PANEL_UART_H__

#include <stdint.h>
#include <stdbool.h>
#include "NuMicro.h"

#define UART5_TX_PDMA_CH       (12)
#define UART5_RX_PDMA_CH       (13)

void ftd_drv_panel_uart_rx_start(uint8_t *buf, uint16_t len);
void ftd_drv_panel_uart_tx_start(uint8_t *buf, uint16_t len);
bool ftd_drv_panel_uart_is_tx_complete(void);
bool ftd_drv_panel_uart_is_tx_fifo_empty(void);
bool ftd_drv_panel_uart_is_rx_complete(void);
bool ftd_drv_panel_uart_is_rx_fifo_empty(void);

#endif
