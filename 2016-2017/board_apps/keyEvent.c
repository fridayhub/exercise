#include <stdio.h>  
#include <linux/input.h>  
#include <stdlib.h>  
#include <sys/types.h>  
#include <sys/stat.h>  
#include <fcntl.h>  
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <string.h>
#include <regex.h>

#define DEV_PATH "/dev/input/event1"   //difference is possible  

#define RxBufferSize 1024
char RxBuffer[RxBufferSize];

int num = 0;
int fd1;

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

int dataRecv(int fd1)
{
    printf("\nselect start\n"); 
    int ret, nread;
    fd_set fds;
    int count = 0;
    struct timeval tv;
    static unsigned char readmsg;
    int i;

    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(fd1, &fds);

        //timeout setting
        tv.tv_sec =  3;
        tv.tv_usec = 0;

        ret = select(fd1 + 1, &fds, NULL, NULL, &tv);
        if(ret < 0){
            perror("select");
            break;
        }else if(ret == 0){
            printf("timeout\n");
            return 0;
            //continue;
        }

        if(FD_ISSET(fd1, &fds)){
            readmsg=0;
            nread = read(fd1, &readmsg, 1);
            RxBuffer[count++] = readmsg;
            if(count >= 13)
            {
                return count;
            }
        }
    }
    return ret;
}

void changeToMac(char *mac)
{
    int i = 0, j = 0;
    for(i = 0, j = 0; i < 17; i++){
        if(i == 2 || i == 5 || i == 8 || i == 11 || i == 14){
            mac[i] = ':';
        }else
        {
            mac[i] = RxBuffer[j];
            j++; 
        }
    } 
}

int writeTofile(char *mac)
{
    int fdm;
    fdm = open("/etc/mac", O_RDWR|O_CREAT);
    if(fdm < 0){
        printf("Can't open mac file to save mac address\n");
        return 0;
    }
    write(fdm, mac, strlen(mac));
}

int ereg(char *pattern, char *value)
{
    int r,cflags=0;  
    regmatch_t pm[10];  
    const size_t nmatch = 10;  
    regex_t reg;  

    r=regcomp(&reg, pattern, cflags);  
    if(r==0){  
        r=regexec(&reg, value, nmatch, pm, cflags);  
    }  
    regfree(&reg);  

    return r;  
}

int isValidMac(char *value)
{
    int r; //r=0:valid, else not valid  
    char *reg="^[0-9A-F]\\([0-9A-F]\\:[0-9A-F]\\)\\{5\\}[0-9A-F]$";  
    r=ereg(reg,value);  
    return r;  
}

int getMacFromBarCode(void)
{
    int nset;
    int ret = 0;
    int i = 0, recvDataLen;
    int cmd_index;
    char *tty = "/dev/ttymxc3";
    char mac[20] = "";

    fd1 = open(tty, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd1 < 0){
        perror("open uart");
        exit(1);
    }
    printf("open  %s success!!\n", tty);

    nset = set_opt(fd1, 9600, 8, 'N', 1);
    if (nset == -1)
        exit(1);
    printf("SET %s success!!\n", tty);

    memset(RxBuffer, 0, 1024);
    recvDataLen = dataRecv(fd1);
    for( i = 0; i < recvDataLen; i++){
        printf("%x ", RxBuffer[i]);
    }
    printf("\n");
    tcflush(fd1, TCIFLUSH); //刷清输入队列 缓冲区
    close(fd1);
    changeToMac(mac); //加：号
    ret = isValidMac(mac); //判断mac地址合法性
    if(ret == 0){
        printf("mac is valid\n");
        writeTofile(mac); //写入文件
    }else{
        printf("invalid mac\n");
        return 0; 
    }
    return 1;
}

int getKeyEvent(int keys_fd)
{
    int ret;
    struct input_event t;  
    struct timeval tv;
    fd_set fds;
    while(1)  
    {  
        FD_ZERO(&fds);
        FD_SET(keys_fd, &fds);

        tv.tv_sec = 0;
        tv.tv_usec = 50000;
        
        ret = select(keys_fd + 1, &fds, NULL, NULL, &tv);
        if(ret < 0){
            perror("select");
            continue;
        }else if(ret == 0){
            //printf("key read time out\n");
            return 0;
            //continue;
        }
        if(FD_ISSET(keys_fd, &fds)){
            if(read(keys_fd, &t, sizeof(t)) == sizeof(t))  
            {  
                if(t.type == EV_KEY && t.value == 1 && t.code == 103) 
                { 
                    printf("key %d %s %d\n", t.code, (t.value) ? "Pressed" : "Released", t.value);  
                    return 1;
                }else if(t.type == EV_KEY && t.value == 2 && t.code == 103){
                    //printf("key %d %s %d\n", t.code, (t.value) ? "Pressed" : "Released", t.value);  
                    num++;
                    if(num > 16)
                        return 2;
                } 
            }  
        }
        t.value = 1;
    }
    return 0;
}

int main()  
{  
    int keys_fd, ret;  
    keys_fd = open(DEV_PATH, O_RDONLY | O_NOCTTY | O_NDELAY);  
    if(keys_fd <= 0)  
    {  
        printf("open /dev/input/event1 device error!\n");  
        return -1;  
    }  

    while(1){ 
        ret = getKeyEvent(keys_fd);
        if(ret == 2)
        {
            system("cp -f /data/Tanda.conf.default /data/Tanda.conf");
            printf("Set to default\n\nReboot Now\n");
            system("reboot");
            num = 0;
        }else if(ret == 1)
        {
            printf("Set mac ready\n");
            system("killall demo.sh");
            system("killall SteamSensor");
            ret = getMacFromBarCode(); //read mac address from BarCode
            if(ret){
                printf("led start\n");
                num = 0;
                while(1){
                    system("echo 1 > /sys/class/leds/led1/brightness");
                    usleep(40000);
                    system("echo 0 > /sys/class/leds/led1/brightness");
                    usleep(40000);
                }
            }
        }
    } 

    close(keys_fd);  
    return 0;  
}  
