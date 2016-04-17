/*****************************************************************************/

/*
 * xmodem
 */

/*************************************************************************************
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *************************************************************************************/
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>/*printf*/
#include <fcntl.h>/*open*/
#include <string.h>/*bzero*/
#include <stdlib.h>/*exit*/
#include <sys/times.h>/*times*/
#include <sys/types.h>/*pid_t*/
#include <termios.h>/*termios,tcgetattr(),tcsetattr()*/
#include <unistd.h>
#include <sys/ioctl.h>/*ioctl*/
#include <sys/wait.h>
#include <string.h>/*bzero*/
/*
Xmodem Frame form: <SOH><blk #><255-blk #><--128 data bytes--><CRC hi><CRC lo>
*/

#define XMODEM_SOH 0x01
#define XMODEM_STX 0x02
#define XMODEM_EOT 0x04
#define XMODEM_ACK 0x06
#define XMODEM_NAK 0x15
#define XMODEM_CAN 0x18
#define XMODEM_CRC_CHR	'C'
#define XMODEM_CRC_SIZE 2			/* Crc_High Byte + Crc_Low Byte */
#define XMODEM_FRAME_ID_SIZE 2 		/* Frame_Id + 255-Frame_Id */
#define XMODEM_DATA_SIZE_SOH 128  	/* for Xmodem protocol */
#define XMODEM_DATA_SIZE_STX 1024 	/* for 1K xmodem protocol */
#define USE_1K_XMODEM 0  			/* 1 for use 1k_xmodem 0 for xmodem */
#define	TIMEOUT_USEC	0
#define	TIMEOUT_SEC(buflen,baud)	(buflen*20/baud+2)/*接收超时*/
#define	TIMEOUT_USEC	0
/*
// 是否使用1K-xModem协议
#if (USE_1K_XMODEM)
	#define XMODEM_DATA_SIZE 	XMODEM_DATA_SIZE_STX
	#define XMODEM_HEAD			XMODEM_STX
#else
	#define XMODEM_DATA_SIZE 	XMODEM_DATA_SIZE_SOH
	#define XMODEM_HEAD 		XMODEM_SOH
#endif
*/

#define XMODEM_DATA_SIZE 128
#define XMODEM_HEAD         XMODEM_SOH


#define SERIAL_DEVICE "/dev/ttyUSB0" //串口1
#define MYBAUDRATE B9600

#define RECV_END  0x11 //自定义文件结束符

/*
 * This function calculates the CRC used by the "Modem Protocol".
 * The first argument is a pointer to the message block. The second argument is the number of bytes in
 * the message block. The message block used by the Modem Protocol contains 128 bytes.
 * The function return value is an integer which contains the CRC.
 */
void delay(int x)
{
    int y;
	for(;x>0;x--)
	 for(y=10;y>0;y--);
}
unsigned short GetCrc16 ( char *ptr, unsigned short count )
{
	unsigned short crc, i;

	crc = 0;
	while(count--)
	{
		crc = crc ^ (int) *ptr++ << 8;//从packet_data中取一个字节数据，强转为16为int，再把低八位移到高八位，赋值给crc
	
		for(i = 0; i < 8; i++)
		{
			if(crc & 0x8000)//判断数据的最高位数据是否为1
				crc = crc << 1 ^ 0x1021; //	CRC-ITU
			else			
				crc = crc << 1;
		}
	}

	return (crc & 0xFFFF);
}
/*******************************************
*receivedata
*返回实际读入的字节数
*
********************************************/
int PortRecv(int fdcom,char* data,int datalen,int baudrate)
{
	
	int readlen,fs_sel;
	int readnum=0x00;
	char readtemp=0;
	fd_set fs_read;
	struct timeval tv_timeout;
	FD_ZERO(&fs_read);
	FD_SET(fdcom,&fs_read);
	tv_timeout.tv_sec=TIMEOUT_SEC(datalen,baudrate);
	tv_timeout.tv_usec=TIMEOUT_USEC;
	
	fs_sel=select(fdcom+1,&fs_read,NULL,NULL,NULL);
	if(fs_sel)
	{
		//while(read(fdcom,&readtemp,1));
		readnum=0;
	    while((readtemp!=RECV_END)&&(readnum<=1000))
		{
		    if(read(fdcom,&readtemp,1)>0)
		   { 
		      data[readnum]=readtemp;
		      readnum++;
			  delay(1);
			}
		}
		//printf("readnum is:%d\n",readnum);
		return readnum;
		//readlen=read(fdcom,data,datalen);
		//return(readlen);
	}
	else
	{
		perror("select");
		return(-1);
	}
	
	return(readnum);
}
/*
 * 串口初始化.
 * Baudrate: 115200 8个数据位, 1个停止位，无奇偶校验位
 */
int Initial_SerialPort(void)
{
	int fd;
	struct termios options, oldtio;
    //打开一个串口设备SERIAL_DEVICE，ttyUSB0
	fd = open( SERIAL_DEVICE , 
	                        O_RDWR    |   //O_RDWR：读写标志
							O_NOCTTY  |   //O_NOCTTY：通知linux，本程序不成为打开串口的终端
							O_NDELAY  );  //O_NDELAY:通知linux，此程序不关心DCD信号线所处状态
	if ( fd == -1 )
	{ 
		/* open error! */
		perror("Can't open ttyUSB0!");
		return -1;
	}

	// 恢复串口为阻塞状态，用来等待串口数据的读入
	if(fcntl(fd, F_SETFL, 0) < 0)
	  printf("fcntl failed!\n");
	else
	  ;

	// 保存原串口属性
	if(tcgetattr(fd, &oldtio) != 0) {
	  perror("setup serial 1");
	  return -1;
	}

	/* Get the current options for the port... */
	tcgetattr(fd, &options);

	/* Set the baud rates to BAUDRATE... */
	cfsetispeed(&options, MYBAUDRATE);
	cfsetospeed(&options, MYBAUDRATE);
	tcsetattr(fd, TCSANOW, &options);

	if (0 != tcgetattr(fd, &options)) 
	{
		perror("SetupSerial 1");
		return -1;
	} 
	
	/*
	 * 8bit Data,no partity,1 stop bit...
	 */
	options.c_cflag &= ~PARENB;//无奇偶校验
	options.c_cflag &= ~CSTOPB;//停止位，1位
	options.c_cflag &= ~CSIZE; //数据位的位掩码
	options.c_cflag |= CS8;    //数据位，8位

	/* inter-character timer, timeout VTIME * 0.1 */
	options.c_cc[VTIME] = 0;
	/* blocking read until VMIN character arrives */
	options.c_cc[VMIN] = 0;

	tcflush(fd,TCIFLUSH);//处理未接收字符，刷新

	/* Choosing Raw Input */
	options.c_lflag &= ~(        ICANON //非终端模式
								| ECHO  //不本地回显
								| ECHOE //不删除前一显示字符
								| ISIG);//不发送与INTR、QUIT和SUSP相对应的信号SIGINT、SIGQUIT和SIGTSTP到tty设置对应前台进程的所有进程
	options.c_oflag &= ~OPOST; 

	/*
	 * Set the new options for the port...
	 */
	if (tcsetattr(fd, TCSANOW, &options) != 0) //激活新配置
	{
 		perror("SetupSerial error");
 		return -1 ;
	}

	return fd ; //打开并初始化好串口1后，返回设备号fd
}

/*
 * 等待串口空闲恢复空闲状态
 */
void ClearReceiveBuffer(int fd)
{
	unsigned char tmp;
	while ((read(fd,&tmp,1)) > 0);
	
	return;
}
