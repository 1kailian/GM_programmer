#ifndef __FTD_DRV_HC_UART_H__
#define __FTD_DRV_HC_UART_H__ 
#include "NuMicro.h"
#include <stdint.h>

#define PROG_COMMS_UART         UART0
#define PROG_COMMS_UART_MOUDLE  UART0_MODULE
#define PROG_COMMS_UART_RX_PDMA_CH  (1)
#define PROG_COMMS_UART_TX_PDMA_CH  (15)
#define PROG_COMMS_UART_PDMA_INT_STATE_RX     (0x200) //channel 0
#define PROG_COMMS_UART_PDMA_INT_STATE_TX     (0x100) //channel 1

#define PROG_COMMS_UART_IRQ         UART0_IRQn
#define PROG_COMMS_UART_BASE        UART0_BASE

#define PROG_COMMS_UART_CLK_INIT    { \
                                        CLK_EnableModuleClock(PROG_COMMS_UART_MOUDLE);\
                                        CLK_SetModuleClock(PROG_COMMS_UART_MOUDLE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1)); \
                                        CLK->APBCLK0 |= CLK_APBCLK0_UART0CKEN_Msk;\
                                        CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_UART0SEL_Msk) | (0x0 << CLK_CLKSEL1_UART0SEL_Pos); \
                                    }

#define PROG_COMMS_UART_PDMA_RX     PDMA_UART0_RX
#define PROG_COMMS_UART_PDMA_TX     PDMA_UART0_TX


//ref jymc uart protocol
#define UART_CMD_LEN_MIN (8)
//programmer 0x54 pkt data max define 1024!!! must sync with pc, 20 contain cmd header,length and so on 
#define PROG_COMMS_BUFF_LEN (1024+20)

typedef enum dma_comms_state
{
    DMA_COMMS_STATE_START,
    DMA_COMMS_STATE_ABORT,
    DMA_COMMS_STATE_TIMEOUT,
    DMA_COMMS_STATE_DONE,
} DMA_COMMS_STATE;

void ftd_drv_hc_uart_init(void);
void ftd_drv_hc_uart_send_sync(uint8_t *p_buf, uint32_t len);
uint8_t ftd_drv_hc_uart_receive(uint8_t *p_buf);
// uint8_t ftd_drv_hc_uart_receive(uint8_t **p_p_buf);


#endif 
