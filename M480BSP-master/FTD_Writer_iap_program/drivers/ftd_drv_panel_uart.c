/****************************************************************************
@FILENAME:  ftd_drv_panel_uart.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.24
@COPYRIGHT: FTD co.ltd
****************************************************************************/

#include "ftd_drv_panel_uart.h"
#include "ftd_mw_log_manager.h"
#include "NuMicro.h"
#include "stdbool.h"

#define UART3_RX_PDMA_CH       (4)
#define UART3_TX_PDMA_CH       (8)

#define USE_UART5              (1)

void ftd_drv_panel_uart_rx_start(uint8_t *buf, uint16_t len)
{
#if USE_UART5
    /* Unlock protected registers */
    SYS_UnlockReg();
    UART5->INTEN &= ~(UART_INTEN_RXPDMAEN_Msk);
    PDMA_CLR_TD_FLAG(PDMA, 1 << UART5_RX_PDMA_CH);

    uint32_t timeout = 10000;
    while ((!(UART5->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk)) && (timeout > 0))
    {
        volatile uint32_t dummy = UART5->DAT;
        (void)dummy;
        timeout--;
    }

    CLK->AHBCLK |= CLK_AHBCLK_PDMACKEN_Msk; // PDMA Clock Enable
    CLK_EnableModuleClock(UART5_MODULE);
    CLK_SetModuleClock(UART5_MODULE, CLK_CLKSEL3_UART5SEL_HXT, CLK_CLKDIV4_UART5(1));
    /* Update System Core Clock */
    SystemCoreClockUpdate();
    /* Lock protected registers */
    SYS_LockReg();
    UART_Open(UART5, 115200);
    /* Open PDMA Channel */
    PDMA_Open(PDMA, 1 << UART5_RX_PDMA_CH); // Channel 1 for UART1 RX
    // Select basic mode
    PDMA_SetTransferMode(PDMA, UART5_RX_PDMA_CH, PDMA_UART5_RX, 0, 0);
    // Set data width and transfer count
    PDMA_SetTransferCnt(PDMA, UART5_RX_PDMA_CH, PDMA_WIDTH_8, len);
    //Set PDMA Transfer Address
    PDMA_SetTransferAddr(PDMA, UART5_RX_PDMA_CH, UART5_BASE, PDMA_SAR_FIX, ((uint32_t)(&buf[0])), PDMA_DAR_INC);
    //Select Single Request
    PDMA_SetBurstType(PDMA, UART5_RX_PDMA_CH, PDMA_REQ_SINGLE, 0);
    NVIC_EnableIRQ(UART5_IRQn);
    UART5->INTEN |= UART_INTEN_RXPDMAEN_Msk;
#else
    /* Unlock protected registers */
    SYS_UnlockReg();
    UART3->INTEN &= ~(UART_INTEN_RXPDMAEN_Msk);
    PDMA_CLR_TD_FLAG(PDMA, 1 << UART3_RX_PDMA_CH);

    uint32_t timeout = 10000;
    while ((!(UART3->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk)) && (timeout > 0))
    {
        volatile uint32_t dummy = UART3->DAT;
        (void)dummy;
        timeout--;
    }

    CLK->AHBCLK |= CLK_AHBCLK_PDMACKEN_Msk; // PDMA Clock Enable
    CLK_EnableModuleClock(UART3_MODULE);
    CLK_SetModuleClock(UART3_MODULE, CLK_CLKSEL3_UART3SEL_HXT, CLK_CLKDIV4_UART3(1));
    /* Update System Core Clock */
    SystemCoreClockUpdate();
    /* Lock protected registers */
    SYS_LockReg();
    UART_Open(UART3, 115200);
    /* Open PDMA Channel */
    PDMA_Open(PDMA, 1 << UART3_RX_PDMA_CH); // Channel 1 for UART1 RX
    // Select basic mode
    PDMA_SetTransferMode(PDMA, UART3_RX_PDMA_CH, PDMA_UART3_RX, 0, 0);
    // Set data width and transfer count
    PDMA_SetTransferCnt(PDMA, UART3_RX_PDMA_CH, PDMA_WIDTH_8, len);
    //Set PDMA Transfer Address
    PDMA_SetTransferAddr(PDMA, UART3_RX_PDMA_CH, UART3_BASE, PDMA_SAR_FIX, ((uint32_t)(&buf[0])), PDMA_DAR_INC);
    //Select Single Request
    PDMA_SetBurstType(PDMA, UART3_RX_PDMA_CH, PDMA_REQ_SINGLE, 0);
    NVIC_EnableIRQ(UART3_IRQn);
    UART3->INTEN |= UART_INTEN_RXPDMAEN_Msk;
#endif
}


