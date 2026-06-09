/****************************************************************************
@FILENAME:  main.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.14
@COPYRIGHT: FTD co.ltd
****************************************************************************/
#include "ftd_app_prog.h"
#include "ftd_app_state_machine.h"
#include "ftd_mw_log_manager.h"
#include "writer_pinconfigure.h"
#include "stdio.h"
#include "NuMicro.h"

#include "ftd_mw_iap_manager.h"
#include "ftd_mw_hc_manager.h"
#include "ftd_mw_misc_manager.h"

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
    CLK_EnableModuleClock(UART4_MODULE);
    /* Configure UART1 and set UART1 baud rate */
    UART_Open(UART4, 115200); // !!!!! DEBUG_PORT  in retarget.c
    CLK_SetModuleClock(UART4_MODULE, CLK_CLKSEL3_UART4SEL_HXT, CLK_CLKDIV4_UART4(1));
    SYS_LockReg();

    //    SYS_UnlockReg();
    //    CLK_EnableModuleClock(UART0_MODULE);
    //    /* Configure UART1 and set UART1 baud rate */
    //    UART_Open(UART0, 115200); // !!!!! DEBUG_PORT  in retarget.c
    //    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));
    //    SYS_LockReg();
}




void GPIO_SetMode(GPIO_T* port, uint32_t u32PinMask, uint32_t u32Mode)
{
    uint32_t i;

    for (i = 0ul; i < GPIO_PIN_MAX; i++)
    {
        if ((u32PinMask & (1ul << i)) == (1ul << i))
        {
            port->MODE = (port->MODE & ~(0x3ul << (i << 1))) | (u32Mode << (i << 1));
        }
    }
}

int32_t main(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Init System, peripheral clock and multi-function I/O */
    SYS_Init();
    FTDWriter_PinConfigure_init();
    GPIO_SetMode(PH, BIT6, GPIO_MODE_OUTPUT);
    PH6 = 0;
    __enable_irq();
    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock and CyclesPerUs automatically. */
    SystemCoreClockUpdate();

    /* Lock protected registers */
    SYS_LockReg();

    /* Init UART0 for printf */
    debug_init();

    ftd_mw_misc_manager_init();

    /* state machine init */
    app_ftd_state_machine_change_state(APP_PROGRAMMING);
    JYMC_LOG_INFO("IAP main running\r\n");


    ftd_mw_iap_manager_init();


    IAP_FLAG_T flag_load;
    ftd_mw_iap_manager_flag_load(&flag_load);


    JYMC_LOG_INFO("flag_load.start_magic:0x%08x\r\n", flag_load.start_magic);
    JYMC_LOG_INFO("flag_load.upgrade:0x%08x\r\n", flag_load.upgrade);

    ftd_mw_iap_manager_set_time_ms(ftd_mw_misc_manager_get_time_ms());

    if (flag_load.start_magic == IAP_MAGIC) //The magic number judgment is correct
    {
        if (flag_load.upgrade == 1) //Determine the upgrade flag, if it is in the upgrade state, jump to the upgrade program
        {
            JYMC_LOG_INFO("goto IAP_TASK\r\n");
            goto IAP_TASK;
        }
        else {
            if (flag_load.app_select == 2 && (ftd_mw_iap_manager_check_addr_valid(APP_2_START_BASE) == true))
            {
                JYMC_LOG_INFO("goto IAP_JUMP_APP2\r\n");
                goto IAP_JUMP_APP2;
            }
            else {
                JYMC_LOG_INFO("goto IAP_JUMP_APP1\r\n");
                goto IAP_JUMP_APP1;
            }
        }
    }
    else {
        JYMC_LOG_INFO("goto IAP_JUMP_APP1\r\n");
        IAP_FLAG_T flag;
        flag.start_magic = IAP_MAGIC;
        flag.upgrade = 0x00;
        flag.app_select = 1; // Set app_select to 1
        ftd_mw_iap_manager_flag_save(flag);
        goto IAP_JUMP_APP1;
    }

IAP_TASK:
    while (1)
    {
        app_ftd_state_machine_process();
        if (ftd_mw_misc_manager_get_time_ms() - ftd_mw_iap_manager_get_time_ms() >= IAP_MODE_TIME_OUT) //60s timeout
        {
            IAP_FLAG_T flag;
            flag.start_magic = IAP_MAGIC;
            flag.upgrade = 0x00;

            JYMC_LOG_INFO("IAP_TASK time out\r\n");
            if (flag_load.app_select == 2 && (ftd_mw_iap_manager_check_addr_valid(APP_2_START_BASE) == true))
            {
                if (ftd_mw_iap_manager_get_upgrade_flag())
                {
                    ftd_mw_iap_manager_erase_app_2();
                    JYMC_LOG_INFO("goto IAP_JUMP_APP1\r\n");
                    flag.app_select = 1; // Set app_select to 1
                    ftd_mw_iap_manager_flag_save(flag);
                    goto IAP_JUMP_APP1;
                }
                JYMC_LOG_INFO("goto IAP_JUMP_APP2\r\n");
                flag.app_select = 2; // Set app_select to 2
                ftd_mw_iap_manager_flag_save(flag);
                goto IAP_JUMP_APP2;
            }
            else {
                ftd_mw_iap_manager_erase_app_2();
                JYMC_LOG_INFO("goto IAP_JUMP_APP1\r\n");
                flag.app_select = 1; // Set app_select to 1
                ftd_mw_iap_manager_flag_save(flag);
                goto IAP_JUMP_APP1;
            }
        }
    }
IAP_JUMP_APP1:
    while (1) {
        if (ftd_mw_iap_manager_check_addr_valid(APP_1_START_BASE))
        {
            ftd_mw_iap_manager_jum_to_app(APP_1_START_BASE);
        }
        CLK_SysTickLongDelay(1000000);
    }

IAP_JUMP_APP2:
    while (1) {
        if (ftd_mw_iap_manager_check_addr_valid(APP_2_START_BASE))
        {
            ftd_mw_iap_manager_jum_to_app(APP_2_START_BASE);
        }
        CLK_SysTickLongDelay(1000000);
    }

}

