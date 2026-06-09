/****************************************************************************
@FILENAME:  ftd_drv_rf_uart.c
@FUNCTION:
@AUTHOR:    chenkailian
@DATE:      2026.1.24
@COPYRIGHT: FTD co.ltd
****************************************************************************/

#include "ftd_drv_rf_uart.h"
#include "ftd_mw_log_manager.h"
#include "NuMicro.h"
#include "stdbool.h"

// Static variables for tracking UART state
static bool g_b_uart5_initialized = false;
static bool g_b_is_rf_mode = false;

static void ftd_drv_rf_uart_pin_mux(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();
    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB5MFP_Msk | SYS_GPB_MFPL_PB4MFP_Msk);
    /* Set PE.6 to UART5 RX pin */
    SYS->GPE_MFPL &= ~SYS_GPE_MFPL_PE6MFP_Msk;
    SYS->GPE_MFPL |= SYS_GPE_MFPL_PE6MFP_UART5_RXD;

    /* Set PE.7 to UART5 TX pin */
    SYS->GPE_MFPL &= ~SYS_GPE_MFPL_PE7MFP_Msk;
    SYS->GPE_MFPL |= SYS_GPE_MFPL_PE7MFP_UART5_TXD;

    GPIO_SetMode(PE, BIT6, GPIO_MODE_INPUT);
    GPIO_SetMode(PE, BIT7, GPIO_MODE_OUTPUT);


    GPIO_SetPullCtl(PE, BIT6, GPIO_PUSEL_PULL_UP);

    /* Lock protected registers */
    SYS_LockReg();

    // Add small delay to ensure pin mux is stable
    CLK_SysTickLongDelay(10000);

    g_b_is_rf_mode = true;
}

static void ftd_drv_rf_uart_pin_mux_back_to_panel(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Set PE.6 and PE.7 back to GPIO or other functions for display */
    /* Note: This assumes display uses different pin functions */
    /* You may need to adjust this based on actual display requirements */
    SYS->GPE_MFPL &= ~(SYS_GPE_MFPL_PE6MFP_Msk | SYS_GPE_MFPL_PE7MFP_Msk);

    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB5MFP_Msk | SYS_GPB_MFPL_PB4MFP_Msk);
    SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB5MFP_UART5_TXD | SYS_GPB_MFPL_PB4MFP_UART5_RXD);
    /* Lock protected registers */
    SYS_LockReg();

    // Add small delay to ensure pin mux is stable
    CLK_SysTickLongDelay(10000);

    g_b_is_rf_mode = false;
}

// Initialize UART5 and PDMA once at system startup
void ftd_drv_rf_uart_init(void)
{
    if (!g_b_uart5_initialized)
    {
        /* Unlock protected registers */
        SYS_UnlockReg();
        
        // Enable PDMA clock
        CLK->AHBCLK |= CLK_AHBCLK_PDMACKEN_Msk;

        // Initialize UART5
        CLK_EnableModuleClock(UART5_MODULE);
        CLK_SetModuleClock(UART5_MODULE, CLK_CLKSEL3_UART5SEL_HXT, CLK_CLKDIV4_UART5(1));

        /* Update System Core Clock */
        SystemCoreClockUpdate();

        /* Lock protected registers */
        SYS_LockReg();

        // Open UART5 with 115200 baud rate
        UART_Open(UART5, 115200);

        // Enable UART5 interrupt
        NVIC_EnableIRQ(UART5_IRQn);

        // g_b_uart5_initialized = true;

        // FTD_LOG_INFO("RF UART initialized successfully");
    }
}

