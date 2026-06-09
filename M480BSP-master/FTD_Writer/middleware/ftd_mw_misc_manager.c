/****************************************************************************
@FILENAME:  ftd_mw_misc_manager.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.27
@COPYRIGHT: FTD co.ltd
****************************************************************************/
#include "ftd_mw_misc_manager.h"
#include "ftd_mw_log_manager.h"
#include "ftd_drv_timer.h"
#include "ftd_drv_gpio.h"
#include "ftd_drv_buzzer_epwm.h"
#include "NuMicro.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 


static bool g_key_press_flag = false;
static uint32_t g_key_press_time = 0;
static soft_timer_t g_timers[SOFT_TIMER_MAX];

static void ftd_mw_misc_manager_key_press_call_back(void)
{
    if (labs(ftd_drv_timer_get_time_ms() - g_key_press_time) > 200) //200ms
    {
        g_key_press_time = ftd_drv_timer_get_time_ms();
        g_key_press_flag = true;
        JYMC_LOG_INFO(" ftd_mw_misc_manager_key_press_call_back");
    }
    else
    {
        JYMC_LOG_INFO(" last_press_time[%ld] cur time[%ld]\n", g_key_press_time, ftd_drv_timer_get_time_ms());
    }

}

static void ftd_mw_misc_manager_soft_timer_call_back(void)
{
    for (int i = 0; i < SOFT_TIMER_MAX; i++)
    {
        if (g_timers[i].active)
        {
            g_timers[i].remain--;
            if (g_timers[i].remain == 0)
            {
                g_timers[i].cb(g_timers[i].arg);
                g_timers[i].active = 0;
            }
        }
    }
}

void ftd_mw_misc_manager_init(void)
{
    ftd_drv_timer_init();
    ftd_drv_gpio_set_key_event_call_back(ftd_mw_misc_manager_key_press_call_back);
    ftd_mw_misc_manager_soft_timer_init();
    ftd_drv_timer_set_soft_timer_call_back(ftd_mw_misc_manager_soft_timer_call_back);
    ftd_drv_buzzer_epwm_init();
}

uint32_t ftd_mw_misc_manager_get_time_ms(void)
{
    return ftd_drv_timer_get_time_ms();
}

bool ftd_mw_misc_manager_is_key_press(void)
{
    bool key_press = g_key_press_flag;
    g_key_press_flag = false;

    return key_press;
}

void ftd_mw_misc_manager_soft_timer_init(void)
{
    ftd_drv_timer_soft_timer_init();
    for (int i = 0; i < SOFT_TIMER_MAX; i++)
    {
        g_timers[i].active = 0;
    }
}

void ftd_mw_misc_manager_soft_timer_start(uint8_t id, uint32_t timeout, soft_timer_cb_t cb, void* arg)
{
    if (id >= SOFT_TIMER_MAX || cb == 0 || timeout == 0)
        return;

    g_timers[id].remain = timeout;
    g_timers[id].cb = cb;
    g_timers[id].arg = arg;
    g_timers[id].active = 1;
}

void ftd_mw_misc_manager_soft_timer_stop(uint8_t id)
{
    if (id >= SOFT_TIMER_MAX)
        return;

    g_timers[id].active = 0;
}




void ftd_mw_misc_manager_buzzer_open(uint32_t duration_ms)
{
    if (duration_ms > 0)
    {
        ftd_mw_misc_manager_soft_timer_start(FTD_MV_SOFT_TIMER_ID_BUZZER, duration_ms, (soft_timer_cb_t)ftd_mw_misc_manager_buzzer_close, NULL);
    }
    ftd_drv_buzzer_epwm_start();
}

void ftd_mw_misc_manager_buzzer_close(void)
{
    ftd_drv_buzzer_epwm_stop();
}

void ftd_mw_misc_manager_led1_on(uint32_t duration_ms)
{
    if (duration_ms > 0)
    {
        ftd_mw_misc_manager_soft_timer_start(FTD_MV_SOFT_TIMER_ID_LED1, duration_ms, (soft_timer_cb_t)ftd_mw_misc_manager_led1_off, NULL);
    }
    GPIO_SET_LOW(SYS_LED1);
}

void ftd_mw_misc_manager_led1_off(void)
{
    GPIO_SET_HIGH(SYS_LED1);
}
void ftd_mw_misc_manager_led2_on(uint32_t duration_ms)
{
    if (duration_ms > 0)
    {
        ftd_mw_misc_manager_soft_timer_start(FTD_MV_SOFT_TIMER_ID_LED2, duration_ms, (soft_timer_cb_t)ftd_mw_misc_manager_led2_off, NULL);
    }
    GPIO_SET_LOW(SYS_LED2);
}

void ftd_mw_misc_manager_led2_off(void)
{
    GPIO_SET_HIGH(SYS_LED2);
}
