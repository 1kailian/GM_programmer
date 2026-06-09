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

// Cyclic task structure
typedef struct {
    uint8_t channel;
    void (*send_func)(uint8_t, uint8_t*, uint16_t);
    uint8_t* buf;
    uint16_t buf_len;
    uint32_t period_ms;
    uint32_t last_send_time;
    bool active;
} cyclic_task_info_t;

// Static array to store cyclic task info for each channel
static cyclic_task_info_t g_cyclic_tasks[4] = {
    {0, NULL, NULL, 0, 0, 0, false},
    {1, NULL, NULL, 0, 0, 0, false},
    {2, NULL, NULL, 0, 0, 0, false},
    {3, NULL, NULL, 0, 0, 0, false}
};

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

static void ftd_mw_misc_manager_cyclic_task_call_back(void)
{
    uint32_t current_time = ftd_mw_misc_manager_get_time_ms();

    for (int i = 0; i < 4; i++)
    {
        if (g_cyclic_tasks[i].active)
        {
            if (current_time - g_cyclic_tasks[i].last_send_time >= g_cyclic_tasks[i].period_ms)
            {
                // Execute the send function
                if (g_cyclic_tasks[i].send_func && g_cyclic_tasks[i].buf && g_cyclic_tasks[i].buf_len > 0)
                {
                    g_cyclic_tasks[i].send_func(g_cyclic_tasks[i].channel,
                        g_cyclic_tasks[i].buf,
                        g_cyclic_tasks[i].buf_len);
                }

                // Update last send time
                g_cyclic_tasks[i].last_send_time = current_time;
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
    ftd_drv_timer_cyclic_init();
    ftd_drv_timer_set_cyclic_call_back(ftd_mw_misc_manager_cyclic_task_call_back);
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

/**
 * @brief Start cyclic send task for a specific channel
 * @param channel Channel number (0-3)
 * @param send_func Send function pointer
 * @param buf Send buffer
 * @param buf_len Buffer length
 * @param period_ms Send period in milliseconds
 */
void ftd_mw_misc_manager_start_cyclic_task(uint8_t channel,
    void (*send_func)(uint8_t, uint8_t*, uint16_t),
    uint8_t* buf,
    uint16_t buf_len,
    uint32_t period_ms)
{
    if (channel >= 4 || !send_func || !buf || buf_len == 0 || period_ms == 0)
    {
        FTD_LOG_ERROR("Invalid parameters for cyclic task - channel:%d, send_func:%p, buf:%p, buf_len:%d, period:%d",
            channel, send_func, buf, buf_len, period_ms);
        return;
    }

    // Initialize cyclic task info
    g_cyclic_tasks[channel].channel = channel;
    g_cyclic_tasks[channel].send_func = send_func;
    g_cyclic_tasks[channel].buf = buf;
    g_cyclic_tasks[channel].buf_len = buf_len;
    g_cyclic_tasks[channel].period_ms = period_ms;
    g_cyclic_tasks[channel].last_send_time = ftd_mw_misc_manager_get_time_ms();
    g_cyclic_tasks[channel].active = true;

    FTD_LOG_INFO("Cyclic task started - channel:%d, period:%dms, buf_len:%d",
        channel, period_ms, buf_len);
}

/**
 * @brief Stop cyclic send task for a specific channel
 * @param channel Channel number (0-3)
 */
void ftd_mw_misc_manager_stop_cyclic_task(uint8_t channel)
{
    if (channel >= 4)
        return;

    g_cyclic_tasks[channel].active = false;
}
