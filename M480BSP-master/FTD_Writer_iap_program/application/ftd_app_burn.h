#ifndef __FTD_APP_BURN_H__ 
#define __FTD_APP_BURN_H__

#include "stdint.h"

void ftd_app_burn_init(void);
void ftd_app_burn_process(void);
void ftd_app_burn_deinit(void);

void ftd_app_burn_fw_select(uint8_t fw_num);
void ftd_app_burn_set_auto(uint8_t auto_burn);
void ftd_app_burn_manual_trigger(void);
uint8_t ftd_app_burn_get_burn_done_state(void);

/////////////////////////////// debug code //////////////////////////////////
void ftd_app_burn_debug_hc_set_burn_addr(uint8_t is_test);
void ftd_app_burn_debug_hc_set_burn_state(uint8_t state_id);
/////////////////////////////// debug code end///////////////////////////////


#endif
