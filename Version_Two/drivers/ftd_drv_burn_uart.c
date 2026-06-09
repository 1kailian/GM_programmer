/****************************************************************************
@FILENAME:  ftd_drv_burn_uart.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.24
@COPYRIGHT: FTD co.ltd
****************************************************************************/

#include "ftd_drv_burn_uart.h"
#include "ftd_mw_log_manager.h"
#include "NuMicro.h"

static void ftd_drv_burn_channel1_rx_stop(void);
static void ftd_drv_burn_channel2_rx_stop(void);
static void ftd_drv_burn_channel3_rx_stop(void);
static void ftd_drv_burn_channel4_rx_stop(void);

// UART initialization flag to prevent re-initialization


/**
 * @brief Initialize all burn UART channels (UART1-UART4)
 * @note This function should be called once at system startup
 *       Do NOT call UART_Open() in rx_start/tx_start to prevent data loss
 */
void ftd_drv_burn_uart_init(uint32_t baudrate)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    // Enable PDMA clock
    CLK->AHBCLK |= CLK_AHBCLK_PDMACKEN_Msk;

    // Initialize UART1
    CLK_EnableModuleClock(UART1_MODULE);
    CLK_SetModuleClock(UART1_MODULE, CLK_CLKSEL1_UART1SEL_HXT, CLK_CLKDIV0_UART1(1));
    UART_Open(UART1, baudrate);
    NVIC_EnableIRQ(UART1_IRQn);

    // Initialize UART2
    CLK_EnableModuleClock(UART2_MODULE);
    CLK_SetModuleClock(UART2_MODULE, CLK_CLKSEL3_UART2SEL_HXT, CLK_CLKDIV4_UART2(1));
    UART_Open(UART2, baudrate);
    NVIC_EnableIRQ(UART2_IRQn);

    // Initialize UART3
    CLK_EnableModuleClock(UART3_MODULE);
    CLK_SetModuleClock(UART3_MODULE, CLK_CLKSEL3_UART3SEL_HXT, CLK_CLKDIV4_UART3(1));
    UART_Open(UART3, baudrate);
    NVIC_EnableIRQ(UART3_IRQn);

    // Initialize UART4
    CLK_EnableModuleClock(UART4_MODULE);
    CLK_SetModuleClock(UART4_MODULE, CLK_CLKSEL3_UART4SEL_HXT, CLK_CLKDIV4_UART4(1));
    UART_Open(UART4, baudrate);
    NVIC_EnableIRQ(UART4_IRQn);

    /* Update System Core Clock */
    SystemCoreClockUpdate();
    /* Lock protected registers */
    SYS_LockReg();

    ftd_drv_burn_channel1_rx_stop();
    ftd_drv_burn_channel2_rx_stop();
    ftd_drv_burn_channel3_rx_stop();
    ftd_drv_burn_channel4_rx_stop();

    FTD_LOG_INFO("Burn UART initialized (UART1-UART4)\n");
}

void ftd_drv_burn_uart_deinit(void)
{
    UART_Close(UART1);
    UART_Close(UART2);
    UART_Close(UART3);
    UART_Close(UART4);

    FTD_LOG_INFO("Burn UART deinitialized (UART1-UART4)\n");
}

