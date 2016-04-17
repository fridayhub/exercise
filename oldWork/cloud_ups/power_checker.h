/*

FileName:

Author:

Description:

Version:

FunctionList:

History:
    <author>  <time>  <version>  <desc>
 
*/

#ifndef POWER_CHECKER_H
#define POWER_CHECKER_H

/* for use pthread_create */
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "self_controller.h"
#include "uart_controller.h"
#include "tcp_server.h"

int run_check_thread(void);

int init(void);
int CheckPower_test(void);

#endif
