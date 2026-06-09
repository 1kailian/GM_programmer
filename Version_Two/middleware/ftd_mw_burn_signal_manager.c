#include "ftd_mw_burn_signal_manager.h"
#include "ftd_mw_log_manager.h"
#include "ftd_mw_misc_manager.h"

//#define PASS_PULSE_TIME  (300) // ms
//#define NG_PULSE_TIME    (300) // ms

#define SIGNAL_INFO     FTD_LOG_INFO
#define SIGNAL_DEBUG     FTD_LOG_DEBUG


volatile BURN_CH_STATE_T burn_ch1_state = { 0 };
volatile BURN_CH_STATE_T burn_ch2_state = { 0 };
volatile BURN_CH_STATE_T burn_ch3_state = { 0 };
volatile BURN_CH_STATE_T burn_ch4_state = { 0 };

// Debounce callback functions
static void ftd_mw_burn_signal_manager_debounce_ch1(void* arg)
{
    // Check actual GPIO level after debounce
    if (GPIO_GET_VALUE(START_CH1) == 0) // START_CH1
    {
        burn_ch1_state.burn_start_flag = true;
        burn_ch1_state.burn_start_trigger_time = ftd_mw_misc_manager_get_time_ms();
        SIGNAL_INFO("start_ch0 debounced: flag=%d, time=%d", burn_ch1_state.burn_start_flag, burn_ch1_state.burn_start_trigger_time);
    }
    else
    {
        SIGNAL_DEBUG("start_ch0 debounce failed: level high");
    }
}

static void ftd_mw_burn_signal_manager_debounce_ch2(void* arg)
{
    // Check actual GPIO level after debounce
    if (GPIO_GET_VALUE(START_CH2) == 0) // START_CH2
    {
        burn_ch2_state.burn_start_flag = true;
        burn_ch2_state.burn_start_trigger_time = ftd_mw_misc_manager_get_time_ms();
        SIGNAL_INFO("start_ch1 debounced: flag=%d, time=%d", burn_ch2_state.burn_start_flag, burn_ch2_state.burn_start_trigger_time);
    }
    else
    {
        SIGNAL_DEBUG("start_ch1 debounce failed: level high");
    }
}

static void ftd_mw_burn_signal_manager_debounce_ch3(void* arg)
{
    // Check actual GPIO level after debounce
    if (GPIO_GET_VALUE(START_CH3) == 0) // START_CH3
    {
        burn_ch3_state.burn_start_flag = true;
        burn_ch3_state.burn_start_trigger_time = ftd_mw_misc_manager_get_time_ms();
        SIGNAL_INFO("start_ch2 debounced: flag=%d, time=%d", burn_ch3_state.burn_start_flag, burn_ch3_state.burn_start_trigger_time);
    }
    else
    {
        SIGNAL_DEBUG("start_ch2 debounce failed: level high");
    }
}

static void ftd_mw_burn_signal_manager_debounce_ch4(void* arg)
{
    // Check actual GPIO level after debounce
    if (GPIO_GET_VALUE(START_CH4) == 0) // START_CH4
    {
        burn_ch4_state.burn_start_flag = true;
        burn_ch4_state.burn_start_trigger_time = ftd_mw_misc_manager_get_time_ms();
        SIGNAL_INFO("start_ch3 debounced: flag=%d, time=%d", burn_ch4_state.burn_start_flag, burn_ch4_state.burn_start_trigger_time);
    }
    else
    {
        SIGNAL_DEBUG("start_ch3 debounce failed: level high");
    }
}

static void ftd_mw_burn_signal_manager_burn_start_ch1_callback(void)
{
    // Start debounce timer
    ftd_mw_misc_manager_soft_timer_start(FTD_MV_SOFT_TIMER_ID_START_CH1, 100,
        (soft_timer_cb_t)ftd_mw_burn_signal_manager_debounce_ch1, NULL);
    SIGNAL_DEBUG("start_ch0 interrupt detected, debounce started");
}

static void ftd_mw_burn_signal_manager_burn_start_ch2_callback(void)
{
    // Start debounce timer
    ftd_mw_misc_manager_soft_timer_start(FTD_MV_SOFT_TIMER_ID_START_CH2, 100,
        (soft_timer_cb_t)ftd_mw_burn_signal_manager_debounce_ch2, NULL);
    SIGNAL_DEBUG("start_ch1 interrupt detected, debounce started");
}

