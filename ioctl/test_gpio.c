#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define on 1
#define off 0

int main(int argc, char **argv)
{
    int led_num;
    int ret = 0;
    
    if( argc != 3)
    {
        printf("Usage:%s<ON/OFF><led_num>\n", argv[0]);
        return 0;
    }

    int fd;

    fd = open("/dev/led", O_RDWR);
    if( fd < 0 )
    {
        printf("open error\n");
        return 0;
    }

    led_num = *(argv[2]+0)-48;
    printf("led_num = %d\n", led_num);

    if( !strcmp(argv[1], "on"))
    {  
        ret =  ioctl(fd, on, (led_num-1)); 
        printf("send on %d\n", ret);
    }
    else if (!strcmp(argv[1], "off"))
    {
        ret = ioctl(fd, off, (led_num-1));
        printf("send off %d\n", ret);
    }
    return 0;
}
