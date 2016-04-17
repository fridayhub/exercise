#ifndef UART_H
#define UART_H

#define BAUD 9600 //波特率采用9600bps
#define BAUD_H 0x00
#define BAUD_L 0x0C 

void uart_init(void);
void uart_putchar(char);
int uart_getchar(void);
char uart_waitchar(void);

#endif