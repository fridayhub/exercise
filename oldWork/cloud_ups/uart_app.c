#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include "uart_app.h"

#define MSG_LEN   1000

static char usedip[100];
static char delay_alarm_c[10];
static int delay_alarm;
static char cmd_buf[MSG_LEN];
static char writemsg[1000];
static char end = 0xd;
static char endl=0x0A;
static FILE* fp = NULL;
static FILE* fp_delay = NULL;
char w_ip[100] = {"/sbin/ifconfig eth0 "};

struct UART_MSG{
    char cmd;
    char msg[MSG_LEN];
};

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

int send_msg(int fd, char* msg){
	int i = 0, ret = 0;
	for(i = 0; i < strlen(msg); i++){
		ret = write(fd, &msg[i], 1);
		usleep(300000);
	}
	write(fd, &end, 1);
	return i;
}

int recv_msg(int fd, char recv){
	int ret = 0;
	printf("%s start\n", __func__);
	if(recv != 0xd && recv != 0xA){
		cmd_buf[strlen(cmd_buf)] = recv;
		cmd_buf[strlen(cmd_buf) + 1] = '\0';
		return 0;
	}else{
		printf("received 0x%x\n", recv);
		if(strlen(cmd_buf) > 0)
			return 1;
		else
			return 0;
	}
	return -1;
}

void showhelp(int fd){
	int i = 0;
	memset(writemsg, 0, sizeof(writemsg));
	//发送ip和delay time (s)
	sprintf(writemsg, "current set: \n Ip: %s\n Delay alarm: %d (s)", usedip, delay_alarm);
	write(fd, writemsg, strlen(writemsg));
	write(fd, &endl, 1);
	write(fd, &end, 1);

	for(i = 0; i < 3; i++){
		write(fd, help[i], strlen(help[i]));
		write(fd, &endl, 1);
	}
}

void showerror(int fd){
	memset(writemsg, 0, sizeof(writemsg));
	strcpy(writemsg, "error commands : ");
	strcat(writemsg, cmd_buf);
	write(fd, writemsg, strlen(writemsg));
	write(fd, &endl, 1);
}

int process_setdelay(int fd){
	int ret;
	char* ptr = cmd_buf;
        int delay = 0;
	//跳过命令
	ptr += 8;
	//跳过空格
	while(' ' == *ptr)
		ptr++;
        //字符串转换为整数
        delay = atoi(ptr);
	memset(writemsg, 0, sizeof(writemsg));
	if(delay >= 0){
		fseek(fp_delay, 0, SEEK_SET);
                ret = ftruncate(fileno(fp_delay), strlen(ptr));
                fwrite(ptr, sizeof(char), strlen(ptr), fp_delay);
		fflush(fp_delay);
		delay_alarm = delay;
                sprintf(writemsg, "setdelay command OK!!\nThe current delay time is %d s\n",
                         delay);
		write(fd, writemsg, strlen(writemsg)); 
		return 1;
	}else{
		sprintf(writemsg, "the argument(%s) is out of range\n", ptr);
                write(fd, writemsg, strlen(writemsg));
                return 0;
	}
}

