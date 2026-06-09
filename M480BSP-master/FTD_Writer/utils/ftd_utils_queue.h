/****************************************************************************
@FILENAME  :   ftd_utils_queue.h
@BRIEF     :   
@AUTHOR    :   
@DATE      :   
****************************************************************************/

#ifndef _FTD_UTILS_QUEUE__H_
#define _FTD_UTILS_QUEUE__H_

#include <stdint.h>

/************************************MACRO***********************************/
#ifdef  USER_DEFINE_UART_RECEIVE_BUFF_LMT
#define UART_RECEIVE_BUFF_LMT                           USER_DEFINE_UART_RECEIVE_BUFF_LMT
#else
#define UART_RECEIVE_BUFF_LMT                           (90)
#endif
#ifdef  USER_DEFINE_UART_PROCESS_BUFF_LMT
#define UART_PROCESS_BUFF_LMT                           USER_DEFINE_UART_PROCESS_BUFF_LMT
#else
#define UART_PROCESS_BUFF_LMT                           (90)
#endif

typedef struct
{
    uint8_t  receive_buff[UART_RECEIVE_BUFF_LMT];
    uint8_t  process_buff[UART_PROCESS_BUFF_LMT];
    uint8_t* queue_in;
    uint8_t* queue_out;
}str_ftd_uart_queue;

/*************************************API************************************/
void        ftd_uart_queue_init(void);
void        ftd_uart_queue_receive(uint8_t data);
uint8_t     ftd_uart_queue_is_empty(void);
uint8_t     ftd_uart_queue_read_byte(void);
uint8_t*    ftd_uart_queue_get_process_buff(void);

#endif