static void ftd_mw_burn_signal_manager_burn_start_ch3_callback(void)
{
    // Start debounce timer
    ftd_mw_misc_manager_soft_timer_start(FTD_MV_SOFT_TIMER_ID_START_CH3, 100,
        (soft_timer_cb_t)ftd_mw_burn_signal_manager_debounce_ch3, NULL);
    SIGNAL_DEBUG("start_ch2 interrupt detected, debounce started");
}

static void ftd_mw_burn_signal_manager_burn_start_ch4_callback(void)
{
    // Start debounce timer
    ftd_mw_misc_manager_soft_timer_start(FTD_MV_SOFT_TIMER_ID_START_CH4, 100,
        (soft_timer_cb_t)ftd_mw_burn_signal_manager_debounce_ch4, NULL);
    SIGNAL_DEBUG("start_ch3 interrupt detected, debounce started");
}

void ftd_mw_burn_signal_manager_init(void)
{
    ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH1, STATE_INIT);
    ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH2, STATE_INIT);
    ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH3, STATE_INIT);
    ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH4, STATE_INIT);

    ftd_drv_gpio_set_burn_start_ch1_event_call_back(ftd_mw_burn_signal_manager_burn_start_ch1_callback);
    ftd_drv_gpio_set_burn_start_ch2_event_call_back(ftd_mw_burn_signal_manager_burn_start_ch2_callback);
    ftd_drv_gpio_set_burn_start_ch3_event_call_back(ftd_mw_burn_signal_manager_burn_start_ch3_callback);
    ftd_drv_gpio_set_burn_start_ch4_event_call_back(ftd_mw_burn_signal_manager_burn_start_ch4_callback);

}

