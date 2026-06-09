/****************************************************************************
@FILENAME:  ftd_drv_hc_uart.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.15
@COPYRIGHT: FTD co.ltd
****************************************************************************/

#include "ftd_drv_hc_uart.h"
#include "ftd_mw_log_manager.h"
#include <stdio.h>
#include "NuMicro.h"
#include <string.h>
#include <stdbool.h> 

#define PDMA_TIME (0x5555)


/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/

static uint8_t g_rx_buffer[PROG_COMMS_BUFF_LEN];
// Used to cache the content that PDMA places in g_rx_buffer and then return it to other modules
// static uint8_t g_rec_buffer[PROG_COMMS_BUFF_LEN];

volatile DMA_COMMS_STATE en_tx_dma_state = DMA_COMMS_STATE_START;
volatile DMA_COMMS_STATE en_rx_dma_state = DMA_COMMS_STATE_START;

/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA_IRQHandler(void);
void UART_PDMATest(void);

void PDMA_IRQHandler(void)
{
    uint32_t status = PDMA_GET_INT_STATUS(PDMA);
    JYMC_LOG_INFO("status[0x%x] pdma[0x%x]\n", status, PDMA_GET_TD_STS(PDMA));

    if (status & 0x1)   /* abort */
    {
        JYMC_LOG_INFO("target abort interrupt !!\n");
        if (PDMA_GET_ABORT_STS(PDMA) & 0x4)
        {
            PDMA_CLR_ABORT_FLAG(PDMA, PDMA_GET_ABORT_STS(PDMA));
        }
    }
    else if (status & 0x2)     /* done */
    {
        if (PDMA_GET_TD_STS(PDMA) & (1 << PROG_COMMS_UART_TX_PDMA_CH))
        {
            en_tx_dma_state = DMA_COMMS_STATE_DONE;
            PDMA_CLR_TD_FLAG(PDMA, 1 << PROG_COMMS_UART_TX_PDMA_CH);
        }

        if (PDMA_GET_TD_STS(PDMA) & (1 << PROG_COMMS_UART_RX_PDMA_CH))
        {
            en_rx_dma_state = DMA_COMMS_STATE_DONE;
            PDMA_CLR_TD_FLAG(PDMA, 1 << PROG_COMMS_UART_RX_PDMA_CH);
        }

        PDMA_CLR_TD_FLAG(PDMA, PDMA_GET_TD_STS(PDMA));

    }
    else if (status & PROG_COMMS_UART_PDMA_INT_STATE_TX)
    {
        en_tx_dma_state = DMA_COMMS_STATE_TIMEOUT;
        PDMA_SetTimeOut(PDMA, PROG_COMMS_UART_TX_PDMA_CH, 0, 0);
        PDMA_CLR_TMOUT_FLAG(PDMA, PROG_COMMS_UART_TX_PDMA_CH);
        PDMA_SetTimeOut(PDMA, PROG_COMMS_UART_TX_PDMA_CH, 1, PDMA_TIME);

        // PDMA_CLR_TMOUT_FLAG(PDMA,PDMA_GET_TD_STS(PDMA));

    }
    else if (status & PROG_COMMS_UART_PDMA_INT_STATE_RX)
    {
        en_rx_dma_state = DMA_COMMS_STATE_TIMEOUT;
        PDMA_SetTimeOut(PDMA, PROG_COMMS_UART_RX_PDMA_CH, 0, 0);
        PDMA_CLR_TMOUT_FLAG(PDMA, PROG_COMMS_UART_RX_PDMA_CH);
        PDMA_SetTimeOut(PDMA, PROG_COMMS_UART_RX_PDMA_CH, 1, PDMA_TIME);

        // PDMA_CLR_TMOUT_FLAG(PDMA,PDMA_GET_TD_STS(PDMA));
    }
    else
    {
    }
}