void ftd_drv_burn_uart_single_init(uint8_t channel, uint32_t baudrate)
{
    SYS_UnlockReg();
    switch(channel)
    {
        case 0:
        CLK_EnableModuleClock(UART1_MODULE);
        CLK_SetModuleClock(UART1_MODULE, CLK_CLKSEL1_UART1SEL_HXT, CLK_CLKDIV0_UART1(1));
        UART_Open(UART1, baudrate);
        NVIC_EnableIRQ(UART1_IRQn);
            /* Update System Core Clock */
        SystemCoreClockUpdate();
        /* Lock protected registers */
        SYS_LockReg();
        ftd_drv_burn_channel1_rx_stop();
        break;
        case 1:
        CLK_EnableModuleClock(UART2_MODULE);
        CLK_SetModuleClock(UART2_MODULE, CLK_CLKSEL3_UART2SEL_HXT, CLK_CLKDIV4_UART2(1));
        UART_Open(UART2, baudrate);
        NVIC_EnableIRQ(UART2_IRQn);
        /* Update System Core Clock */
        SystemCoreClockUpdate();
        /* Lock protected registers */
        SYS_LockReg();
        ftd_drv_burn_channel2_rx_stop();
        break;
        case 2:
        CLK_EnableModuleClock(UART3_MODULE);
        CLK_SetModuleClock(UART3_MODULE, CLK_CLKSEL3_UART3SEL_HXT, CLK_CLKDIV4_UART3(1));
        UART_Open(UART3, baudrate);
        NVIC_EnableIRQ(UART3_IRQn);
        /* Update System Core Clock */
        SystemCoreClockUpdate();
        /* Lock protected registers */
        SYS_LockReg();
        ftd_drv_burn_channel3_rx_stop();
        break;
        case 3:
        CLK_EnableModuleClock(UART4_MODULE);
        CLK_SetModuleClock(UART4_MODULE, CLK_CLKSEL3_UART4SEL_HXT, CLK_CLKDIV4_UART4(1));
        UART_Open(UART4, baudrate);
        NVIC_EnableIRQ(UART4_IRQn);
            /* Update System Core Clock */
        SystemCoreClockUpdate();
        /* Lock protected registers */
        SYS_LockReg();
        ftd_drv_burn_channel4_rx_stop();
        break;
    }

}

void ftd_drv_burn_uart_single_deinit(uint8_t channel)
{
    switch(channel)
    {
        case 0:
        UART_Close(UART1);
        break;
        case 1:
        UART_Close(UART2);
        break;
        case 2:
        UART_Close(UART3);
        break;
        case 3:
        UART_Close(UART4);
        break;
    }
}

/**
 * @brief Clear UART RX FIFO with proper timeout
 * @param uart Pointer to UART peripheral
 */
static void ftd_drv_burn_uart_clear_rx_fifo(UART_T* uart)
{
    uint32_t timeout = 1000;
    while ((!(uart->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk)) && (timeout > 0)) {
        volatile uint32_t dummy = uart->DAT;
        (void)dummy;
        timeout--;
    }
    // Reset RX FIFO
    uart->FIFO |= UART_FIFO_RXRST_Msk;
    timeout = 1000;
    while ((uart->FIFO & UART_FIFO_RXRST_Msk) && (timeout > 0)) {
        timeout--;
    }
}

static void ftd_drv_burn_channel1_rx_start(uint8_t* buf, uint16_t len)
{
    // Disable RX PDMA first
    UART1->INTEN &= ~(UART_INTEN_RXPDMAEN_Msk);

    // Stop and clear PDMA channel
    PDMA_STOP(PDMA, UART1_RX_PDMA_CH);
    PDMA_CLR_TD_FLAG(PDMA, 1 << UART1_RX_PDMA_CH);

    // Clear RX FIFO
    ftd_drv_burn_uart_clear_rx_fifo(UART1);

    /* Configure PDMA Channel */
    PDMA_Open(PDMA, 1 << UART1_RX_PDMA_CH);
    PDMA_SetTransferMode(PDMA, UART1_RX_PDMA_CH, PDMA_UART1_RX, 0, 0);
    PDMA_SetTransferCnt(PDMA, UART1_RX_PDMA_CH, PDMA_WIDTH_8, len);
    PDMA_SetTransferAddr(PDMA, UART1_RX_PDMA_CH, UART1_BASE, PDMA_SAR_FIX, ((uint32_t)(&buf[0])), PDMA_DAR_INC);
    PDMA_SetBurstType(PDMA, UART1_RX_PDMA_CH, PDMA_REQ_SINGLE, 0);

    // Enable RX PDMA
    UART1->INTEN |= UART_INTEN_RXPDMAEN_Msk;
}

static void ftd_drv_burn_channel1_rx_stop(void)
{
    UART1->INTEN &= ~(UART_INTEN_RXPDMAEN_Msk);
    PDMA_STOP(PDMA, UART1_RX_PDMA_CH);
}

