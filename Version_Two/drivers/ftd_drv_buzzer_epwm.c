#include "ftd_drv_buzzer_epwm.h"

void ftd_drv_buzzer_epwm_init(void)
{
    /* Enable EPWM0 module clock */
    CLK_EnableModuleClock(EPWM0_MODULE);

    /* Select BPWM module clock source */
    CLK_SetModuleClock(EPWM0_MODULE, CLK_CLKSEL2_EPWM0SEL_PLL, (uint32_t)NULL);
    /* Configure the frequency and duty cycle of the PWM output. */
    EPWM_ConfigOutputChannel(FTD_DRV_BUZZER_EPWM_PORT, FTD_DRV_BUZZER_EPWM_CH, FTD_DRV_BUZZER_EPWM_FREQ_HZ, FTD_DRV_BUZZER_EPWM_DUTY);

}

void ftd_drv_buzzer_epwm_start(void)
{
    // /* Configure the frequency and duty cycle of the PWM output. */
    // EPWM_ConfigOutputChannel(FTD_DRV_BUZZER_EPWM_PORT, FTD_DRV_BUZZER_EPWM_CH, FTD_DRV_BUZZER_EPWM_FREQ_HZ, FTD_DRV_BUZZER_EPWM_DUTY);

    /* Enable the PWM output. */
    EPWM_EnableOutput(FTD_DRV_BUZZER_EPWM_PORT, FTD_DRV_BUZZER_EPWM_CH_BIT);
    EPWM_Start(FTD_DRV_BUZZER_EPWM_PORT, FTD_DRV_BUZZER_EPWM_CH_BIT);
}

void ftd_drv_buzzer_epwm_stop(void)
{
    /* Disable the PWM output. */
    EPWM_DisableOutput(FTD_DRV_BUZZER_EPWM_PORT, FTD_DRV_BUZZER_EPWM_CH_BIT);
    EPWM_ForceStop(FTD_DRV_BUZZER_EPWM_PORT, FTD_DRV_BUZZER_EPWM_CH_BIT);
}