bool ftd_mw_burn_signal_manager_get_ch_state(uint8_t ch)
{
    switch (ch)
    {
        case 0:
        {
            if (burn_ch1_state.burn_start_flag == true)
            {
                burn_ch1_state.burn_start_flag = false;
                return true;
            }
            else {
                return false;
            }
        }
        case 1:
        {
            if (burn_ch2_state.burn_start_flag == true)
            {
                burn_ch2_state.burn_start_flag = false;
                return true;
            }
            else {
                return false;
            }
        }
        case 2:
        {
            if (burn_ch3_state.burn_start_flag == true)
            {
                burn_ch3_state.burn_start_flag = false;
                return true;
            }
            else {
                return false;
            }
        }
        case 3:
        {
            if (burn_ch4_state.burn_start_flag == true)
            {
                burn_ch4_state.burn_start_flag = false;
                return true;
            }
            else {
                return false;
            }
        }
        default:
            return false;
    }
}
void ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH ch, SIGNAL_STATE state)
{
    SIGNAL_DEBUG("burn_ch_set_signal: ch=%d, state=%d", ch, state);
    switch (ch)
    {
        case BURN_CH1:
        {
            switch (state)
            {
                case STATE_INIT:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH1, state=STATE_INIT");
                    burn_ch1_state.signal_state = STATE_INIT;
                    GPIO_SET_HIGH(PASS_CH1);
                    GPIO_SET_HIGH(NG_CH1);
                    // GPIO_SET_HIGH(BUSY_CH1);
                }
                break;
                case STATE_PASS:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH1, state=STATE_PASS");
                    burn_ch1_state.signal_state = STATE_PASS;
                    GPIO_SET_LOW(PASS_CH1);
                    GPIO_SET_HIGH(NG_CH1);

                    //for (uint16_t i = 0; i < PASS_PULSE_TIME; i++)
                    //    CLK_SysTickLongDelay(1000);

                    //GPIO_SET_HIGH(PASS_CH1);
                    //GPIO_SET_HIGH(NG_CH1);
                    //GPIO_SET_HIGH(BUSY_CH1);
                }
                break;
                case STATE_NG:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH1, state=STATE_NG");
                    burn_ch1_state.signal_state = STATE_NG;
                    GPIO_SET_HIGH(PASS_CH1);
                    GPIO_SET_LOW(NG_CH1);

                    //for (uint16_t i = 0; i < NG_PULSE_TIME; i++)
                    //    CLK_SysTickLongDelay(1000);

                    //GPIO_SET_HIGH(PASS_CH1);
                    //GPIO_SET_HIGH(NG_CH1);
                    //  GPIO_SET_HIGH(BUSY_CH1);
                }
                break;
                case STATE_BUSY:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH1, state=STATE_BUSY");
                    burn_ch1_state.signal_state = STATE_BUSY;
                    GPIO_SET_HIGH(PASS_CH1);
                    GPIO_SET_HIGH(NG_CH1);
                    // GPIO_SET_LOW(BUSY_CH1);
                }
                break;
                case STATE_OTHER:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH1, state=STATE_OTHER");
                    burn_ch1_state.signal_state = STATE_OTHER;
                    GPIO_SET_LOW(PASS_CH1);
                    GPIO_SET_LOW(NG_CH1);
                    // GPIO_SET_LOW(BUSY_CH1);
                }
                break;
                default:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH1, state=NOT_DEFINE");

                }
                break;
            }
        }
        break;
        case BURN_CH2:
        {
            switch (state)
            {
                case STATE_INIT:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH2, state=STATE_INIT");
                    burn_ch2_state.signal_state = STATE_INIT;
                    GPIO_SET_HIGH(PASS_CH2);
                    GPIO_SET_HIGH(NG_CH2);
                }
                break;
                case STATE_PASS:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH2, state=STATE_PASS");
                    burn_ch2_state.signal_state = STATE_PASS;
                    GPIO_SET_LOW(PASS_CH2);
                    GPIO_SET_HIGH(NG_CH2);
                }
                break;
                case STATE_NG:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH2, state=STATE_NG");
                    burn_ch2_state.signal_state = STATE_NG;
                    GPIO_SET_HIGH(PASS_CH2);
                    GPIO_SET_LOW(NG_CH2);
                }
                break;
                case STATE_BUSY:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH2, state=STATE_BUSY");
                    burn_ch2_state.signal_state = STATE_BUSY;
                    GPIO_SET_HIGH(PASS_CH2);
                    GPIO_SET_HIGH(NG_CH2);
                }
                break;
                case STATE_OTHER:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH2, state=STATE_OTHER");
                    burn_ch2_state.signal_state = STATE_OTHER;
                    GPIO_SET_LOW(PASS_CH2);
                    GPIO_SET_LOW(NG_CH2);
                }
                break;
                default:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH2, state=NOT_DEFINE");

                }
                break;
            }
        }
        break;
        case BURN_CH3:
        {
            switch (state)
            {
                case STATE_INIT:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH3, state=STATE_INIT");
                    burn_ch3_state.signal_state = STATE_INIT;
                    GPIO_SET_HIGH(PASS_CH3);
                    GPIO_SET_HIGH(NG_CH3);
                }
                break;
                case STATE_PASS:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH3, state=STATE_PASS");
                    burn_ch3_state.signal_state = STATE_PASS;
                    GPIO_SET_LOW(PASS_CH3);
                    GPIO_SET_HIGH(NG_CH3);
                }
                break;
                case STATE_NG:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH3, state=STATE_NG");
                    burn_ch3_state.signal_state = STATE_NG;
                    GPIO_SET_HIGH(PASS_CH3);
                    GPIO_SET_LOW(NG_CH3);
                }
                break;
                case STATE_BUSY:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH3, state=STATE_BUSY");
                    burn_ch3_state.signal_state = STATE_BUSY;
                    GPIO_SET_HIGH(PASS_CH3);
                    GPIO_SET_HIGH(NG_CH3);
                }
                break;
                case STATE_OTHER:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH3, state=STATE_OTHER");
                    burn_ch3_state.signal_state = STATE_OTHER;
                    GPIO_SET_LOW(PASS_CH3);
                    GPIO_SET_LOW(NG_CH3);
                }
                break;
                default:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH3, state=NOT_DEFINE");

                }
                break;
            }
        }
        break;
        case BURN_CH4:
        {
            switch (state)
            {
                case STATE_INIT:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH4, state=STATE_INIT");
                    burn_ch4_state.signal_state = STATE_INIT;
                    GPIO_SET_HIGH(PASS_CH4);
                    GPIO_SET_HIGH(NG_CH4);
                }
                break;
                case STATE_PASS:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH4, state=STATE_PASS");
                    burn_ch4_state.signal_state = STATE_PASS;
                    GPIO_SET_LOW(PASS_CH4);
                    GPIO_SET_HIGH(NG_CH4);
                }
                break;
                case STATE_NG:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH4, state=STATE_NG");
                    burn_ch4_state.signal_state = STATE_NG;
                    GPIO_SET_HIGH(PASS_CH4);
                    GPIO_SET_LOW(NG_CH4);
                }
                break;
                case STATE_BUSY:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH4, state=STATE_BUSY");
                    burn_ch4_state.signal_state = STATE_BUSY;
                    GPIO_SET_HIGH(PASS_CH4);
                    GPIO_SET_HIGH(NG_CH4);
                }
                break;
                case STATE_OTHER:
                {
                    SIGNAL_DEBUG("burn_ch_set_signal: ch=BURN_CH4, state=STATE_OTHER");
                    burn_ch4_state.signal_state = STATE_OTHER;
                    GPIO_SET_LOW(PASS_CH4);
                    GPIO_SET_LOW(NG_CH4);
                }
                break;
                default:
                {
                    SIGNAL_INFO("burn_ch_set_signal: ch=BURN_CH4, state=NOT_DEFINE");

                }
                break;
            }
        }
        break;
        default:
            break;
    }
}

