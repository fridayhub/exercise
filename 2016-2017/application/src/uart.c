#include "include.h"
struct serial_rs485_my {
    __u32	flags;			/* RS485 feature flags */
#define SER_RS485_ENABLED		(1 << 0)
#define SER_RS485_RTS_ON_SEND		(1 << 1)
#define SER_RS485_RTS_AFTER_SEND	(1 << 2)
    __u32	delay_rts_before_send;	/* Milliseconds */
    __u32	delay_rts_after_send;	/* Delay after send (milliseconds) */
    __u32	padding[6];		/* Memory is cheap, new structs
                               are a royal PITA .. */
}rs485conf;

#define TIOCGRS485      0x542E
#define TIOCSRS485      0x542F





int open232Port(char *rs232port, int baudrate)
{
    int nset = 0;
	int fd1;
    //char *tty = "/dev/ttySU0";
    //char *tty = "/dev/ttyUSB0";

    fd1 = open(rs232port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd1 < 0)
	{
        perror("open uart");
        //exit(1);
        return fd1;
    }
    printf("open  %s success!!\n", rs232port);

    nset = set_opt(fd1, baudrate, 8, 'N', 1);

    if (nset == -1)
		printf("SET %s Failes!!\n", rs232port);
    else
    	printf("SET %s success!!\n", rs232port);	

    return fd1;

}

void close232Port(int fd1)
{
	if (fd1 <0)
		return;
    close(fd1);
}

int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop)
{
	 struct termios newtio,oldtio;
	 if  ( tcgetattr( fd,&oldtio)  !=  0) {
	  perror("SetupSerial 1");
	  return -1;
	 }
	 bzero( &newtio, sizeof( newtio ) );
	 newtio.c_cflag  |=  CLOCAL | CREAD; //CLOCAL:忽略modem控制线  CREAD：打开接受者
	 newtio.c_cflag &= ~CSIZE; //字符长度掩码。取值为：CS5，CS6，CS7或CS8

	 switch( nBits )
	 {
	 case 7:
	  newtio.c_cflag |= CS7;
	  break;
	 case 8:
	  newtio.c_cflag |= CS8;
	  break;
	 }

	 switch( nEvent )
	 {
	 case 'O':
	  newtio.c_cflag |= PARENB; //允许输出产生奇偶信息以及输入到奇偶校验
	  newtio.c_cflag |= PARODD;  //输入和输出是奇及校验
	  newtio.c_iflag |= (INPCK | ISTRIP); // INPACK:启用输入奇偶检测；ISTRIP：去掉第八位
	  break;
	 case 'E':
	  newtio.c_iflag |= (INPCK | ISTRIP);
	  newtio.c_cflag |= PARENB;
	  newtio.c_cflag &= ~PARODD;
	  break;
	 case 'N': 
	  newtio.c_cflag &= ~PARENB;
	  break;
	 }

	switch( nSpeed )
	{
		case 2400:
			cfsetispeed(&newtio, B2400);
			cfsetospeed(&newtio, B2400);
			break;
		case 4800:
			cfsetispeed(&newtio, B4800);
			cfsetospeed(&newtio, B4800);
			break;
		case 9600:
			cfsetispeed(&newtio, B9600);
			cfsetospeed(&newtio, B9600);
			break;
		case 19200:
			cfsetispeed(&newtio, B19200);
			cfsetospeed(&newtio, B19200);
			break;
		case 38400:
			cfsetispeed(&newtio, B38400);
			cfsetospeed(&newtio, B38400);
			break;
		case 57600:
			cfsetispeed(&newtio, B57600);
			cfsetospeed(&newtio, B57600);
			break;		
		case 115200:
			cfsetispeed(&newtio, B115200);
			cfsetospeed(&newtio, B115200);
			break;
		case 460800:
			cfsetispeed(&newtio, B460800);
			cfsetospeed(&newtio, B460800);
			break;
		default:
			cfsetispeed(&newtio, B9600);
			cfsetospeed(&newtio, B9600);
			break;
	 }

	 if( nStop == 1 )
	  newtio.c_cflag &=  ~CSTOPB; //CSTOPB:设置两个停止位，而不是一个
	 else if ( nStop == 2 )
	 newtio.c_cflag |=  CSTOPB;
	 
	 newtio.c_cc[VTIME]  = 0; //VTIME:非cannoical模式读时的延时，以十分之一秒位单位
	 newtio.c_cc[VMIN] = 0; //VMIN:非canonical模式读到最小字符数
	 tcflush(fd,TCIFLUSH); // 改变在所有写入 fd 引用的对象的输出都被传输后生效，所有已接受但未读入的输入都在改变发生前丢弃。
	 if((tcsetattr(fd,TCSANOW,&newtio))!=0) //TCSANOW:改变立即发生
	 {
	  perror("com set error");
	  return -1;
	 }
	 printf("set done!\n\r");
	 return 0;
}

