/****************************************************************************
@FILENAME  :   ftd_utils_queue.c
@BRIEF     :
@AUTHOR    :   
@DATE      :   
****************************************************************************/
#include "NuMicro.h"
#include "ftd_utils_queue.h"

//#ifdef FTD_UART_QUEUE_ENABLE
#if 1
str_ftd_uart_queue uart_queue;

void ftd_uart_queue_init(void)
{
    memset(&uart_queue, 0x00, sizeof(str_ftd_uart_queue));
    uart_queue.queue_in = (uint8_t *)uart_queue.receive_buff;
    uart_queue.queue_out = (uint8_t *)uart_queue.receive_buff;
}

void ftd_uart_queue_receive(uint8_t data)
{
    if (0x1 == (uart_queue.queue_out - uart_queue.queue_in))
    {
        // uart rx queue is full,not allow to fill data
    }
    else if ((uart_queue.queue_in > uart_queue.queue_out) && ((uart_queue.queue_in - uart_queue.queue_out) >= UART_RECEIVE_BUFF_LMT))
    {
        // uart rx queue is full,not allow to fill data
    }
    else
    {
        if (uart_queue.queue_in >= (uint8_t *)(uart_queue.receive_buff + UART_RECEIVE_BUFF_LMT))
        {
            uart_queue.queue_in = (uint8_t *)uart_queue.receive_buff;
        }

        *uart_queue.queue_in++ = data;
    }
}

uint8_t ftd_uart_queue_is_empty(void)
{
    if (uart_queue.queue_out == uart_queue.queue_in)
    {
        return 0x1;
    }
    else
    {
        return 0x0;
    }
}

uint8_t ftd_uart_queue_read_byte(void)
{
    uint8_t byte = 0x00;

    if (uart_queue.queue_out != uart_queue.queue_in)
    {
        if (uart_queue.queue_out >= (uint8_t *)(uart_queue.receive_buff + UART_RECEIVE_BUFF_LMT))
        {
            uart_queue.queue_out = (uint8_t *)uart_queue.receive_buff;
        }

        byte = *uart_queue.queue_out++;
    }

    return byte;
}

uint8_t *ftd_uart_queue_get_process_buff(void)
{
    return uart_queue.process_buff;
}
#endif