static void ftd_drv_burn_channel1_tx_start(uint8_t* buf, uint16_t len)
{
    // Disable TX PDMA first
    UART1->INTEN &= ~(UART_INTEN_TXPDMAEN_Msk);

    // Stop and clear PDMA channel
    PDMA_STOP(PDMA, UART1_TX_PDMA_CH);
    PDMA_CLR_TD_FLAG(PDMA, 1 << UART1_TX_PDMA_CH);

    /* Configure PDMA Channel */
    PDMA_Open(PDMA, 1 << UART1_TX_PDMA_CH);
    PDMA_SetTransferMode(PDMA, UART1_TX_PDMA_CH, PDMA_UART1_TX, 0, 0);
    PDMA_SetTransferCnt(PDMA, UART1_TX_PDMA_CH, PDMA_WIDTH_8, len);
    PDMA_SetTransferAddr(PDMA, UART1_TX_PDMA_CH, ((uint32_t)(&buf[0])), PDMA_SAR_INC, UART1_BASE, PDMA_DAR_FIX);
    PDMA_SetBurstType(PDMA, UART1_TX_PDMA_CH, PDMA_REQ_SINGLE, 0);

    // Enable TX PDMA
    UART1->INTEN |= UART_INTEN_TXPDMAEN_Msk;
}

static void ftd_drv_burn_channel2_rx_start(uint8_t* buf, uint16_t len)
{
    // Disable RX PDMA first
    UART2->INTEN &= ~(UART_INTEN_RXPDMAEN_Msk);

    // Stop and clear PDMA channel
    PDMA_STOP(PDMA, UART2_RX_PDMA_CH);
    PDMA_CLR_TD_FLAG(PDMA, 1 << UART2_RX_PDMA_CH);

    // Clear RX FIFO
    ftd_drv_burn_uart_clear_rx_fifo(UART2);

    /* Configure PDMA Channel */
    PDMA_Open(PDMA, 1 << UART2_RX_PDMA_CH);
    PDMA_SetTransferMode(PDMA, UART2_RX_PDMA_CH, PDMA_UART2_RX, 0, 0);
    PDMA_SetTransferCnt(PDMA, UART2_RX_PDMA_CH, PDMA_WIDTH_8, len);
    PDMA_SetTransferAddr(PDMA, UART2_RX_PDMA_CH, UART2_BASE, PDMA_SAR_FIX, ((uint32_t)(&buf[0])), PDMA_DAR_INC);
    PDMA_SetBurstType(PDMA, UART2_RX_PDMA_CH, PDMA_REQ_SINGLE, 0);

    // Enable RX PDMA
    UART2->INTEN |= UART_INTEN_RXPDMAEN_Msk;
}

static void ftd_drv_burn_channel2_rx_stop(void)
{
    UART2->INTEN &= ~(UART_INTEN_RXPDMAEN_Msk);
    PDMA_STOP(PDMA, UART2_RX_PDMA_CH);
}

static void ftd_drv_burn_channel2_tx_start(uint8_t* buf, uint16_t len)
{
    // Disable TX PDMA first
    UART2->INTEN &= ~(UART_INTEN_TXPDMAEN_Msk);

    // Stop and clear PDMA channel
    PDMA_STOP(PDMA, UART2_TX_PDMA_CH);
    PDMA_CLR_TD_FLAG(PDMA, 1 << UART2_TX_PDMA_CH);

    /* Configure PDMA Channel */
    PDMA_Open(PDMA, 1 << UART2_TX_PDMA_CH);
    PDMA_SetTransferMode(PDMA, UART2_TX_PDMA_CH, PDMA_UART2_TX, 0, 0);
    PDMA_SetTransferCnt(PDMA, UART2_TX_PDMA_CH, PDMA_WIDTH_8, len);
    PDMA_SetTransferAddr(PDMA, UART2_TX_PDMA_CH, ((uint32_t)(&buf[0])), PDMA_SAR_INC, UART2_BASE, PDMA_DAR_FIX);
    PDMA_SetBurstType(PDMA, UART2_TX_PDMA_CH, PDMA_REQ_SINGLE, 0);

    // Enable TX PDMA
    UART2->INTEN |= UART_INTEN_TXPDMAEN_Msk;
}

