/****************************************************************************
@FILENAME:  main.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.14
@COPYRIGHT: FTD co.ltd
****************************************************************************/
#include "drivers/ftd_drv_gpio.h"
#include "ftd_app_prog.h"
#include "ftd_app_burn.h"
#include "ftd_app_state_machine.h"
#include "ftd_mw_misc_manager.h"
#include "ftd_mw_display_manager.h"
#include "ftd_mw_sys_info_manager.h"
#include "ftd_mw_log_manager.h"
#include "writer_pinconfigure.h"
#include "ftd_data_model.h"
#include "stdio.h"
#include "NuMicro.h"
#include "ftd_mw_powermon_manager.h"

#include "ftd_drv_gpio.h"
#include "ftd_mw_burn_signal_manager.h"

#define PLL_CLOCK       192000000
void SYS_Init(void)
{

    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);

    /* Enable HXT clock (external XTAL 12MHz) */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);

    /* Wait for HXT clock ready */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(PLL_CLOCK);

    /* Set PCLK0/PCLK1 to HCLK/2 */
    CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2);

    /* Enable UART module clock */
    // CLK_EnableModuleClock(UART0_MODULE);
    // CLK_EnableModuleClock(SPI1_MODULE);

    /* Select UART module clock source as HXT and UART module clock divider as 1 */
    // CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));
}

void debug_init(void)
{
    SYS_UnlockReg();
#if DEBUG_USE_UART4
    CLK_EnableModuleClock(UART4_MODULE);
    /* Configure UART4 and set UART4 baud rate */
    UART_Open(UART4, 115200); // !!!!! DEBUG_PORT  in retarget.c
    CLK_SetModuleClock(UART4_MODULE, CLK_CLKSEL3_UART4SEL_HXT, CLK_CLKDIV4_UART4(1));
#else
    CLK_EnableModuleClock(UART0_MODULE);
    /* Configure UART1 and set UART1 baud rate */
    UART_Open(UART0, 115200); // !!!!! DEBUG_PORT  in retarget.c
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));
#endif
    SYS_LockReg();
}


void app_ui_event_handler(PANEL_TO_MCU_CMD event, uint8_t param)
{
    switch (event)
    {
        case PANEL_TO_MCU_CMD_APP_MODE: //0-host programmer  1-burn mcu 
            app_ftd_state_machine_change_state((APPLICATION)param);
            JYMC_LOG_INFO("Received PANEL_TO_MCU_CMD_APP_MODE with param: %d\n", param);
            break;
        case PANEL_TO_MCU_CMD_BURNNING_MODE:
            ftd_app_burn_set_burn_mode();
            JYMC_LOG_INFO("Received PANEL_TO_MCU_CMD_BURNNING_MODE with param: %d\n", param);
            break;
        case PANEL_TO_MCU_CMD_BURNNING_FW_SELECT:
            ftd_app_burn_fw_select(param);
            JYMC_LOG_INFO("Received PANEL_TO_MCU_CMD_BURNNING_FW_SELECT with param: %d\n", param);
            break;
        //case PANEL_TO_MCU_CMD_BURNNING_MANUAL_TRIGGER:
        //    JYMC_LOG_INFO("Received PANEL_TO_MCU_CMD_BURNNING_MANUAL_TRIGGER\n");
        //    break;
        default:
            break;
    }
}

int32_t main(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Init System, peripheral clock and multi-function I/O */
    SYS_Init();
    FTDWriter_PinConfigure_init();

#if DEBUG_USE_UART4
    ftd_drv_gpio_init();
    GPIO_SET_LOW(UART_EN_CH4);
#endif

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock and CyclesPerUs automatically. */
    SystemCoreClockUpdate();

    /* Lock protected registers */
    SYS_LockReg();

    /* Init UART0 for printf */
    debug_init();

    /*key(impl in misc) init */
    ftd_mw_misc_manager_init();

    /* sys info init */
    SYS_INFO st_sys_info;
    ftd_mw_sys_info_manager_init();
    ftd_mw_sys_info_manager_get(&st_sys_info);

    /* panel init */
    ftd_mw_display_manager_init(&st_sys_info);
    ftd_mw_display_manager_set_event_call_back(app_ui_event_handler);

    /* current detect init */
    ftd_mw_powermon_manager_init();

    /* state machine init */
    app_ftd_state_machine_change_state(APP_PROGRAMMING);

    /* burn signal init */
    ftd_mw_burn_signal_manager_init(); //burn signal init

    while (1)
    {
        if (ftd_mw_misc_manager_is_key_press())
        {
            JYMC_LOG_INFO(" key press in main \n ");
            ftd_mw_display_manager_key_press(); //send key press event to panel
        }

        /* auto change to program state when burn task done */
        if (BURN_DONE == ftd_app_burn_get_general_burn_state() && APP_BURN_ENGINE == app_ftd_state_machine_get_current_state())
        {
            ftd_mw_display_manager_set_button_state(BURN_STOP);
            JYMC_LOG_INFO(" quit BURN ENGINE \n ");
            app_ftd_state_machine_change_state(APP_PROGRAMMING);
        }

        ftd_mw_display_manager_rx_process();

        app_ftd_state_machine_process();

    }

}


#if (1 == DEBUG_USE_UART4)
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
