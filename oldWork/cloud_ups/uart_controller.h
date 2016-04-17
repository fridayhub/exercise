/*
FileName:
Author:
*/

#ifndef UART_CONTROLLER_H
#define UART_CONTROLLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <error.h>

int do_uart_alarm(char* buf);
int uart_init(void);
void do_uart_alarm_exit();

#endif