int process_setip(int fd){
	int ret;
	char* ptr = cmd_buf;
	char ip[20];
	char set_ip[100];
	//跳过setip命令
	ptr += 5;
	//跳过空格
	while(*ptr == ' ')
		ptr++;
	//判断ip合法性
	if(if_a_string_is_a_valid_ipv4_address(ptr) <= 0){
		//ip 非法
		memset(writemsg, 0, sizeof(writemsg));
		sprintf(writemsg, "the argument(%s) of setip command is wrong\n", ptr);
		write(fd, writemsg, strlen(writemsg));
		return 0;
	}
	
	//写入文件
	fseek(fp, 0, SEEK_SET);
	printf("ptr = %s, len = %zu\n", ptr, strlen(ptr));
    strcat(w_ip, ptr);     //add "/sbin/ifconfig eth0 " to head
	fwrite(w_ip, sizeof(char), strlen(w_ip), fp);
	fflush(fp);
	//write(fileno(fp), ptr, strlen(ptr));	
	ret = ftruncate(fileno(fp), strlen(w_ip));
	perror("ftruncate");

	//设置当前ip
	memset(set_ip, 0, sizeof(set_ip));
	sprintf(set_ip, "ifconfig eth0 %s", ptr);
    printf("sprintf set_ip is: %s\n", set_ip);
	if(system(set_ip) == 0)
    {
        printf("set ip successufl\n");
    }
	strcpy(usedip, ptr);
	
	//回复设置成功
	memset(writemsg, 0, sizeof(writemsg));
	sprintf(writemsg, "setip OK!!!\nThe current ip is:\n%s\n", usedip);
	write(fd, writemsg, strlen(writemsg));

	return 1;
}

int main(int argc, char* argv[])
{
    int fd1,nset,nread,ret;
    char readmsg;
    fd_set fds;
    struct timeval tv;
    size_t len = 0, retf = 0;
    memset(cmd_buf, 0, sizeof(cmd_buf));

    //open and read the usedip file
    fp = fopen("/etc/upsip", "r+");
    if(fp == NULL){
        perror("open ip file");
        return -1;
    };
    fseek(fp, strlen(w_ip), SEEK_SET);
    fgets(usedip, sizeof(usedip), fp);
    printf("the used ip is %s\n", usedip);

    //open and write delay_alarm file
    fp_delay = fopen("/etc/delay_alarm", "r+");
    if(fp_delay == NULL){
        perror("open ip file");
        return -1;
    };
    memset(delay_alarm_c, 0, sizeof(delay_alarm_c));
    fgets(delay_alarm_c, sizeof(delay_alarm_c), fp_delay);
    printf("the delay_alarm_c is %s\n", delay_alarm_c);
    delay_alarm = atoi(delay_alarm_c);
    printf("the delay_alarm is %d\n", delay_alarm);


    
    fd1 = open( argv[1], O_RDWR);
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

    while (1)
    {
	FD_ZERO(&fds);
	FD_SET(fd1, &fds);

	//timeout setting
	tv.tv_sec = 10;
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
		//memset(readmsg, 0, sizeof(readmsg));
		readmsg = 0;
		nread = read(fd1, &readmsg, 1);
		
		printf("received char = %x\n", readmsg);
		if(nread > 0){
			/*
			if(!strcmp(readmsg, "login")){
				//memset(writemsg, 0, sizeof(writemsg));
				//strcpy(writemsg, help);
				//write(fd1, writemsg, strlen(writemsg));
				showhelp(fd1);
			}
			*/
			if(recv_msg(fd1, readmsg) > 0){
				//接受到一个完整的消息
				printf("recve a full msg %s\n", cmd_buf);
				if(!strncmp(cmd_buf, "login", 5)){							
					showhelp(fd1);
					memset(cmd_buf, 0, sizeof(cmd_buf));
				}else if(!strncmp(cmd_buf, "setip", 5)){
				      //如果是setip命令
					printf("recve a setip command %s\n", cmd_buf);
				      if(!process_setip(fd1)){
						showhelp(fd1);
					}
				      memset(cmd_buf, 0, sizeof(cmd_buf));
				}else if(!strncmp(cmd_buf, "setdelay", 8)){
				      printf("receive set delay command %s\n", cmd_buf);
				      if(!process_setdelay(fd1)){
				                showhelp(fd1);	
				      }
				      memset(cmd_buf, 0, sizeof(cmd_buf));
				}else{
					printf("error commands\n");
					showerror(fd1);
					showhelp(fd1);
					memset(cmd_buf, 0, sizeof(cmd_buf));
				}				
			}   
		}
	}
    }
    close(fd1);
    fclose(fp);
    fclose(fp_delay);

 return 0;
}

