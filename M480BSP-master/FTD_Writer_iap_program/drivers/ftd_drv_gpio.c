/****************************************************************************
@FILENAME:  ftd_drv_gpio.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.23
@COPYRIGHT: FTD co.ltd
****************************************************************************/

#include "ftd_drv_gpio.h"
#include "ftd_mw_log_manager.h"

static void (*g_fp_key_event_callback)(void) = NULL;

static void (*g_fp_burn_start_ch1_event_callback)(void) = NULL;
static void (*g_fp_burn_start_ch2_event_callback)(void) = NULL;
static void (*g_fp_burn_start_ch3_event_callback)(void) = NULL;
static void (*g_fp_burn_start_ch4_event_callback)(void) = NULL;

void ftd_drv_gpio_init(void)
{
    static uint8_t s_gpio_init_flag = 0;

    if (s_gpio_init_flag == 0)
    {
        // PWR_EN_CH
        GPIO_SetMode(PH, BIT4, GPIO_MODE_OUTPUT);
        GPIO_SetMode(PB, BIT8, GPIO_MODE_OUTPUT);
        GPIO_SetMode(PB, BIT9, GPIO_MODE_OUTPUT);
        GPIO_SetMode(PH, BIT5, GPIO_MODE_OUTPUT);
        // UART_EN_CH
        GPIO_SetMode(PG, BIT2, GPIO_MODE_OUTPUT);
        GPIO_SetMode(PB, BIT10, GPIO_MODE_OUTPUT);
        GPIO_SetMode(PB, BIT11, GPIO_MODE_OUTPUT);
#if 1
        // for log uart
        GPIO_SetMode(PH, BIT6, GPIO_MODE_OUTPUT);
#endif
        // PWR_DET_CH
        GPIO_SetMode(PB, BIT3, GPIO_MODE_INPUT);
        GPIO_SetMode(PB, BIT6, GPIO_MODE_INPUT);
        GPIO_SetMode(PB, BIT7, GPIO_MODE_INPUT);
        GPIO_SetMode(PB, BIT2, GPIO_MODE_INPUT);
        // PASS_CH
        GPIO_SetMode(PF, BIT9, GPIO_MODE_OUTPUT);
        GPIO_SetMode(PA, BIT9, GPIO_MODE_OUTPUT);
        GPIO_SetMode(PA, BIT8, GPIO_MODE_OUTPUT);
        GPIO_SetMode(PF, BIT8, GPIO_MODE_OUTPUT);

        // NG_CH
        GPIO_SetMode(PF, BIT11, GPIO_MODE_OUTPUT);
        GPIO_SetMode(PA, BIT11, GPIO_MODE_OUTPUT);
        GPIO_SetMode(PA, BIT10, GPIO_MODE_OUTPUT);
        GPIO_SetMode(PF, BIT10, GPIO_MODE_OUTPUT);

        // START_CH
        GPIO_SetMode(PG, BIT3, GPIO_MODE_INPUT);
        GPIO_SetMode(PC, BIT12, GPIO_MODE_OUTPUT);
        GPIO_SetMode(PC, BIT11, GPIO_MODE_INPUT);
        GPIO_SetMode(PG, BIT4, GPIO_MODE_INPUT);

        GPIO_SetMode(PE, BIT0, GPIO_MODE_OUTPUT);  // GPIO_RV1
        GPIO_SetMode(PH, BIT8, GPIO_MODE_OUTPUT);  // GPIO_RV2

        GPIO_SetMode(PD, BIT14, GPIO_MODE_OUTPUT);  // BUZZER
        GPIO_SetMode(PH, BIT11, GPIO_MODE_OUTPUT);  // LED1
        GPIO_SetMode(PE, BIT2, GPIO_MODE_OUTPUT);  // LED2
        GPIO_SetMode(PH, BIT10, GPIO_MODE_OUTPUT);  // LCD_EN

        // GPIO_SetMode(PA, BIT11, GPIO_MODE_INPUT);  // MECH_KEY
        GPIO_SetMode(PE, BIT1, GPIO_MODE_QUASI);  // MECH_KEY
        GPIO_SetPullCtl(PE, BIT1, GPIO_PUSEL_PULL_UP); //PE1  START_CH1
        GPIO_EnableInt(PE, 1, GPIO_INT_FALLING);
        NVIC_EnableIRQ(GPE_IRQn);

        GPIO_SetMode(PA, BIT4, GPIO_MODE_OUTPUT);  // FLASH_WP

        GPIO_SetPullCtl(PC, BIT11, GPIO_PUSEL_PULL_UP); //PC11  START_CH1
        GPIO_EnableInt(PC, 11, GPIO_INT_FALLING);
        NVIC_EnableIRQ(GPC_IRQn);


        // GPIO_SetPullCtl(PC, BIT12, GPIO_PUSEL_PULL_UP); //PC12  START_CH2
        // GPIO_EnableInt(PC, 12, GPIO_INT_FALLING);
        // NVIC_EnableIRQ(GPC_IRQn);

        GPIO_SetPullCtl(PG, BIT4, GPIO_PUSEL_PULL_UP); //PG4  START_CH3
        GPIO_EnableInt(PG, 4, GPIO_INT_FALLING);
        NVIC_EnableIRQ(GPG_IRQn);

        GPIO_SetPullCtl(PG, BIT3, GPIO_PUSEL_PULL_UP); //PG3  START_CH4
        GPIO_EnableInt(PG, 3, GPIO_INT_FALLING);
        NVIC_EnableIRQ(GPG_IRQn);

        s_gpio_init_flag = 1;
    }
}