void ftd_drv_rf_uart_rx_start(uint8_t* buf, uint16_t len)
{

    if (!g_b_uart5_initialized)
    {
        ftd_drv_rf_uart_init();
    }

    /* Unlock protected registers */
    SYS_UnlockReg();

    // Set RF UART pin mux if not already in RF mode
    if (!g_b_is_rf_mode)
    {
        ftd_drv_rf_uart_pin_mux();
    }

    // Stop any ongoing RX and TX operations to prevent cross-talk
    PDMA_STOP(PDMA, UART5_RX_PDMA_CH);
    PDMA_STOP(PDMA, UART5_TX_PDMA_CH);
    PDMA_CLR_TD_FLAG(PDMA, 1 << UART5_RX_PDMA_CH);
    PDMA_CLR_TD_FLAG(PDMA, 1 << UART5_TX_PDMA_CH);
    UART5->INTEN &= ~(UART_INTEN_RXPDMAEN_Msk | UART_INTEN_TXPDMAEN_Msk);

    // Clear RX FIFO completely
    // First, read out any existing data
    uint32_t timeout = 10000;
    while ((!(UART5->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk)) && (timeout > 0))
    {
        volatile uint32_t dummy = UART5->DAT;
        (void)dummy;
        timeout--;
    }

    // Reset RX FIFO to ensure it's completely empty
    UART5->FIFO |= UART_FIFO_RXRST_Msk;
    // Wait for reset to complete
    while (UART5->FIFO & UART_FIFO_RXRST_Msk);

    // Add a short delay to ensure FIFO is stable
    // CLK_SysTickLongDelay(1000);

    /* Lock protected registers */
    SYS_LockReg();

    /* Configure PDMA Channel for RX */
    PDMA_Open(PDMA, 1 << UART5_RX_PDMA_CH); // Channel for UART5 RX
    // Select basic mode
    PDMA_SetTransferMode(PDMA, UART5_RX_PDMA_CH, PDMA_UART5_RX, 0, 0);
    // Set data width and transfer count
    PDMA_SetTransferCnt(PDMA, UART5_RX_PDMA_CH, PDMA_WIDTH_8, len);
    //Set PDMA Transfer Address
    PDMA_SetTransferAddr(PDMA, UART5_RX_PDMA_CH, UART5_BASE, PDMA_SAR_FIX, ((uint32_t)(&buf[0])), PDMA_DAR_INC);
    //Select Single Request
    PDMA_SetBurstType(PDMA, UART5_RX_PDMA_CH, PDMA_REQ_SINGLE, 0);

    // Enable RX PDMA
    UART5->INTEN |= UART_INTEN_RXPDMAEN_Msk;
}

static void ftd_drv_rf_uart_rx_stop(void)
{
    PDMA_STOP(PDMA, UART5_RX_PDMA_CH);
    PDMA_CLR_TD_FLAG(PDMA, 1 << UART5_RX_PDMA_CH);
    UART5->INTEN &= ~(UART_INTEN_RXPDMAEN_Msk);
}

void ftd_drv_rf_uart_stop(void)
{
    /* Stop RX and TX PDMA channels */
    PDMA_STOP(PDMA, UART5_RX_PDMA_CH);
    PDMA_STOP(PDMA, UART5_TX_PDMA_CH);

    /* Clear flags */
    PDMA_CLR_TD_FLAG(PDMA, 1 << UART5_RX_PDMA_CH);
    PDMA_CLR_TD_FLAG(PDMA, 1 << UART5_TX_PDMA_CH);

    /* Disable UART5 interrupts */
    UART5->INTEN &= ~(UART_INTEN_RXPDMAEN_Msk | UART_INTEN_TXPDMAEN_Msk);

    /* Switch pin mux back to panel mode if in RF mode */
    if (g_b_is_rf_mode)
    {
        ftd_drv_rf_uart_pin_mux_back_to_panel();
    }
}

