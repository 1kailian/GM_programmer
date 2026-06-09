#ifndef __FTD_DRV_BUZZER_EPWM_H__
#define __FTD_DRV_BUZZER_EPWM_H__

#include "NuMicro.h"

#define FTD_DRV_BUZZER_EPWM_PORT       EPWM0
#define FTD_DRV_BUZZER_EPWM_CH         4
#define FTD_DRV_BUZZER_EPWM_CH_BIT     BIT4
#define FTD_DRV_BUZZER_EPWM_FREQ_HZ    2760 // 2.76 kHz
#define FTD_DRV_BUZZER_EPWM_DUTY       50   // 50% duty cycle

void ftd_drv_buzzer_epwm_init(void);
void ftd_drv_buzzer_epwm_start(void);
void ftd_drv_buzzer_epwm_stop(void);


#endif /* __FTD_DRV_BUZZER_EPWM_H__ */