void ftd_drv_gpio_set_key_event_call_back(void (*fp_callback)(void))
{
    g_fp_key_event_callback = fp_callback;
}

void GPE_IRQHandler(void)
{
    if (GPIO_GET_INT_FLAG(PE, BIT1))
    {
        GPIO_CLR_INT_FLAG(PE, BIT1);

        if (g_fp_key_event_callback != NULL)
        {
            JYMC_LOG_INFO("key INT occurred.\n");
            g_fp_key_event_callback();
        }

    }
    else
    {
        /* Un-expected interrupt. Just clear all PC interrupts */
        PE->INTSRC = PE->INTSRC;
        JYMC_LOG_INFO("Un-expected interrupts.\n");
    }
}

void ftd_drv_gpio_set_burn_start_ch1_event_call_back(void (*fp_callback)(void))
{
    g_fp_burn_start_ch1_event_callback = fp_callback;
}

void ftd_drv_gpio_set_burn_start_ch2_event_call_back(void (*fp_callback)(void))
{
    g_fp_burn_start_ch2_event_callback = fp_callback;
}

void ftd_drv_gpio_set_burn_start_ch3_event_call_back(void (*fp_callback)(void))
{
    g_fp_burn_start_ch3_event_callback = fp_callback;
}

void ftd_drv_gpio_set_burn_start_ch4_event_call_back(void (*fp_callback)(void))
{
    g_fp_burn_start_ch4_event_callback = fp_callback;
}

void GPC_IRQHandler(void)
{
    if (GPIO_GET_INT_FLAG(PC, BIT11))//PC11  START_CH1
    {
        GPIO_CLR_INT_FLAG(PC, BIT11);

        if (g_fp_burn_start_ch1_event_callback != NULL)
        {
            JYMC_LOG_INFO("burn start ch1 INT occurred.\n");
            g_fp_burn_start_ch1_event_callback();
        }
    }
    if (GPIO_GET_INT_FLAG(PC, BIT12)) // PC12  START_CH2
    {
        GPIO_CLR_INT_FLAG(PC, BIT12);

        if (g_fp_burn_start_ch2_event_callback != NULL)
        {
            JYMC_LOG_INFO("burn start ch2 INT occurred.\n");
            g_fp_burn_start_ch2_event_callback();
        }
    }
    else
    {
        /* Un-expected interrupt. Just clear all PC interrupts */
        PC->INTSRC = PC->INTSRC;
        JYMC_LOG_INFO("Un-expected interrupts.\n");
    }
}

void GPG_IRQHandler(void)
{
    if (GPIO_GET_INT_FLAG(PG, BIT3))//PG3  START_CH4
    {
        GPIO_CLR_INT_FLAG(PG, BIT3);

        if (g_fp_burn_start_ch4_event_callback != NULL)
        {
            JYMC_LOG_INFO("burn start ch4 INT occurred.\n");
            g_fp_burn_start_ch4_event_callback();
        }
    }
    if (GPIO_GET_INT_FLAG(PG, BIT4)) // PG4  START_CH3
    {
        GPIO_CLR_INT_FLAG(PG, BIT4);

        if (g_fp_burn_start_ch3_event_callback != NULL)
        {
            JYMC_LOG_INFO("burn start ch3 INT occurred.\n");
            g_fp_burn_start_ch3_event_callback();
        }
    }
    else
    {
        /* Un-expected interrupt. Just clear all PC interrupts */
        PG->INTSRC = PG->INTSRC;
        JYMC_LOG_INFO("Un-expected interrupts.\n");
    }
}