static void ftd_drv_burn_channel3_rx_start(uint8_t* buf, uint16_t len)
{
    // Disable RX PDMA first
    UART3->INTEN &= ~(UART_INTEN_RXPDMAEN_Msk);

    // Stop and clear PDMA channel
    PDMA_STOP(PDMA, UART3_RX_PDMA_CH);
    PDMA_CLR_TD_FLAG(PDMA, 1 << UART3_RX_PDMA_CH);

    // Clear RX FIFO
    ftd_drv_burn_uart_clear_rx_fifo(UART3);

    /* Configure PDMA Channel */
    PDMA_Open(PDMA, 1 << UART3_RX_PDMA_CH);
    PDMA_SetTransferMode(PDMA, UART3_RX_PDMA_CH, PDMA_UART3_RX, 0, 0);
    PDMA_SetTransferCnt(PDMA, UART3_RX_PDMA_CH, PDMA_WIDTH_8, len);
    PDMA_SetTransferAddr(PDMA, UART3_RX_PDMA_CH, UART3_BASE, PDMA_SAR_FIX, ((uint32_t)(&buf[0])), PDMA_DAR_INC);
    PDMA_SetBurstType(PDMA, UART3_RX_PDMA_CH, PDMA_REQ_SINGLE, 0);

    // Enable RX PDMA
    UART3->INTEN |= UART_INTEN_RXPDMAEN_Msk;
}

static void ftd_drv_burn_channel3_rx_stop(void)
{
    UART3->INTEN &= ~(UART_INTEN_RXPDMAEN_Msk);
    PDMA_STOP(PDMA, UART3_RX_PDMA_CH);
}

static void ftd_drv_burn_channel3_tx_start(uint8_t* buf, uint16_t len)
{
    // Disable TX PDMA first
    UART3->INTEN &= ~(UART_INTEN_TXPDMAEN_Msk);

    // Stop and clear PDMA channel
    PDMA_STOP(PDMA, UART3_TX_PDMA_CH);
    PDMA_CLR_TD_FLAG(PDMA, 1 << UART3_TX_PDMA_CH);

    /* Configure PDMA Channel */
    PDMA_Open(PDMA, 1 << UART3_TX_PDMA_CH);
    PDMA_SetTransferMode(PDMA, UART3_TX_PDMA_CH, PDMA_UART3_TX, 0, 0);
    PDMA_SetTransferCnt(PDMA, UART3_TX_PDMA_CH, PDMA_WIDTH_8, len);
    PDMA_SetTransferAddr(PDMA, UART3_TX_PDMA_CH, ((uint32_t)(&buf[0])), PDMA_SAR_INC, UART3_BASE, PDMA_DAR_FIX);
    PDMA_SetBurstType(PDMA, UART3_TX_PDMA_CH, PDMA_REQ_SINGLE, 0);

    // Enable TX PDMA
    UART3->INTEN |= UART_INTEN_TXPDMAEN_Msk;
}

static void ftd_drv_burn_channel4_rx_start(uint8_t* buf, uint16_t len)
{
    // Disable RX PDMA first
    UART4->INTEN &= ~(UART_INTEN_RXPDMAEN_Msk);

    // Stop and clear PDMA channel
    PDMA_STOP(PDMA, UART4_RX_PDMA_CH);
    PDMA_CLR_TD_FLAG(PDMA, 1 << UART4_RX_PDMA_CH);

    // Clear RX FIFO
    ftd_drv_burn_uart_clear_rx_fifo(UART4);

    /* Configure PDMA Channel */
    PDMA_Open(PDMA, 1 << UART4_RX_PDMA_CH);
    PDMA_SetTransferMode(PDMA, UART4_RX_PDMA_CH, PDMA_UART4_RX, 0, 0);
    PDMA_SetTransferCnt(PDMA, UART4_RX_PDMA_CH, PDMA_WIDTH_8, len);
    PDMA_SetTransferAddr(PDMA, UART4_RX_PDMA_CH, UART4_BASE, PDMA_SAR_FIX, ((uint32_t)(&buf[0])), PDMA_DAR_INC);
    PDMA_SetBurstType(PDMA, UART4_RX_PDMA_CH, PDMA_REQ_SINGLE, 0);

    // Enable RX PDMA
    UART4->INTEN |= UART_INTEN_RXPDMAEN_Msk;
}

