#ifndef __FTD_DRV_TIMER_H__
#define __FTD_DRV_TIMER_H__

#include "NuMicro.h"
#include <stdint.h>


void ftd_drv_timer_soft_timer_init(void);
void ftd_drv_timer_set_soft_timer_call_back(void (*fp_callback)(void));


void ftd_drv_timer_init(void);
uint32_t ftd_drv_timer_get_time_ms(void);



#endif