void ftd_drv_panel_uart_tx_start(uint8_t *buf, uint16_t len)
{
#if USE_UART5
    /* Unlock protected registers */
    SYS_UnlockReg();
    UART5->INTEN &= ~(UART_INTEN_TXPDMAEN_Msk);
    PDMA_CLR_TD_FLAG(PDMA, 1 << UART5_TX_PDMA_CH);

    CLK->AHBCLK |= CLK_AHBCLK_PDMACKEN_Msk; // PDMA Clock Enable
    CLK_EnableModuleClock(UART5_MODULE);
    CLK_SetModuleClock(UART5_MODULE, CLK_CLKSEL3_UART5SEL_HXT, CLK_CLKDIV4_UART5(1));

    /* Update System Core Clock */
    SystemCoreClockUpdate();
    /* Lock protected registers */
    SYS_LockReg();
    UART_Open(UART5, 115200);

    /* Open PDMA Channel */
    PDMA_Open(PDMA, 1 << UART5_TX_PDMA_CH); // Channel 1 for UART1 RX
    // Select basic mode
    PDMA_SetTransferMode(PDMA, UART5_TX_PDMA_CH, PDMA_UART5_TX, 0, 0);
    // Set data width and transfer count
    PDMA_SetTransferCnt(PDMA, UART5_TX_PDMA_CH, PDMA_WIDTH_8, len);
    //Set PDMA Transfer Address
    PDMA_SetTransferAddr(PDMA, UART5_TX_PDMA_CH, ((uint32_t)(&buf[0])), PDMA_SAR_INC, UART5_BASE, PDMA_DAR_FIX);
    //Select Single Request
    PDMA_SetBurstType(PDMA, UART5_TX_PDMA_CH, PDMA_REQ_SINGLE, 0);
    NVIC_EnableIRQ(UART5_IRQn);
    UART5->INTEN |= UART_INTEN_TXPDMAEN_Msk;
#else
    /* Unlock protected registers */
    SYS_UnlockReg();
    UART3->INTEN &= ~(UART_INTEN_TXPDMAEN_Msk);
    PDMA_CLR_TD_FLAG(PDMA, 1 << UART3_TX_PDMA_CH);

    CLK->AHBCLK |= CLK_AHBCLK_PDMACKEN_Msk; // PDMA Clock Enable
    CLK_EnableModuleClock(UART3_MODULE);
    CLK_SetModuleClock(UART3_MODULE, CLK_CLKSEL3_UART3SEL_HXT, CLK_CLKDIV4_UART3(1));

    /* Update System Core Clock */
    SystemCoreClockUpdate();
    /* Lock protected registers */
    SYS_LockReg();
    UART_Open(UART3, 115200);

    /* Open PDMA Channel */
    PDMA_Open(PDMA, 1 << UART3_TX_PDMA_CH); // Channel 1 for UART1 RX
    // Select basic mode
    PDMA_SetTransferMode(PDMA, UART3_TX_PDMA_CH, PDMA_UART3_TX, 0, 0);
    // Set data width and transfer count
    PDMA_SetTransferCnt(PDMA, UART3_TX_PDMA_CH, PDMA_WIDTH_8, len);
    //Set PDMA Transfer Address
    PDMA_SetTransferAddr(PDMA, UART3_TX_PDMA_CH, ((uint32_t)(&buf[0])), PDMA_SAR_INC, UART3_BASE, PDMA_DAR_FIX);
    //Select Single Request
    PDMA_SetBurstType(PDMA, UART3_TX_PDMA_CH, PDMA_REQ_SINGLE, 0);
    NVIC_EnableIRQ(UART3_IRQn);
    UART3->INTEN |= UART_INTEN_TXPDMAEN_Msk;
#endif
}