static void ftd_drv_burn_channel4_rx_stop(void)
{
    UART4->INTEN &= ~(UART_INTEN_RXPDMAEN_Msk);
    PDMA_STOP(PDMA, UART4_RX_PDMA_CH);
}

static void ftd_drv_burn_channel4_tx_start(uint8_t* buf, uint16_t len)
{
    // Disable TX PDMA first
    UART4->INTEN &= ~(UART_INTEN_TXPDMAEN_Msk);

    // Stop and clear PDMA channel
    PDMA_STOP(PDMA, UART4_TX_PDMA_CH);
    PDMA_CLR_TD_FLAG(PDMA, 1 << UART4_TX_PDMA_CH);

    /* Configure PDMA Channel */
    PDMA_Open(PDMA, 1 << UART4_TX_PDMA_CH);
    PDMA_SetTransferMode(PDMA, UART4_TX_PDMA_CH, PDMA_UART4_TX, 0, 0);
    PDMA_SetTransferCnt(PDMA, UART4_TX_PDMA_CH, PDMA_WIDTH_8, len);
    PDMA_SetTransferAddr(PDMA, UART4_TX_PDMA_CH, ((uint32_t)(&buf[0])), PDMA_SAR_INC, UART4_BASE, PDMA_DAR_FIX);
    PDMA_SetBurstType(PDMA, UART4_TX_PDMA_CH, PDMA_REQ_SINGLE, 0);

    // Enable TX PDMA
    UART4->INTEN |= UART_INTEN_TXPDMAEN_Msk;
}

void ftd_drv_burn_channel_start(uint8_t* tx_buf, uint16_t tx_len, uint8_t* rx_buf, uint16_t rx_len)
{
    // Disable TX/RX PDMA first
    UART4->INTEN &= ~(UART_INTEN_TXPDMAEN_Msk | UART_INTEN_RXPDMAEN_Msk);

    // Stop and clear PDMA channels
    PDMA_STOP(PDMA, UART4_TX_PDMA_CH);
    PDMA_STOP(PDMA, UART4_RX_PDMA_CH);
    PDMA_CLR_TD_FLAG(PDMA, 1 << UART4_TX_PDMA_CH);
    PDMA_CLR_TD_FLAG(PDMA, 1 << UART4_RX_PDMA_CH);

    // Clear RX FIFO
    ftd_drv_burn_uart_clear_rx_fifo(UART4);

    /* Configure TX PDMA Channel */
    PDMA_Open(PDMA, 1 << UART4_TX_PDMA_CH);
    PDMA_SetTransferMode(PDMA, UART4_TX_PDMA_CH, PDMA_UART4_TX, 0, 0);
    PDMA_SetTransferCnt(PDMA, UART4_TX_PDMA_CH, PDMA_WIDTH_8, tx_len);
    PDMA_SetTransferAddr(PDMA, UART4_TX_PDMA_CH, ((uint32_t)(&tx_buf[0])), PDMA_SAR_INC, UART4_BASE, PDMA_DAR_FIX);
    PDMA_SetBurstType(PDMA, UART4_TX_PDMA_CH, PDMA_REQ_SINGLE, 0);

    /* Configure RX PDMA Channel */
    PDMA_Open(PDMA, 1 << UART4_RX_PDMA_CH);
    PDMA_SetTransferMode(PDMA, UART4_RX_PDMA_CH, PDMA_UART4_RX, 0, 0);
    PDMA_SetTransferCnt(PDMA, UART4_RX_PDMA_CH, PDMA_WIDTH_8, rx_len);
    PDMA_SetTransferAddr(PDMA, UART4_RX_PDMA_CH, UART4_BASE, PDMA_SAR_FIX, ((uint32_t)(&rx_buf[0])), PDMA_DAR_INC);
    PDMA_SetBurstType(PDMA, UART4_RX_PDMA_CH, PDMA_REQ_SINGLE, 0);

    // Enable TX/RX PDMA
    UART4->INTEN |= (UART_INTEN_TXPDMAEN_Msk | UART_INTEN_RXPDMAEN_Msk);
}


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
void ftd_drv_burn_uart_tx_start(uint8_t channel, uint8_t* buf, uint16_t len)
{
    if (channel == 0)
        ftd_drv_burn_channel1_tx_start(buf, len);
    else if (channel == 1)
        ftd_drv_burn_channel2_tx_start(buf, len);
    else if (channel == 2)
        ftd_drv_burn_channel3_tx_start(buf, len);
    else if (channel == 3)
        ftd_drv_burn_channel4_tx_start(buf, len);
    else;
}


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
void ftd_drv_burn_uart_rx_start(uint8_t channel, uint8_t* buf, uint16_t len)
{
    if (channel == 0)
        ftd_drv_burn_channel1_rx_start(buf, len);
    else if (channel == 1)
        ftd_drv_burn_channel2_rx_start(buf, len);
    else if (channel == 2)
        ftd_drv_burn_channel3_rx_start(buf, len);
    else if (channel == 3)
        ftd_drv_burn_channel4_rx_start(buf, len);
    else;
}

