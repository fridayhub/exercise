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
#include <poll.h>
#include <stropts.h>
#include <linux/serial.h>

#define DEV_PATH "/dev/input/event1"   //difference is possible  

#define RxBufferSize 1024
char RxBuffer[RxBufferSize];

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

typedef struct packet_begin {
	unsigned short beginId;         // 启动符，固定为"@@" 64,64
	unsigned short businessNo;      // 业务流水号
	unsigned short protocolNo;      // 协议版本号
	unsigned char timeTag[6];       // 时间标签,预留
	unsigned char srcAddr[6];       // 源地址
	unsigned char destAddr[6];      // 目的地址
	unsigned short dataLen;         // 应用数据长度
	unsigned char cmd;              // 命令
} packet_begin_t;

typedef struct packet_data {
	unsigned char type;             // 类型标志
	unsigned char num;              // 信息对象数目
} packet_data_t;

typedef struct packet_end {
	unsigned char crc;              // 校验和
	unsigned short endId;           // 结束符,固定为"##" 35,35
} packet_end_t;

static int moduleFlage = 0, flag = 0;
static int ttyfd;
static struct termios oldtio;

int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop)
{
    struct termios newtio;
    
    if (tcgetattr(fd, &oldtio)  !=  0) {
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
    if (tcsetattr(fd, TCSANOW, &newtio) != 0) {    //TCSANOW:改变立即发生
        perror("com set error");
        return -1;
    }
    printf("set done!\n");
    return 0;
}

ssize_t send_data(int fd, const char * writemsg)
{
    return write(fd, writemsg, strlen(writemsg));
}

int init_tty(int *fd, const char *tty)
{
    if ((*fd = open(tty, O_RDWR | O_NOCTTY | O_NDELAY)) < 0) {
        perror("open uart");
        return -1;
    }

    if (set_opt(*fd, 115200, 8, 'N', 1) < 0)
        return -1;
    
    return 0;
}

int test_rs232(void)
{
    if (init_tty(&ttyfd, "/dev/ttymxc3") != 0)
        return -1;

    if (send_data(ttyfd, "\r\n") <= 0)
        return -1;

    return 0;
}

int my_485_config_rx(int fd)
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

	if (ioctl(fd, TIOCSRS485, &rs485conf) < 0) {
        perror("ioctl TIOCSRS485");
		/* Error handling. See errno. */
		return -1;
	}
	return 0;
}

int test_rs485(void)
{
    int fd;
    int ret;
    fd_set rset;
    struct timeval tv;
    packet_begin_t begin;
    packet_data_t data;
    packet_end_t end;
    //char buf[128] = {0};
    unsigned char *p = NULL;
    unsigned int crc = 0;

    if (init_tty(&fd, "/dev/ttymxc1") != 0)
        return -1;
    
    if (my_485_config_rx(fd) != 0)
        perror("config 485 to read !\n");

    tv.tv_sec = 10;
    tv.tv_usec = 0;
    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    if ((ret = select(fd + 1, &rset, NULL, NULL, &tv)) < 0) {
        perror("select");
        return -1;
    }
    if (ret == 0) {
        printf("select of rs485 time out !\n");
        return -1;
    }

    bzero(&begin, sizeof(begin));
    bzero(&data, sizeof(data));
    bzero(&end, sizeof(end));
#if 0
    if (read(fd, buf, 128) <= 0) {
        printf("RS485: error!\n");
        return -1;                
    }
    printf("RS485: %s\n", buf);


#else
    if (read(fd, &begin, sizeof(begin)) != sizeof(begin)) {
        printf("read rs485 error!\n");
        return -1;                
    }
    if (read(fd, &data, sizeof(data)) != sizeof(data)) {
        printf("read rs485 error!\n");
        return -1;                
    }
    if (read(fd, &end, sizeof(end)) != sizeof(end)) {
        printf("read rs485 error!\n");
        return -1;                
    }

    if (begin.beginId != 0x4040 || end.endId != 0x2323)
        return -1;

    for (p = (unsigned char *)&begin.businessNo; p <= (unsigned char *)&begin.cmd; p++)
        crc += *p;
    crc += data.type;
    crc += data.num;
    if (end.crc != (unsigned char)crc)
        return -1;

    if (data.type != 31 || data.num != 0x01)
        return -1;
#endif

    if((tcsetattr(fd, TCSADRAIN, &oldtio)) != 0)
        perror("com set error");
    close(fd);

    return 0;
}