//unuse
void UART0_IRQHandler(void)
{
    uint32_t u32IntSts = UART0->INTSTS;

    if (u32IntSts & UART_INTSTS_HWRLSIF_Msk)
    {
        if (UART0->FIFOSTS & UART_FIFOSTS_BIF_Msk)
            JYMC_LOG_INFO("\n BIF \n");
        if (UART0->FIFOSTS & UART_FIFOSTS_FEF_Msk)
            JYMC_LOG_INFO("\n FEF \n");
        if (UART0->FIFOSTS & UART_FIFOSTS_PEF_Msk)
            JYMC_LOG_INFO("\n PEF \n");

        UART0->FIFOSTS = (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk);
    }
}

static void ftd_drv_hc_uart_rx_pdma_init(void)
{
    /* Open PDMA Channel */
    PDMA_Open(PDMA, 1 << PROG_COMMS_UART_RX_PDMA_CH); // Channel 1 for UART1 RX
    // Select basic mode
    PDMA_SetTransferMode(PDMA, PROG_COMMS_UART_RX_PDMA_CH, PROG_COMMS_UART_PDMA_RX, 0, 0);
    // Set data width and transfer count
    PDMA_SetTransferCnt(PDMA, PROG_COMMS_UART_RX_PDMA_CH, PDMA_WIDTH_8, PROG_COMMS_BUFF_LEN);
    //Set PDMA Transfer Address
    PDMA_SetTransferAddr(PDMA, PROG_COMMS_UART_RX_PDMA_CH, PROG_COMMS_UART_BASE, PDMA_SAR_FIX, ((uint32_t)(&g_rx_buffer[0])), PDMA_DAR_INC);
    //Select Single Request
    PDMA_SetBurstType(PDMA, PROG_COMMS_UART_RX_PDMA_CH, PDMA_REQ_SINGLE, 0);
    //Set timeout
    /* PDMA channel  timeout*/
    PDMA_SetTimeOut(PDMA, PROG_COMMS_UART_RX_PDMA_CH, 1, PDMA_TIME); /* channel 0 timeout period = CLK * TOC0  */

    /* Enable interrupt */
    PDMA_EnableInt(PDMA, PROG_COMMS_UART_RX_PDMA_CH, PDMA_INT_TRANS_DONE); /* channel transfer done */
    PDMA_EnableInt(PDMA, PROG_COMMS_UART_RX_PDMA_CH, PDMA_INT_TIMEOUT); /*channel timeout */

    PDMA_EnableInt(PDMA, PROG_COMMS_UART_RX_PDMA_CH, 0);
    NVIC_EnableIRQ(PDMA_IRQn);

    NVIC_EnableIRQ(PROG_COMMS_UART_IRQ);

    PROG_COMMS_UART->INTEN |= UART_INTEN_RXPDMAEN_Msk;
}

void ftd_drv_hc_uart_init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    CLK->AHBCLK |= CLK_AHBCLK_PDMACKEN_Msk; // PDMA Clock Enable

    PROG_COMMS_UART_CLK_INIT

        //pin configuer in main

        /* Update System Core Clock */
        SystemCoreClockUpdate();
    /* Lock protected registers */
    SYS_LockReg();

    UART_Open(PROG_COMMS_UART, 115200);

    ftd_drv_hc_uart_rx_pdma_init();

}



/**
 * @brief Send data via UART1 using PDMA (Peripheral Direct Memory Access) and wait for completion.
 *
 * This function configures and starts a PDMA-based UART1 transmission operation.
 * It blocks until the transfer completes, is aborted, or times out, then prints
 * the corresponding status message.
 *
 * @param buf Pointer to the data buffer to be sent.
 * @param len Length of the data to be sent in bytes.
 */