/**
 * Stop UART reception on the specified channel
 * @param channel UART channel number (0-3 corresponding to UART1-UART4)
 * @param buf Pointer to the data buffer where received data will be stored
 * @param len Length of data to be received in bytes
 *
 * @note The function to stop PDMA for efficient data transfer without CPU intervention.
 */
void ftd_drv_burn_uart_rx_stop(uint8_t channel)
{
    if (channel == 0)
        ftd_drv_burn_channel1_rx_stop();
    else if (channel == 1)
        ftd_drv_burn_channel2_rx_stop();
    else if (channel == 2)
        ftd_drv_burn_channel3_rx_stop();
    else if (channel == 3)
        ftd_drv_burn_channel4_rx_stop();
    else;
}

/**
 * Check if the transmission on the specified UART channel is complete
 *
 * @param channel UART channel number (0-3 corresponding to UART1-UART4)
 * @return true: transmission complete, false: transmission not complete or invalid channel
 */
bool ftd_drv_burn_channel_is_tx_complete(uint8_t channel)
{
    // PDMA channel number and transmission complete status
    uint8_t pdma_ch = 0;
    bool tx_complete = false;

    // Check if the channel number is valid
    if (channel >= BURN_UART_CHANNELS)
    {
        return false;
    }

    // Map UART channel number to corresponding PDMA channel
    if (channel == 0)
        pdma_ch = UART1_TX_PDMA_CH;
    else if (channel == 1)
        pdma_ch = UART2_TX_PDMA_CH;
    else if (channel == 2)
        pdma_ch = UART3_TX_PDMA_CH;
    else if (channel == 3)
        pdma_ch = UART4_TX_PDMA_CH;
    else;

    // Check PDMA transfer done status register to determine if transmission is complete
    tx_complete = (PDMA_GET_TD_STS(PDMA) & (1 << pdma_ch)) ? true : false;

    return tx_complete;
}

/**
 * Check if the reception on the specified UART channel is complete
 *
 * @param channel UART channel number (0-3 corresponding to UART1-UART4)
 * @return true: reception complete, false: reception not complete or invalid channel
 */
bool ftd_drv_burn_channel_is_rx_complete(uint8_t channel)
{
    // PDMA channel number and reception complete status
    uint8_t pdma_ch = 0;
    bool rx_complete = false;

    // Check if the channel number is valid
    if (channel >= BURN_UART_CHANNELS)
    {
        return false;
    }

    // Map UART channel number to corresponding PDMA channel
    if (channel == 0)
        pdma_ch = UART1_RX_PDMA_CH;
    else if (channel == 1)
        pdma_ch = UART2_RX_PDMA_CH;
    else if (channel == 2)
        pdma_ch = UART3_RX_PDMA_CH;
    else if (channel == 3)
        pdma_ch = UART4_RX_PDMA_CH;
    else;

    // Check PDMA transfer done status register to determine if reception is complete
    rx_complete = (PDMA_GET_TD_STS(PDMA) & (1 << pdma_ch)) ? true : false;

    return rx_complete;
}

