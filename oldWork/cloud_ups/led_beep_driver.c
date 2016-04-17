/*

FileName:

Author:

Description:

Version:

FunctionList:

History:
    <author>  <time>  <version>  <desc>
 
*/
#include "led_beep_driver.h"
#include "power_checker.h"

static int fd;
int ret;
unsigned char key_val;

void driver_initial(void)
{

   fd = open("/dev/led", O_RDWR);
   if( fd < 0 )
   {
    perror("Driver fd open");
   }

}

unsigned char do_check(void)
{
	
	read(fd, &key_val, 1);
    //printf("key_val = 0x%x\n", key_val);
	
	return key_val;
}


int Red_led(int var)
{
   ret = ioctl(fd, var, 0);
   return ret;
}

int Green_led(int var)
{
    ret = ioctl(fd, var, 2);
    return ret;
}

int Beep_alarm(int var)
{
    ret = ioctl(fd, var, 1);
        return ret;
}

