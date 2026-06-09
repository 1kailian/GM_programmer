#ifndef __FTD_MW_MISC_MANAGER_H__
#define __FTD_MW_MISC_MANAGER_H__ 

#include <stdint.h>
#include <stdbool.h>


#define SOFT_TIMER_MAX   8
typedef void (*soft_timer_cb_t)(void* arg);

typedef struct
{
    uint32_t         remain;
    soft_timer_cb_t  cb;
    void* arg;
    uint8_t          active;
} soft_timer_t;

enum {
    FTD_MV_SOFT_TIMER_ID_BUZZER = 0,
    FTD_MV_SOFT_TIMER_ID_LED1,
    FTD_MV_SOFT_TIMER_ID_LED2,
    FTD_MV_SOFT_TIMER_ID_MAX,
};


uint32_t ftd_mw_misc_manager_get_time_ms(void);
void ftd_mw_misc_manager_init(void);
void ftd_mw_misc_manager_key_press_call_back(void);
bool ftd_mw_misc_manager_is_key_press(void);

void ftd_mw_misc_manager_buzzer_open(uint32_t duration_ms);
void ftd_mw_misc_manager_buzzer_close(void);

void ftd_mw_misc_manager_led1_on(uint32_t duration_ms);
void ftd_mw_misc_manager_led1_off(void);
void ftd_mw_misc_manager_led2_on(uint32_t duration_ms);
void ftd_mw_misc_manager_led2_off(void);

void ftd_mw_misc_manager_soft_timer_init(void);
void ftd_mw_misc_manager_soft_timer_start(uint8_t id, uint32_t timeout, soft_timer_cb_t cb, void* arg);
void ftd_mw_misc_manager_soft_timer_stop(uint8_t id);


#endif