#if 1//FTD_DRV_BURN_UART_TEST
void burn_uart_test(void)
{
    uint16_t i;
    uint8_t buffer_tx[512];

    // ftd_mw_burn_uart_init();

    for (i = 0;i < 512;i++)
    {
        buffer_tx[i] = i % 256;
    }

    while (1)
    {
        ftd_drv_burn_channel2_rx_start(buffer_tx, 8);
        while (ftd_drv_burn_channel_is_rx_complete(1) == false);
        FTD_LOG_INFO(" [0x%x][0x%x][0x%x][0x%x]\n", buffer_tx[0], buffer_tx[1], buffer_tx[6], buffer_tx[7]);

        ftd_drv_burn_channel1_tx_start(buffer_tx, 512);
        ftd_drv_burn_channel2_tx_start(buffer_tx, 512);
        ftd_drv_burn_channel3_tx_start(buffer_tx, 512);
        ftd_drv_burn_channel4_tx_start(buffer_tx, 512);

        while ((ftd_drv_burn_channel_is_tx_complete(1) == false) || (ftd_drv_burn_channel_is_tx_complete(1) == false)
            || (ftd_drv_burn_channel_is_tx_complete(2) == false) || (ftd_drv_burn_channel_is_tx_complete(3) == false))
        {
            ;
        }
        FTD_LOG_INFO(" tx_complete");
    }
}
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* UART IRQ Handler                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
void UART1_IRQHandler(void)
{
    uint32_t u32IntSts = UART1->INTSTS;

    if (u32IntSts & UART_INTSTS_HWRLSIF_Msk)
    {
        if (UART1->FIFOSTS & UART_FIFOSTS_BIF_Msk)
            FTD_LOG_DEBUG("\n UART1 BIF \n");
        if (UART1->FIFOSTS & UART_FIFOSTS_FEF_Msk)
            FTD_LOG_DEBUG("\n UART1 FEF \n");
        if (UART1->FIFOSTS & UART_FIFOSTS_PEF_Msk)
            FTD_LOG_DEBUG("\n UART1 PEF \n");

        UART1->FIFOSTS = (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk);
    }
}

void UART2_IRQHandler(void)
{
    uint32_t u32IntSts = UART2->INTSTS;

    if (u32IntSts & UART_INTSTS_HWRLSIF_Msk)
    {
        if (UART2->FIFOSTS & UART_FIFOSTS_BIF_Msk)
            FTD_LOG_DEBUG("\n UART2 BIF \n");
        if (UART2->FIFOSTS & UART_FIFOSTS_FEF_Msk)
            FTD_LOG_DEBUG("\n UART2 FEF \n");
        if (UART2->FIFOSTS & UART_FIFOSTS_PEF_Msk)
            FTD_LOG_DEBUG("\n UART2 PEF \n");

        UART2->FIFOSTS = (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk);
    }
}

void UART3_IRQHandler(void)
{
    uint32_t u32IntSts = UART3->INTSTS;

    if (u32IntSts & UART_INTSTS_HWRLSIF_Msk)
    {
        if (UART3->FIFOSTS & UART_FIFOSTS_BIF_Msk)
            FTD_LOG_DEBUG("\n UART3 BIF \n");
        if (UART3->FIFOSTS & UART_FIFOSTS_FEF_Msk)
            FTD_LOG_DEBUG("\n UART3 FEF \n");
        if (UART3->FIFOSTS & UART_FIFOSTS_PEF_Msk)
            FTD_LOG_DEBUG("\n UART3 PEF \n");

        UART3->FIFOSTS = (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk);
    }
}

#if (0 == DEBUG_USE_UART4)
void UART4_IRQHandler(void)
{
    uint32_t u32IntSts = UART4->INTSTS;

    if (u32IntSts & UART_INTSTS_HWRLSIF_Msk)
    {
        if (UART4->FIFOSTS & UART_FIFOSTS_BIF_Msk)
            FTD_LOG_DEBUG("\n UART4 BIF \n");
        if (UART4->FIFOSTS & UART_FIFOSTS_FEF_Msk)
            FTD_LOG_DEBUG("\n UART4 FEF \n");
        if (UART4->FIFOSTS & UART_FIFOSTS_PEF_Msk)
            FTD_LOG_DEBUG("\n UART4 PEF \n");

        UART4->FIFOSTS = (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk);
    }
}
#endif