bool ftd_drv_panel_uart_is_tx_complete(void)
{
#if USE_UART5
    bool tx_complete = false;
    // Check PDMA transfer done status register to determine if transmission is complete
    tx_complete = (PDMA_GET_TD_STS(PDMA) & (1 << UART5_TX_PDMA_CH)) ? true : false;

    if(tx_complete)
        PDMA_CLR_TD_FLAG(PDMA, 1 << UART5_TX_PDMA_CH);
#else
    bool tx_complete = false;
    // Check PDMA transfer done status register to determine if transmission is complete
    tx_complete = (PDMA_GET_TD_STS(PDMA) & (1 << UART3_TX_PDMA_CH)) ? true : false;

    if (tx_complete)
        PDMA_CLR_TD_FLAG(PDMA, 1 << UART3_TX_PDMA_CH);
#endif
    return tx_complete;
}

bool ftd_drv_panel_uart_is_tx_fifo_empty(void)
{
#if USE_UART5
    bool fifo_empty = false;
    fifo_empty = (UART5->FIFOSTS & UART_FIFOSTS_TXEMPTY_Msk) ? true : false;
#else
    bool fifo_empty = false;
    fifo_empty = (UART3->FIFOSTS & UART_FIFOSTS_TXEMPTY_Msk) ? true : false;
#endif
    return fifo_empty;
}

bool ftd_drv_panel_uart_is_rx_complete(void)
{
#if USE_UART5
    bool rx_complete = false;
    // Check PDMA transfer done status register to determine if reception is complete
    rx_complete = (PDMA_GET_TD_STS(PDMA) & (1 << UART5_RX_PDMA_CH)) ? true : false;

    if(rx_complete)
        PDMA_CLR_TD_FLAG(PDMA, 1 << UART5_RX_PDMA_CH);
#else
    bool rx_complete = false;
    // Check PDMA transfer done status register to determine if reception is complete
    rx_complete = (PDMA_GET_TD_STS(PDMA) & (1 << UART3_RX_PDMA_CH)) ? true : false;

    if (rx_complete)
        PDMA_CLR_TD_FLAG(PDMA, 1 << UART3_RX_PDMA_CH);
#endif

    return rx_complete;
}

bool ftd_drv_panel_uart_is_rx_fifo_empty(void)
{
#if USE_UART5
    bool fifo_empty = false;
    fifo_empty = (UART5->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk) ? true : false;
#else
    bool fifo_empty = false;
    fifo_empty = (UART3->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk) ? true : false;
#endif
    return fifo_empty;
}
void ftd_drv_panel_uart_test(void)
{
    uint8_t buff[64];
    uint8_t i = 0;

    for (size_t i = 0; i < 64; i++)
    {
        buff[i] = i;
    }
    
    while (1)
    {
        JYMC_LOG_INFO(" tx start ");
        ftd_drv_panel_uart_tx_start(buff,64);
        while (ftd_drv_panel_uart_is_tx_complete() == false);
        
        JYMC_LOG_INFO(" rx start ");
        ftd_drv_panel_uart_rx_start(buff,8);
        while (ftd_drv_panel_uart_is_rx_complete() == false);
    }
}


/*---------------------------------------------------------------------------------------------------------*/
/* UART IRQ Handler                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#if USE_UART5
void UART5_IRQHandler(void)
{
    uint32_t u32IntSts = UART5->INTSTS;

    if (u32IntSts & UART_INTSTS_HWRLSIF_Msk)
    {
        if (UART5->FIFOSTS & UART_FIFOSTS_BIF_Msk)
            FTD_LOG_TRACE("\n UART5 BIF \n");
        if (UART5->FIFOSTS & UART_FIFOSTS_FEF_Msk)
            FTD_LOG_TRACE("\n UART5 FEF \n");
        if (UART5->FIFOSTS & UART_FIFOSTS_PEF_Msk)
            FTD_LOG_TRACE("\n UART5 PEF \n");

        UART5->FIFOSTS = (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk);
    }
}
#else
void UART3_IRQHandler(void)
{
    uint32_t u32IntSts = UART3->INTSTS;

    if (u32IntSts & UART_INTSTS_HWRLSIF_Msk)
    {
        if (UART3->FIFOSTS & UART_FIFOSTS_BIF_Msk)
            FTD_LOG_TRACE("\n UART3 BIF \n");
        if (UART3->FIFOSTS & UART_FIFOSTS_FEF_Msk)
            FTD_LOG_TRACE("\n UART3 FEF \n");
        if (UART3->FIFOSTS & UART_FIFOSTS_PEF_Msk)
            FTD_LOG_TRACE("\n UART3 PEF \n");

        UART3->FIFOSTS = (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk);
    }
}
#endif