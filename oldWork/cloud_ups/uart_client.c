#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include "uart_client.h"

#define MSG_LEN   1000
static char end=0xd;
static char* usedip;
static char setip[100];
static char cmd_buf[MSG_LEN];
static char writemsg[1000];
static char cmd_data[1024];
static char readmsg;

struct UART_MSG{
    char cmd;
    char msg[MSG_LEN];
};

void print_usage(){
    printf(UART_HELP_MSG);
}
int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
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

int recv_data(int fd1, char data)
{
    if( data != 0xd )
    {
       cmd_data[strlen(cmd_data)] = data;
       cmd_data[strlen(cmd_data)+1] = '\0';
       return 0;
    }else
        return 1;
}
int process_recv(int fd1){
  printf("select start\n"); 
    int ret, nread;
    fd_set fds;
    struct timeval tv;
    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(fd1, &fds);

        //timeout setting
        tv.tv_sec =  10;
        tv.tv_usec = 0;

        ret = select(fd1 + 1, &fds, NULL, NULL, &tv);
        if(ret < 0){
            perror("select");
            break;
        }else if(ret == 0){
            printf("timeout\n");
            continue;
        }

        if(FD_ISSET(fd1, &fds)){
            readmsg=0;
            nread = read(fd1, &readmsg, sizeof(readmsg));
            if(nread > 0){
                printf("the enter is received \n");
                printf("%c\n", readmsg);
                if(recv_data(fd1, readmsg)){
                    printf("select recv: %c\n", readmsg);
                    
                    printf("select start\n"); 
                    return 1;
                }
                    //recived the cmd
            }
        }
    }
    return ret;
}

void send_data(int fd1, char* writemsg)
{
    printf("send_data is :%s\n", writemsg);
	//strcpy(writemsg, "192.168.0.190:12345");
   int i; 
    for(i = 0; i < strlen(writemsg); i++)
   {
	 write(fd1, &writemsg[i], 1);
     usleep(150000);
     //printf("write is : %c \n", writemsg[i]);
   }
   //write(fd1, &writemsg[strlen(writemsg)], 1); 
   write(fd1, &end, 1);

   printf("write done\n");
}

int read_data(int fd1, char* r_buf)
{
    int i = 0;
    printf("%s start\n", __func__);
    while(1)
    {   
        if(r_buf[i] == 0xd){
            r_buf[i] = '\0';
            break;
        }
        else
        {
            if(read(fd1, &r_buf[i], 1 ) > 0)
            {
                if(r_buf[i] == 0xd)
                {   r_buf[i] = '\0';
                    break;
                }
                i++;
            }
                //perror("read");
        }
    }
return 1;
}

int main(int argc, char* argv[])
{
    int fd1,nset,nread,ret;
    size_t len = 0, retf = 0;
    int i = 0;
    int cmd_index;
    char r_buf[30];

    fd1 = open( argv[1], O_RDWR|O_NOCTTY|O_NDELAY);
    if (fd1 < 0){
	    perror("open uart");
	    exit(1);
    }
    printf("open  %s success!!\n", argv[1]);

    nset = set_opt(fd1, 115200, 8, 'N', 1);
    if (nset == -1)
	    exit(1);
    printf("SET  SAC0 success!!\n");
    printf("enter the loop!!\n");

    send_data(fd1, "login");
    printf("start read data\n");
    //memset(r_buf, 0, sizeof(r_buf));
    read_data(fd1, r_buf);
    printf("first r_bur is:%s\n", r_buf);
    //if(process_recv(fd1)){
    if(strcmp(r_buf, "OK") != 0){
        printf("login error\n");
        close(fd1);
        return -1;
    }else
    {
        memset(r_buf, '\0', sizeof(r_buf));
        read_data(fd1, r_buf);
        //login ok
        printf("login OK\n");
        printf("currnet ip is:%s\n", r_buf);
        while(1){
            //print help
            print_usage();
            scanf("%d", &cmd_index);
            switch(cmd_index){
                case 0:
                    break;
                case 1:
                    break;
                case 2:
                    break;
                case 3:
                    break;
                case 4:
                    break;
                case 5:
                    break;
                default:
                    break;
            }
        }
    }

    close(fd1);

 return 0;
}

