#ifndef __FTD_APP_STATE_MACHINE_H__ 
#define __FTD_APP_STATE_MACHINE_H__

#include "ftd_data_model.h"

typedef struct {
    void (*init)(void);
    void (*process)(void);
    void (*deinit)(void);
} STATE_HANDLER;

APPLICATION app_ftd_state_machine_get_current_state(void);
void app_ftd_state_machine_change_state(APPLICATION new_state);
void app_ftd_state_machine_process(void);

#endif