void ftd_drv_hc_uart_send_sync(uint8_t* p_buf, uint32_t len)
{
    JYMC_LOG_INFO("ftd_drv_hc_uart_send_sync\r\n");
    /* Open PDMA Channel */
    PDMA_Open(PDMA, 1 << PROG_COMMS_UART_TX_PDMA_CH); // Channel 0 for UART1 TX
    // Select basic mode
    PDMA_SetTransferMode(PDMA, PROG_COMMS_UART_TX_PDMA_CH, PROG_COMMS_UART_PDMA_TX, 0, 0);
    // Set data width and transfer count
    PDMA_SetTransferCnt(PDMA, PROG_COMMS_UART_TX_PDMA_CH, PDMA_WIDTH_8, len);
    //Set PDMA Transfer Address
    PDMA_SetTransferAddr(PDMA, PROG_COMMS_UART_TX_PDMA_CH, ((uint32_t)(&p_buf[0])), PDMA_SAR_INC, PROG_COMMS_UART_BASE, PDMA_DAR_FIX);
    //Select Single Request
    PDMA_SetBurstType(PDMA, PROG_COMMS_UART_TX_PDMA_CH, PDMA_REQ_SINGLE, 0);

    PDMA_EnableInt(PDMA, PROG_COMMS_UART_TX_PDMA_CH, PDMA_INT_TRANS_DONE);
    PDMA_EnableInt(PDMA, PROG_COMMS_UART_TX_PDMA_CH, PDMA_INT_TIMEOUT);

    NVIC_EnableIRQ(PROG_COMMS_UART_IRQ);

    en_tx_dma_state = DMA_COMMS_STATE_START;

    //enable TX PDMA
    PROG_COMMS_UART->INTEN |= UART_INTEN_TXPDMAEN_Msk;
    PROG_COMMS_UART->INTEN |= UART_INTEN_RXPDMAEN_Msk;

    while (en_tx_dma_state == DMA_COMMS_STATE_START);

    if (en_tx_dma_state == DMA_COMMS_STATE_DONE)
    {
        JYMC_LOG_TRACE("\n UART1 TX transfer done");
    }
    else if (en_tx_dma_state == DMA_COMMS_STATE_ABORT)
    {
        JYMC_LOG_TRACE("\n UART1 TX transfer abort");
    }
    else if (en_tx_dma_state == DMA_COMMS_STATE_TIMEOUT)
    {
        JYMC_LOG_TRACE("\n UART1 TX timeout");
    }
    else
    {
        JYMC_LOG_TRACE("\n UART1 TX transfer error");
    }
    JYMC_LOG_INFO("ftd_drv_hc_uart_send_sync ok\r\n");
    //disable TX PDMA
    PROG_COMMS_UART->INTEN &= ~UART_INTEN_TXPDMAEN_Msk;

    PROG_COMMS_UART->INTEN |= UART_INTEN_RXPDMAEN_Msk;

}

/**
 * @brief Receives communication data and prepares buffer pointer
 *
 * @param p_buf Pointer that will be set to the data buffer address
 *
 * @return uint8_t Data status indicator
 *         - 1: Data is ready and available
 *         - 0: No data available
 */
uint8_t ftd_drv_hc_uart_receive(uint8_t* p_buf)
{
    if ((en_rx_dma_state == DMA_COMMS_STATE_DONE)
        || (en_rx_dma_state == DMA_COMMS_STATE_TIMEOUT))
    {
        memcpy(p_buf, &g_rx_buffer[0], PROG_COMMS_BUFF_LEN);
        en_rx_dma_state = DMA_COMMS_STATE_START;
        memset(g_rx_buffer, 0, PROG_COMMS_BUFF_LEN);

        // // CRITICAL: Restart PDMA transfer for continuous reception
        // PDMA_SetTransferAddr(PDMA, PROG_COMMS_UART_RX_PDMA_CH, PROG_COMMS_UART_BASE, PDMA_SAR_FIX, 
        //                     ((uint32_t)(&g_rx_buffer[0])), PDMA_DAR_INC);
        // PDMA_SetTransferCnt(PDMA, PROG_COMMS_UART_RX_PDMA_CH, PDMA_WIDTH_8, PROG_COMMS_BUFF_LEN);

        // // Ensure RX PDMA is enabled
        // PROG_COMMS_UART->INTEN |= UART_INTEN_RXPDMAEN_Msk;
        ftd_drv_hc_uart_rx_pdma_init();
        return 1; // data ready
    }

    return 0; // no data
}