void ftd_drv_rf_uart_tx_start(uint8_t* buf, uint16_t len)
{
    if (!g_b_uart5_initialized)
    {
        ftd_drv_rf_uart_init();
    }

    /* Unlock protected registers */
    SYS_UnlockReg();

    // Set RF UART pin mux if not already in RF mode
    if (!g_b_is_rf_mode)
    {
        ftd_drv_rf_uart_pin_mux();
    }

    // Stop any ongoing TX operation
    PDMA_STOP(PDMA, UART5_TX_PDMA_CH);
    PDMA_CLR_TD_FLAG(PDMA, 1 << UART5_TX_PDMA_CH);
    UART5->INTEN &= ~(UART_INTEN_TXPDMAEN_Msk);

    /* Lock protected registers */
    SYS_LockReg();

    /* Configure PDMA Channel for TX */
    PDMA_Open(PDMA, 1 << UART5_TX_PDMA_CH); // Channel for UART5 TX
    // Select basic mode
    PDMA_SetTransferMode(PDMA, UART5_TX_PDMA_CH, PDMA_UART5_TX, 0, 0);
    // Set data width and transfer count
    PDMA_SetTransferCnt(PDMA, UART5_TX_PDMA_CH, PDMA_WIDTH_8, len);
    //Set PDMA Transfer Address
    PDMA_SetTransferAddr(PDMA, UART5_TX_PDMA_CH, ((uint32_t)(&buf[0])), PDMA_SAR_INC, UART5_BASE, PDMA_DAR_FIX);
    //Select Single Request
    PDMA_SetBurstType(PDMA, UART5_TX_PDMA_CH, PDMA_REQ_SINGLE, 0);

    // Enable TX PDMA
    UART5->INTEN |= UART_INTEN_TXPDMAEN_Msk;
}

bool ftd_drv_rf_uart_is_tx_complete(void)
{
    bool tx_complete = false;

    // Check PDMA transfer done status register to determine if transmission is complete
    tx_complete = (PDMA_GET_TD_STS(PDMA) & (1 << UART5_TX_PDMA_CH)) ? true : false;

    if (tx_complete)
    {
        // Clear PDMA transfer done flag to avoid false detection in next operation
        PDMA_CLR_TD_FLAG(PDMA, 1 << UART5_TX_PDMA_CH);
    }

    return tx_complete;
}

bool ftd_drv_rf_uart_is_tx_fifo_empty(void)
{
    bool fifo_empty = false;
    fifo_empty = (UART5->FIFOSTS & UART_FIFOSTS_TXEMPTY_Msk) ? true : false;

    return fifo_empty;
}

bool ftd_drv_rf_uart_is_rx_complete(void)
{
    bool rx_complete = false;
    // Check PDMA transfer done status register to determine if reception is complete
    rx_complete = (PDMA_GET_TD_STS(PDMA) & (1 << UART5_RX_PDMA_CH)) ? true : false;

    if (rx_complete)
    {
        // Clear PDMA transfer done flag to avoid false detection in next operation
        PDMA_CLR_TD_FLAG(PDMA, 1 << UART5_RX_PDMA_CH);
    }

    return rx_complete;
}

bool ftd_drv_rf_uart_is_rx_fifo_empty(void)
{
    bool fifo_empty = false;
    fifo_empty = (UART5->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk) ? true : false;

    return fifo_empty;
}

void ftd_drv_rf_uart_test(void)
{
    uint8_t tx_buff[64];
    uint8_t rx_buff[64];

    // Initialize RF UART first
    ftd_drv_rf_uart_init();

    for (size_t i = 0; i < 64; i++)
    {
        tx_buff[i] = i;
    }

    while (1)
    {
        JYMC_LOG_INFO(" tx start ");
        ftd_drv_rf_uart_tx_start(tx_buff, 64);
        while (ftd_drv_rf_uart_is_tx_complete() == false);

        // Wait for TX line to settle before switching to RX
        // This prevents signal reflection from being received as data
        CLK_SysTickLongDelay(20000);  // ~20ms delay

        JYMC_LOG_INFO(" rx start ");
        memset(rx_buff, 0x00, sizeof(rx_buff));
        ftd_drv_rf_uart_rx_start(rx_buff, 8);
        while (ftd_drv_rf_uart_is_rx_complete() == false);

        JYMC_LOG_INFO(" rf uart stop, switch back to panel mode ");
        ftd_drv_rf_uart_stop();

        // Simulate some delay before next RF UART usage
        for (volatile uint32_t i = 0; i < 1000000; i++);
    }
}

