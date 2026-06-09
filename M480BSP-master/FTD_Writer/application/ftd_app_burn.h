#ifndef __FTD_APP_BURN_H__ 
#define __FTD_APP_BURN_H__

#include "ftd_data_model.h"
#include "stdint.h"

#if DEBUG_USE_UART4
#define BURN_CHANNEL (3) // for debug only, 4 channel normally
#else
#define BURN_CHANNEL (4) // for debug only, 4 channel normally
#endif

void ftd_app_burn_init(void);
void ftd_app_burn_process(void);
void ftd_app_burn_deinit(void);
void ftd_app_burn_fw_select(uint8_t fw_num);
void ftd_app_burn_set_wind_up(uint8_t value);
BRUN_STATUS_E ftd_app_burn_get_general_burn_state(void);

/////////////////////////////// debug code //////////////////////////////////
void ftd_app_burn_debug_hc_set_burn_addr(uint8_t is_test);
void ftd_app_burn_debug_hc_set_burn_state(uint8_t state_id);
/////////////////////////////// debug code end //////////////////////////////


#endif
