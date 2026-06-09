#ifndef __FTD_DRV_GPIO_H__
#define __FTD_DRV_GPIO_H__

#include "NuMicro.h"

#define PEN_CH1         PB9
#define PEN_CH2         PB8
#define PEN_CH3         PH5
#define PEN_CH4         PH4

#define UART_EN_CH1     PG2
#define UART_EN_CH2     PB10
#define UART_EN_CH3     PB11
#define UART_EN_CH4     PH6

#define PWR_DET_CH1     PB7
#define PWR_DET_CH2     PB6
#define PWR_DET_CH3     PB2
#define PWR_DET_CH4     PB3

#define PASS_CH1        PA8
#define PASS_CH2        PA9
#define PASS_CH3        PF8
#define PASS_CH4        PF9

#define NG_CH1          PA10
#define NG_CH2          PA11
#define NG_CH3          PF10
#define NG_CH4          PF11

#define START_CH1       PC11
#define START_CH2       PC12
#define START_CH3       PG4
#define START_CH4       PG3

#define GPIO_RV1        PE0
#define GPIO_RV2        PH8

#define BUZZER          PD14
#define SYS_LED1        PH11
#define SYS_LED2        PE2
#define LCD_EN          PH10
#define MECH_KEY1       PE1
#define FLASH0_WP       PA4


#define GPIO_SET_HIGH(pin)    (pin = 1)
#define GPIO_SET_LOW(pin)     (pin = 0)
#define GPIO_GET_VALUE(pin)   (pin == 1)

void ftd_drv_gpio_init(void);
void ftd_drv_gpio_set_key_event_call_back(void (*fp_callback)(void));

void ftd_drv_gpio_set_burn_start_ch1_event_call_back(void (*fp_callback)(void));
void ftd_drv_gpio_set_burn_start_ch2_event_call_back(void (*fp_callback)(void));
void ftd_drv_gpio_set_burn_start_ch3_event_call_back(void (*fp_callback)(void));
void ftd_drv_gpio_set_burn_start_ch4_event_call_back(void (*fp_callback)(void));


#endif