int dataRecv(int fd1)
{
    int ret;
    fd_set fds;
    int count = 0;
    struct timeval tv;
    unsigned char readmsg;

    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(fd1, &fds);

        //timeout setting
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        ret = select(fd1 + 1, &fds, NULL, NULL, &tv);
        if(ret < 0){
            perror("select");
            break;
        }else if(ret == 0){
            //printf("timeout\n");
            break;
        }

        if(FD_ISSET(fd1, &fds)){
            readmsg=0;
            if (read(fd1, &readmsg, 1) > 0) {
                RxBuffer[count++] = readmsg;
            }
        }
        count &= 0x3ff;
    }
    return count;
}

void judgementModule(void)
{
     FILE *fp = NULL;
    char buf[1024];
    char idBuf[20][1024];
    int i = 0;
    if(NULL == (fp = popen("lsusb | awk '{print $6}'", "r"))){
        printf("command can't execute\n");
    }
    memset(buf, 0, sizeof(buf));
    memset(idBuf, 0, sizeof(idBuf));
    while(NULL != fgets(buf, sizeof(buf), fp)){
        strcat(idBuf[i++], buf);
    }

    while(i >= 0){
        //printf("%s", idBuf[i]);
        i--;
        if(strncmp(idBuf[i], "12d1:1c25", 9) == 0){ //HUAWEI MU709s-2
            //printf("find HUAWEI MU709s-2!\n");
            moduleFlage = 1;
            break;
        }else if(strncmp(idBuf[i], "19d2:1476", 9) == 0){ //ZTE ME3620
            //printf("find ZTE ME3620!\n");
            moduleFlage = 2;
            break;
        }else if(strncmp(idBuf[i], "19d2:ffeb", 9) == 0){ //ZTE MW3650
            //printf("find ZTE MW3650!\n");
            moduleFlage = 3;
            break;
        }else if(0 == i){
            printf("\e[1;31mNo module insert!\n Test 3G failed\e[0m\n !");
            send_data(ttyfd, "No module insert!\r\n Test 3G failed\r\n !");
        }
    }
    pclose(fp);
}

int checkConnect(void)
{
    FILE *stream;
    char recvBuf[16] = {0};
    char cmdBuf[256] = {0};
    char *dst = "192.168.2.10";
    int cnt = 4;

    //sprintf(cmdBuf, "ping %s -c %d -i 0.2 | grep time= | wc -l", dst, cnt);
    sprintf(cmdBuf, "/etc/ping %s -c %d  -i 0.2| grep time= | wc -l", dst, cnt);
    stream = popen(cmdBuf, "r");
    fread(recvBuf, sizeof(char), sizeof(recvBuf) - 1, stream);
    pclose(stream);

    if(atoi(recvBuf) > 0)
        return 0;

    return -1;
}

