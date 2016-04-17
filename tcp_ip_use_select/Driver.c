#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


static int fd;
int ret;

void driver_initial(void)
{

   fd = open("/dev/led", O_RDWR);
   if( fd < 0 )
   {
    perror("Driver fd open");
   }

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

