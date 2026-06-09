/****************************************************************************
@FILENAME:  ftd_app_state_machine.c
@FUNCTION:
@AUTHOR:    yanxijiang
@DATE:      2025.10.27
@COPYRIGHT: FTD co.ltd
****************************************************************************/
#include "ftd_app_state_machine.h"
#include "ftd_app_prog.h"
#include "stdio.h"


static APPLICATION g_e_current_state = APP_PROGRAMMING;
// static APPLICATION g_e_previous_state = APP_DEFALUT;

static const STATE_HANDLER state_handlers[] = {
    [APP_PROGRAMMING] =
    {
        .init = ftd_app_prog_init,
        .process = ftd_app_prog_process,
        .deinit = ftd_app_prog_deinit,
    }, 
};

APPLICATION app_ftd_state_machine_get_current_state(void)
{
    return g_e_current_state;
}

void app_ftd_state_machine_change_state(APPLICATION new_state)
{
    if (new_state >= sizeof(state_handlers)/sizeof(state_handlers[0]))
    {
        return; 
    }
    
    if (state_handlers[g_e_current_state].deinit != NULL)
    {
        state_handlers[g_e_current_state].deinit();
    }
    
    // g_e_previous_state = g_e_current_state;
    g_e_current_state = new_state;
    
    if (state_handlers[g_e_current_state].init != NULL)
    {
        state_handlers[g_e_current_state].init();
    }
}

void app_ftd_state_machine_process(void)
{
    if (state_handlers[g_e_current_state].process != NULL)
    {
        state_handlers[g_e_current_state].process();
    }
}