int getKeyEvent(int keys_fd, int point)
{
    int ret = 0;
    struct input_event t;  

    struct pollfd fds[1];
    fds[0].fd = keys_fd;
    fds[0].events = POLLIN;
    
    while(1)  
    {  
        ret = poll(fds, 1, 10000);

        if(ret < 0) {
            perror("poll");
            continue;
        } else if(ret == 0) {
            if(point == 1){
                printf("\e[1;31mButton FAILED !\e[0m\n");
                send_data(ttyfd, "Button FAILED !\r\n");
            }
            else if(point == 2){
                printf("\e[1;31mCheck 1 FAILED !\e[0m\n");
                send_data(ttyfd, "Check 1 FAILED !\r\n");
            }
            else if(point == 3){
                printf("\e[1;31mCheck 2 FAILED !\e[0m\n");
                send_data(ttyfd, "Check 2 FAILED !\r\n");
            }
            return -1;
        }

        if ((fds[0].revents & POLLIN) == POLLIN) {
            if(read(keys_fd, &t, sizeof(t)) == sizeof(t))  
            {  
                if(t.type == EV_KEY && t.code == 103 && point == 1) 
                { 
                    printf("\e[1;32mButton OK !\e[0m\n");
                    send_data(ttyfd, "Button OK !\r\n");
                    return 0;
                }else if(t.type == EV_KEY && t.code == 107 && point == 2){
                    printf("\e[1;32mCheck 1 OK !\e[0m\n"); 
                    send_data(ttyfd, "Check 1 OK !\r\n");
                    return 0;
                }else if(t.type == EV_KEY && t.code == 108 && point == 3){
                    printf("\e[1;32mCheck 2 OK !\e[0m\n");
                    send_data(ttyfd, "Check 2 OK !\r\n");
                    return 0;
                }
            }  
        }
    }
}

void getModuleStatus(char *tty){
    int nset, i, fd1;
    int recvDataLen = 0;

    fd1 = open(tty, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd1 < 0){
        perror("open uart");
        exit(1);
    }
//    printf("open  %s success!!\n", tty);

    nset = set_opt(fd1, 115200, 8, 'N', 1);
    if (nset == -1)
        exit(1);
 //   printf("SET %s success!!\n", tty);

    memset(RxBuffer, 0, 1024);
    send_data(fd1, "AT\r\n");
    recvDataLen = dataRecv(fd1);
    for( i = 1; i < recvDataLen - 1; i++){
       // printf("%c", RxBuffer[i]);
        if((RxBuffer[i-1] == 'O') && (RxBuffer[i] == 'K'))
            flag = 1;
    }
    close(fd1);
}

void test3G(void){
    judgementModule();
    if(1 == moduleFlage){ //HUAWEI MU709s-2
        getModuleStatus("/dev/ttyUSB2");
    }else if(2 == moduleFlage){
        getModuleStatus("/dev/ttyUSB1"); //ZTE ME3620 
    }else if(3 == moduleFlage){ //ZTE MW3650 
        getModuleStatus("/dev/ttyUSB0");
    }
}

