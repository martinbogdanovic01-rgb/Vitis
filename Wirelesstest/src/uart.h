#ifndef UART_H
#define UART_H
#include "xil_types.h"
#include "xstatus.h"
int uart_init(void);
int uart_data_available(void);
u8  uart_read_char(void);
#endif