// void ftd_mw_burn_signal_manager_test(void)
// {
//     static uint32_t test_count = 0;
//     if (test_count == 0)
//     {
//         test_count = ftd_mw_misc_manager_get_time_ms();
//         SIGNAL_INFO("test_count=%d", test_count);
//     }
//     switch (burn_ch1_state.signal_state)
//     {
//         case STATE_INIT:
//         {
//             if (burn_ch1_state.burn_start_flag == true)
//             {
//                 if (GPIO_GET_VALUE(BUSY_CH1) == 0)
//                 {
//                     uint32_t now = ftd_mw_misc_manager_get_time_ms();
//                     if (now - burn_ch1_state.burn_start_trigger_time >= 50)
//                     {
//                         ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH1, STATE_BUSY);
//                         break;
//                     }
//                     else {
//                         burn_ch1_state.burn_start_flag = false;
//                         break;
//                     }
//                 }
//                 else
//                 {
//                     FTD_LOG_INFO("burn_ch1_state.burn_start_flag = false\r\n");
//                     burn_ch1_state.burn_start_flag = false;
//                 }
//             }
//         }
//         break;
//         case STATE_BUSY:
//         {

//             // generate burn signal in here
//             //1s test
//             burn_ch1_state.burn_start_flag = false;
//             if (ftd_mw_misc_manager_get_time_ms() - burn_ch1_state.burn_start_trigger_time > 1000)
//             {
//                 test_count = ftd_mw_misc_manager_get_time_ms();
//                 SIGNAL_INFO("test_count=%d", test_count);
//                 if (test_count % 2 == 0)
//                     ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH1, STATE_PASS);
//                 else
//                     ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH1, STATE_NG);

//             }
//         }
//         break;
//         case STATE_PASS:
//         {
//             if (burn_ch1_state.burn_start_flag == true)
//             {
//                 if (GPIO_GET_VALUE(BUSY_CH1) == 0)
//                 {
//                     uint32_t now = ftd_mw_misc_manager_get_time_ms();
//                     if (now - burn_ch1_state.burn_start_trigger_time >= 50)
//                     {
//                         ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH1, STATE_BUSY);
//                         break;
//                     }
//                     else {
//                         burn_ch1_state.burn_start_flag = false;
//                         break;
//                     }
//                 }
//                 else
//                 {
//                     burn_ch1_state.burn_start_flag = false;
//                 }
//             }
//         }
//         break;
//         case STATE_NG:
//         {
//             if (burn_ch1_state.burn_start_flag == true)
//             {
//                 if (GPIO_GET_VALUE(BUSY_CH1) == 0)
//                 {
//                     uint32_t now = ftd_mw_misc_manager_get_time_ms();
//                     if (now - burn_ch1_state.burn_start_trigger_time >= 50)
//                     {
//                         ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH1, STATE_BUSY);
//                         break;
//                     }
//                     else {
//                         burn_ch1_state.burn_start_flag = false;
//                         break;
//                     }
//                 }
//                 else
//                 {
//                     burn_ch1_state.burn_start_flag = false;
//                 }
//             }
//         }
//         break;
//         case STATE_OTHER:
//         {

