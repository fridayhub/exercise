#include <linux/serial.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>  
#include <termios.h>       
#include <string.h>

/* Driver-specific ioctls: */
#define TIOCGRS485      0x542E
#define TIOCSRS485      0x542F

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned long U32;

#define  CMSPAR 010000000000

int fd;

//*********************************************************
//串口选项设置：
//*********************************************************
int setTermios(U16 uBaudRate)
{
	struct termios pNewtio, oldtio;
	 if( tcgetattr(fd, &oldtio) != 0) {
	  perror("SetupSerial 1");
	  return -1;
	 }

	bzero(&pNewtio, sizeof(struct termios)); /* clear struct for new port settings */

	//8N1
	pNewtio.c_cflag = CS8 |  CREAD | CLOCAL;
	pNewtio.c_iflag = IGNPAR;
	cfsetospeed(&pNewtio, uBaudRate); 
	cfsetispeed(&pNewtio, uBaudRate);
	
	//8E1
	//pNewtio.c_cflag = uBaudRate | CS8 |  CREAD | CLOCAL |PARENB;
	//pNewtio.c_iflag = 0;
	
	//8O1
	//pNewtio.c_cflag = uBaudRate | CS8 |  CREAD | CLOCAL |PARENB | PARODD;
	//pNewtio.c_iflag = 0;
	
	//8S1
	//pNewtio.c_cflag = uBaudRate | CS8 |  CREAD | CLOCAL |PARENB | CMSPAR;
	//pNewtio.c_iflag = 0;
	
	//8M1
	//pNewtio.c_cflag = uBaudRate | CS8 |  CREAD | CLOCAL |PARENB | PARODD | CMSPAR;
	//pNewtio.c_iflag = 0;
	
	pNewtio.c_oflag = 0;
	pNewtio.c_lflag = 0; //non ICANON
	/* 
	initialize all control characters 
	default values can be found in /usr/include/termios.h, and 
	are given in the comments, but we don't need them here
	*/
	pNewtio.c_cc[VINTR]    = 0;     /* Ctrl-c */ 
	pNewtio.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
	pNewtio.c_cc[VERASE]   = 0;     /* del */
	pNewtio.c_cc[VKILL]    = 0;     /* @ */
	pNewtio.c_cc[VEOF]     = 4;     /* Ctrl-d */
	pNewtio.c_cc[VTIME]    = 5;     /* inter-character timer, timeout VTIME*0.1 */
	pNewtio.c_cc[VMIN]     = 0;     /* blocking read until VMIN character arrives */
	pNewtio.c_cc[VSWTC]    = 0;     /* '\0' */
	pNewtio.c_cc[VSTART]   = 0;     /* Ctrl-q */ 
	pNewtio.c_cc[VSTOP]    = 0;     /* Ctrl-s */
	pNewtio.c_cc[VSUSP]    = 0;     /* Ctrl-z */
	pNewtio.c_cc[VEOL]     = 0;     /* '\0' */
	pNewtio.c_cc[VREPRINT] = 0;     /* Ctrl-r */
	pNewtio.c_cc[VDISCARD] = 0;     /* Ctrl-u */
	pNewtio.c_cc[VWERASE]  = 0;     /* Ctrl-w */
	pNewtio.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
	pNewtio.c_cc[VEOL2]    = 0;     /* '\0' */

	tcflush(fd,TCIFLUSH); // 改变在所有写入 fd 引用的对象的输出都被传输后生效，所有已接受但未读入的输入都在改变发生前丢弃。
	 if((tcsetattr(fd,TCSANOW, &pNewtio))!=0) //TCSANOW:改变立即发生
	 {
	  perror("com set error");
	  return -1;
	 }
	 printf("set done!\n\r");
	 return 0;
}

//*********************************************************
//写缓冲函数：
//*********************************************************

void send_data(char* writemsg)
{
    printf("send_data is :%s\n", writemsg);
    int i; 
    for(i = 0; i < strlen(writemsg); i++)
    {
        write(fd, &writemsg[i], 1);
        //usleep(1500);
        printf("write is : %c \n", writemsg[i]);
    }
    //write(fd1, &writemsg[strlen(writemsg)], 1); 
    printf("write done\n");
}



int my_485_config(void)
{

	struct serial_rs485 rs485conf;

	/* Enable RS485 mode: */
	rs485conf.flags |= SER_RS485_ENABLED;

	/* Set logical level for RTS pin equal to 1 when sending: */
	rs485conf.flags |= SER_RS485_RTS_ON_SEND;
	/* or, set logical level for RTS pin equal to 0 when sending: */
	//rs485conf.flags &= ~(SER_RS485_RTS_ON_SEND);

	/* Set logical level for RTS pin equal to 1 after sending: */
	rs485conf.flags |= SER_RS485_RTS_AFTER_SEND;
	/* or, set logical level for RTS pin equal to 0 after sending: */
	//rs485conf.flags &= ~(SER_RS485_RTS_AFTER_SEND);

	/* Set rts delay before send, if needed: */
	rs485conf.delay_rts_before_send = 50;

	/* Set rts delay after send, if needed: */
	rs485conf.delay_rts_after_send = 50;

	/* Set this flag if you want to receive data even whilst sending data */
	rs485conf.flags |= SER_RS485_RX_DURING_TX;

	if (ioctl(fd, TIOCSRS485, &rs485conf) < 0) {
        perror("ioctl TIOCSRS485");
		/* Error handling. See errno. */
		return -1;
	}
	return 0;
}

void main(void){
	int nCount = 0;
    int i;
	/* Open your specific device (e.g., /dev/mydevice): */
	fd = open ("/dev/ttymxc4", O_RDWR);
	if (fd < 0) {
        printf("Open ttymxc4 failed\n");
		/* Error handling. See errno. */
	}

	setTermios(B9600);  /*uart set*/

	if (my_485_config() != 0){ /*set 485*/
        perror("config 485");
    }
	/* Use read() and write() syscalls here... */


 	for (i = 0; i < 10; i++)	//数据重复发送
	  {
		 send_data("hello");
         sleep(1);
  	  }

	/* Close the device when finished: */
	if (close (fd) < 0) {
		/* Error handling. See errno. */
	}

}
