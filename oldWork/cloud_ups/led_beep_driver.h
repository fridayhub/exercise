/*

FileName:

Author:

Description:

Version:

FunctionList:

History:
    <author>  <time>  <version>  <desc>
 
*/


#ifndef DRIVER_H_
#define DRIVER_H_
#include <errno.h>

void driver_initial(void);
int  Red_led(int);
int  Green_led(int);
int  Beep_alarm(int);
extern unsigned char key_val;
unsigned char do_check();

#endif