void send_data(int fd1, unsigned char* writemsg, int len)
{
    int i = 0;
    printf("\nsend data:\n");
    for(i = 0; i < len; i++)
    {
        printf("%x ", writemsg[i]);
    }
    write(fd1, writemsg, len);
    printf("\nwrite done\n");
}


/*****************************************************************************/
/******************             485部分					****************/
/*****************************************************************************/

int Set_485RX_Enable(int fd)
{

	/* Enable RS485 mode: */
	rs485conf.flags |= SER_RS485_ENABLED;

	/* Set logical level for RTS pin equal to 1 when sending: */
	//rs485conf.flags |= SER_RS485_RTS_ON_SEND;
	/* or, set logical level for RTS pin equal to 0 when sending: */
	rs485conf.flags &= ~(SER_RS485_RTS_ON_SEND);

	/* Set logical level for RTS pin equal to 1 after sending: */
	//rs485conf.flags |= SER_RS485_RTS_AFTER_SEND;
	/* or, set logical level for RTS pin equal to 0 after sending: */
	rs485conf.flags &= ~(SER_RS485_RTS_AFTER_SEND);

	/* Set rts delay before send, if needed: */
	rs485conf.delay_rts_before_send = 0;

	/* Set rts delay after send, if needed: */
	rs485conf.delay_rts_after_send = 0;

	/* Set this flag if you want to receive data even whilst sending data */
	rs485conf.flags |= SER_RS485_RX_DURING_TX;

	if (ioctl(fd, TIOCSRS485, &rs485conf) < 0) 
	{
        perror("ioctl TIOCSRS485");
		/* Error handling. See errno. */
		return -1;
	}
	return 0;
}

int Set_485TX_Enable(int fd)  //set to hig
{

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
	rs485conf.delay_rts_before_send = 0;

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

int rs485Config(int fd)
{
	if (fd <0)
		return 1;

    memset(&rs485conf,0x0,sizeof(rs485conf));

    rs485conf.flags |= SER_RS485_ENABLED;
    rs485conf.delay_rts_before_send = 0;
    rs485conf.delay_rts_after_send = 50;


    printf("rs485conf.delay_rts_before_send=%d\n", rs485conf.delay_rts_before_send);
    printf("rs485conf.delay_rts_after_send=%d\n", rs485conf.delay_rts_after_send);

    if (ioctl (fd, TIOCSRS485, &rs485conf) < 0) 
    {
      //  close(fd);
        printf("ttySU1 no support  485");
        return 1;
    }else
        printf("485 set ok\n");
	
    return 0;
}

int open485Port(char *rs485port, int baudrate)
{

    int nset = 0;
	int fd1;
    
    fd1 = open(rs485port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd1 < 0){
        perror("open uart");
        return fd1;
    }
    printf("open  %s success!!\n", rs485port);

    nset = set_opt(fd1, baudrate, 8, 'N', 1);
    if (nset == -1)
        printf("SET %s Failes!!\n", rs485port);
    else
	printf("SET %s success!!\n", rs485port);


    if((rs485Config(fd1))!=0){
       return fd1;
    }
	Set_485RX_Enable(fd1);
    return fd1;

}

void close485Port(int fd1)
{
	if (fd1 < 0)
		return;

    rs485conf.flags &= ~ SER_RS485_ENABLED;
    ioctl(fd1,TIOCSRS485, &rs485conf);

    close(fd1);
}