//         }
//         break;
//         default:
//         {
//             ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH1, STATE_INIT);
//         }
//         break;
//     }

// }


// void ftd_mw_burn_signal_manager_test(void)
// {
//     static uint32_t test_count = 0;
//     if (test_count == 0)
//     {
//         test_count = ftd_mw_misc_manager_get_time_ms();
//         SIGNAL_INFO("test_count=%d", test_count);
//     }
//     switch (burn_ch1_state.signal_state)
//     {
//         case STATE_INIT:
//         {
//             if (burn_ch1_state.burn_start_flag == true)
//             {
//                 ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH1, STATE_BUSY);
//                 break;
//             }
//         }
//         break;
//         case STATE_BUSY:
//         {

//             // generate burn signal in here
//             //1s test
//             burn_ch1_state.burn_start_flag = false;
//             if (ftd_mw_misc_manager_get_time_ms() - burn_ch1_state.burn_start_trigger_time > 1000)
//             {
//                 test_count = ftd_mw_misc_manager_get_time_ms();
//                 SIGNAL_INFO("test_count=%d", test_count);
//                 if (test_count % 2 == 0)
//                     ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH1, STATE_PASS);
//                 else
//                     ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH1, STATE_NG);

//             }
//         }
//         break;
//         case STATE_PASS:
//         {
//             if (burn_ch1_state.burn_start_flag == true)
//             {

//                 ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH1, STATE_BUSY);

//             }
//         }
//         break;
//         case STATE_NG:
//         {
//             if (burn_ch1_state.burn_start_flag == true)
//             {
//                 ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH1, STATE_BUSY);
//             }
//         }
//         break;
//         case STATE_OTHER:
//         {

//         }
//         break;
//         default:
//         {
//             ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH1, STATE_INIT);
//         }
//         break;
//     }

// }

void ftd_mw_burn_signal_manager_test(void)
{
    static uint32_t test_count = 0;
    if (test_count == 0)
    {
        test_count = ftd_mw_misc_manager_get_time_ms();
        SIGNAL_INFO("test_count=%d", test_count);
    }
    switch (burn_ch1_state.signal_state)
    {
        case STATE_INIT:
        {
            if (ftd_mw_burn_signal_manager_get_ch_state(0))
            {
                ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH1, STATE_BUSY);


                // if (GPIO_GET_VALUE(BUSY_CH1) == 0)
                // {
                //     uint32_t now = ftd_mw_misc_manager_get_time_ms();
                //     if (now - burn_ch1_state.burn_start_trigger_time >= 50)
                //     {
                //         ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH1, STATE_BUSY);
                //         break;
                //     }
                //     else {
                //         burn_ch1_state.burn_start_flag = false;
                //         break;
                //     }
                // }
                // else
                // {
                //     FTD_LOG_INFO("burn_ch1_state.burn_start_flag = false\r\n");
                //     burn_ch1_state.burn_start_flag = false;
                // }
            }
        }
        break;
        case STATE_BUSY:
        {

            // generate burn signal in here
            //1s test
            burn_ch1_state.burn_start_flag = false;
            if (ftd_mw_misc_manager_get_time_ms() - burn_ch1_state.burn_start_trigger_time > 1000)
            {
                test_count = ftd_mw_misc_manager_get_time_ms();
                SIGNAL_INFO("test_count=%d", test_count);
                if (test_count % 2 == 0)
                    ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH1, STATE_PASS);
                else
                    ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH1, STATE_NG);

            }
        }
        break;
        case STATE_PASS:
        {
            if (ftd_mw_burn_signal_manager_get_ch_state(0))
            {
                ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH1, STATE_BUSY);
            }
        }
        break;
        case STATE_NG:
        {
            if (ftd_mw_burn_signal_manager_get_ch_state(0))
            {
                ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH1, STATE_BUSY);
            }
        }
        break;
        case STATE_OTHER:
        {

        }
        break;
        default:
        {
            ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH1, STATE_INIT);
        }
        break;
    }

}
