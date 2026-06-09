/****************************************************************************
@FILENAME:  ftd_drv_timer.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.23
@COPYRIGHT: FTD co.ltd
****************************************************************************/

#include "ftd_drv_timer.h"
#include "ftd_mw_log_manager.h"
#include "NuMicro.h"


volatile uint32_t g_ftd_system_time_ms = 0;

static void (*ftd_drv_timer_soft_timer_callback_funtion)(void) = NULL;

void TMR0_IRQHandler(void)
{
    // 清除中断标志
    TIMER_ClearIntFlag(TIMER0);

    g_ftd_system_time_ms++;

    // 调试输出（前几次）
    if (g_ftd_system_time_ms <= 15) {
        FTD_LOG_INFO("TMR0: %lu\n", g_ftd_system_time_ms);
    }
}

void TMR1_IRQHandler(void)
{
    // 清除中断标志
    TIMER_ClearIntFlag(TIMER1);

    if (ftd_drv_timer_soft_timer_callback_funtion != NULL)
        ftd_drv_timer_soft_timer_callback_funtion();
}

static void (*ftd_drv_timer_cyclic_callback_funtion)(void) = NULL;

void TMR2_IRQHandler(void)
{
    // 清除中断标志
    TIMER_ClearIntFlag(TIMER2);

    if (ftd_drv_timer_cyclic_callback_funtion != NULL)
        ftd_drv_timer_cyclic_callback_funtion();
}

void ftd_drv_timer_cyclic_init(void)
{
    // 启用TIMER2时钟
    CLK_EnableModuleClock(TMR2_MODULE);

    // 设置TIMER2时钟源
    CLK_SetModuleClock(TMR2_MODULE, CLK_CLKSEL1_TMR2SEL_HXT, 0);

    // 打开寄存器保护
    SYS_UnlockReg();

    // 配置TIMER2为周期模式，1ms中断一次
    TIMER_Open(TIMER2, TIMER_PERIODIC_MODE, 1000);

    // 启用中断
    TIMER_EnableInt(TIMER2);
    NVIC_EnableIRQ(TMR2_IRQn);

    // 启动定时器
    TIMER_Start(TIMER2);

    // 锁定寄存器保护
    SYS_LockReg();

    FTD_LOG_INFO("TIMER2 initialized for 1ms interrupts (cyclic tasks)\n");
}

void ftd_drv_timer_set_cyclic_call_back(void (*fp_callback)(void))
{
    ftd_drv_timer_cyclic_callback_funtion = fp_callback;
}

void ftd_drv_timer_init(void)
{
    // 启用TIMER0时钟
    CLK_EnableModuleClock(TMR0_MODULE);

    // 设置TIMER0时钟源
    CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_HXT, 0);

    // 打开寄存器保护
    SYS_UnlockReg();

    // 配置TIMER0为周期模式，1ms中断一次
    TIMER_Open(TIMER0, TIMER_PERIODIC_MODE, 1000);

    // 启用中断
    TIMER_EnableInt(TIMER0);
    NVIC_EnableIRQ(TMR0_IRQn);

    // 启动定时器
    TIMER_Start(TIMER0);

    // 锁定寄存器保护
    SYS_LockReg();

    FTD_LOG_INFO("TIMER0 initialized for 1ms interrupts\n");
}

uint32_t ftd_drv_timer_get_time_ms(void)
{
    return g_ftd_system_time_ms;
}


void ftd_drv_timer_soft_timer_init(void)
{
    // 启用TIMER1时钟
    CLK_EnableModuleClock(TMR1_MODULE);

    // 设置TIMER1时钟源
    CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HXT, 0);

    // 打开寄存器保护
    SYS_UnlockReg();

    // 配置TIMER1为周期模式，1ms中断一次
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);

    // 启用中断
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);

    // 启动定时器
    TIMER_Start(TIMER1);

    // 锁定寄存器保护
    SYS_LockReg();

    FTD_LOG_INFO("TIMER1 initialized for 1ms interrupts\n");

}


void ftd_drv_timer_set_soft_timer_call_back(void (*fp_callback)(void))
{
    ftd_drv_timer_soft_timer_callback_funtion = fp_callback;
}
