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
#include "NuMicro.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 


void ftd_mw_misc_manager_init(void)
{
    ftd_drv_timer_init();

}

uint32_t ftd_mw_misc_manager_get_time_ms(void)
{
    return ftd_drv_timer_get_time_ms();
}


