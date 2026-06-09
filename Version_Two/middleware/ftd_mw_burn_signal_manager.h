#ifndef FTD_MW_BURN_SIGNAL_MANAGER_H
#define FTD_MW_BURN_SIGNAL_MANAGER_H

#include "ftd_drv_gpio.h"
#include "stdbool.h"
#include "stdint.h"


typedef enum {
    BURN_CH1 = 0,
    BURN_CH2,
    BURN_CH3,
    BURN_CH4
} BURN_CH;

typedef enum
{
    STATE_INIT = 0,
    STATE_BUSY,
    STATE_PASS,
    STATE_NG,
    STATE_OTHER,
} SIGNAL_STATE;

typedef struct {
    bool burn_start_flag;
    uint32_t burn_start_trigger_time;
    SIGNAL_STATE signal_state;
} BURN_CH_STATE_T;

void ftd_mw_burn_signal_manager_init(void);
void ftd_mw_burn_signal_manager_ch_set_signal(BURN_CH ch, SIGNAL_STATE state);
void ftd_mw_burn_signal_manager_test(void);
bool ftd_mw_burn_signal_manager_get_ch_state(uint8_t ch);

#endif