int test_sdcard(void)
{
    FILE *fp = NULL;
    char buf[256] = {0};
    int fd = 0;
    int flag = -1;

    if ((fp = popen("df -lh | grep /media/mmcblk0p1", "r")) == NULL) {
        perror("popen");
        return -1;
    }

    while (fgets(buf, 200, fp) != NULL) {
        if (strncmp(buf, "/dev/mmcblk0p1", 14) == 0) {
            if ((fd = open("/media/mmcblk0p1/test", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) > 0) {    
                flag = 0;
                close(fd);
                system("rm -f /media/mmcblk0p1/test");
            }
            break;
        }
    }
    pclose(fp);

    return flag;
}

int test_led(void)
{
    int n = 50;
    while(n--) {
        system("echo 1 > /sys/class/leds/led1/brightness");
        usleep(40000);
        system("echo 0 > /sys/class/leds/led1/brightness");
        usleep(40000);
    }

    return 0;
}

int main()  
{
    printf("Start test !\n");
    //system("killall keyEvent");
    int keys_fd, ret;  

    /* Test RS232 */
    if (test_rs232() != 0) {
        printf("RS232 FAILED !");
        return -1;
    }
    send_data(ttyfd, "\r\n\r\n\r\n\r\n\r\n");
    send_data(ttyfd, "****************************************************************\r\n");
    send_data(ttyfd, "* TEST: RS232 Button Check1 Check2 Ethernet LED 3G SDcard RS485*\r\n");
    send_data(ttyfd, "* TIME: 15 ~ 30s                                               *\r\n");
    send_data(ttyfd, "****************************************************************\r\n");
    send_data(ttyfd, "************************* TEST START ***************************\r\n");
    send_data(ttyfd, "\r\nStart test RS232\r\n");
    send_data(ttyfd, "RS232 OK !\r\n");

    /* Test Button and check */
    keys_fd = open(DEV_PATH, O_RDONLY);  
    if(keys_fd <= 0)  
    {  
        printf("open /dev/input/event1 device error!\n");  
        send_data(ttyfd, "open /dev/input/event1 device error!\r\n");
        return -1;  
    }  
    
    printf("\nStart test Button\n");
    send_data(ttyfd, "\r\nStart test Button\r\n");
    getKeyEvent(keys_fd, 1);
    usleep(1000);
    
    printf("\nStart test check 1\n");
    send_data(ttyfd, "\r\nStart test check1\r\n");
    getKeyEvent(keys_fd, 2);
    usleep(1000);

    printf("\nStart test check 2\n");
    send_data(ttyfd, "\r\nStart test check2\r\n");
    getKeyEvent(keys_fd, 3);
    usleep(1000);

    close(keys_fd);  

    /* test ethernet */
    printf("\nStart test ethernet \n");
    send_data(ttyfd, "\r\nStart test ethernet\r\n");
    usleep(1000);
    ret = checkConnect();
    if(ret == 0){
        printf("\e[1;32mEthernet OK !\e[0m\n");
        send_data(ttyfd, "Ethernet OK !\r\n");
    }else{
        printf("\e[1;31mEthernet Failed !\e[0m\n");
        send_data(ttyfd, "Ethernet Failed !\r\n");
    }
    usleep(1000);

    /* test led */
    printf("\nStart test led\n");
    send_data(ttyfd, "\r\nStart test led\r\n");
    usleep(1000);
    test_led();
    usleep(1000);

    /* test 3G */
    printf("\nStart test 3G\n");
    send_data(ttyfd, "\r\nStart test 3G\r\n");
    usleep(1000);
    test3G();
    if(flag){
        printf("\e[1;32m3G OK !\e[0m\n");
        send_data(ttyfd, "3G OK !\r\n");
    } else {
        printf("\e[1;31m3G FAILED !\e[0m\n");
        send_data(ttyfd, "3G FAILED !\r\n");
    }
    usleep(1000);
    
    /* test SDcard */
    printf("\nStart test SDcard\n");
    send_data(ttyfd, "\r\nStart test SDcard\r\n");
    usleep(1000);
    if (test_sdcard() == 0) {
        printf("\e[1;32mSDcard OK !\e[0m\n");
        send_data(ttyfd, "SDcard OK !\r\n");
    } else {
        printf("\e[1;31mSDcard FAILED !\e[0m\n");
        send_data(ttyfd, "SDcard FAILED !\r\n");
    }
    usleep(1000);
    
    /* test RS485 */
    printf("\nStart test RS485\n");
    send_data(ttyfd, "\r\nStart test RS485\r\n");
    usleep(1000);
    if (test_rs485() == 0) {
        printf("\e[1;32mRS485 OK !\e[0m\n");
        send_data(ttyfd, "RS485 OK !\r\n");
    } else {
        printf("\e[1;31mRS485 FAILED !\e[0m\n");
        send_data(ttyfd, "RS485 FAILED !\r\n");
    }
    usleep(1000);

    send_data(ttyfd, "\r\n************************* TEST DONE **************************\r\n");

    if((tcsetattr(ttyfd, TCSADRAIN, &oldtio)) != 0) {
        perror("com set error");
        return -1;
    }
    close(ttyfd);

    return 0;
}
