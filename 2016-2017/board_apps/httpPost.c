#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

#define RxBufferSize 1024

int fd1, moduleFlage = 0;

unsigned char RxBuffer[RxBufferSize];

struct ModuleInfo{
    unsigned char lac[10];
    unsigned char cell[20];
    unsigned char mnc[10];
    unsigned char mcc[10];
    unsigned char sing[5];
    unsigned char singMode[5];
    char imei[20];
};

//#define IPSTR "61.147.124.120"
#define PORT 80
#define BUFSIZE 1024

//char *twHostName="xf.tandatech.com";

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

    int i = 0;
    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(fd1, &fds);

        //timeout setting
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        ret = select(fd1 + 1, &fds, NULL, NULL, &tv);
        if(ret < 0){
            perror("select");
            break;
        }else if(ret == 0){
            printf("timeout\n");
            return count;
            //continue;
        }

        if(FD_ISSET(fd1, &fds)){
            readmsg=0;
            nread = read(fd1, &readmsg, 1);
            //printf("read buf %x\n", readmsg);
            if(nread > 0){
                //printf("recv char is %x\n", readmsg);
                RxBuffer[count++] = readmsg;
            }
        }
    }
    return ret;
}

void send_data(int fd1, unsigned char* writemsg, int len)
{
    int i = 0;
    //printf("\nsend data:\n");
    //printf("%s\n", writemsg);
    write(fd1, writemsg, len);
    //printf("\nwrite done\n");
}

int setIMEI(char *imei)
{
    FILE *fd;
    char lineBuf[256]; //save one line
    long configLen = 0;
    memset(lineBuf, 0 , 256);

    fd = fopen("/data/some.conf", "r");
    if(fd == NULL)
    {
        printf("Can't open config file to set imei\n");
        return 0;      
    }
    
    fseek(fd, 0, SEEK_END);
    configLen = ftell(fd); //save config file lenth 
    fseek(fd, 0, SEEK_SET);

    int ConfigBufferLen = strlen(imei);
    char sumBuf[ConfigBufferLen + configLen];
    memset(sumBuf, 0, sizeof(ConfigBufferLen + configLen));
    while(fgets(lineBuf, 256, fd) != NULL){
        if(strlen(lineBuf) < 4){ //space line
            strcat(sumBuf, lineBuf);
            continue;
        }

        char *linePos = NULL;
        linePos = strstr(lineBuf, "="); //not match
        if(linePos == NULL){
            strcat(sumBuf, lineBuf);
            continue;
        }
        int lineNum = linePos - lineBuf;
        char lineName[lineNum + 1];
        memset(lineName, 0, sizeof(lineName));
       strncpy(lineName, lineBuf, lineNum); //save string before =
      if(lineName[0] == "#"){
        strcat(sumBuf, lineBuf);
       continue; 
      } 

      if(strcmp("IMEI", lineName) == 0){
        strcat(sumBuf, "IMEI");
        strcat(sumBuf, "=");
        strcat(sumBuf, imei);
        strcat (sumBuf, "\n");
      }else{
        strcat(sumBuf, lineBuf);
      }
    }

    fclose(fd);
    remove("/data/some.conf");
    FILE * f = fopen("/data/some.conf", "w+");
    fputs(sumBuf, f);
    fclose(f);
    system("chmod 777 /data/some.conf");

    return 1;
}

void getZteCellLac(unsigned char *lac, unsigned char *cell, int len){
    int posit[5];
    int i = 0, j = 0;

    for(i = 0, j = 0; i < len; i++){ //record posit of the ,
        if(RxBuffer[i] == ','){
            posit[j] = i;
            j++;
        }
    }

    if(3 == moduleFlage){
        for(i = 0; i < (posit[2] - posit[1] - 2); i++){
            lac[i] = RxBuffer[posit[1] + 2 + i];
        }

        j = 0;
        i = posit[2] + 2;
        while(RxBuffer[i] != '\r'){
            cell[j++] = RxBuffer[i++];
        }
    }else{
        for(i = 0; i < (posit[2] - posit[1] - 3); i++){
            lac[i] = RxBuffer[posit[1] + 2 + i];
        }

        for(i = 0; i < (posit[3] - posit[2] - 3); i++){
            cell[i] = RxBuffer[posit[2] + 2 + i];
        }
    }
}

void getCellLac(unsigned char *lac, unsigned char *cell){
    unsigned char dest[20][20];
    unsigned char *p;
    int i = 0, j = 0;
    int len = 0;
    int posit[4];
    len = strlen(RxBuffer);

    for(i = 0; i < len; i++){
        if(RxBuffer[i] == '"'){
            posit[j] = i;
            j++;
        }
    }
    for(i = 0; i < (posit[1] - posit[0] - 1); i++){
        lac[i] = RxBuffer[posit[0] + i + 1]; 
    }

    for(i = 0; i < (posit[3] - posit[2] - 1); i++){
        cell[i] = RxBuffer[posit[2] + i + 1]; 
    }
    //    p = strchr(RxBuffer, ':'); //找到：号
    //    p = p + 2; //后移两位 到第一个数字
    //    char *token = strtok(p, ",");
    //            while(token != NULL){
    //                strcpy(dest[i++], token);
    //            }
    //            for(j = 0; j < i; j++){
    //                printf("%s\t", dest[j]);
    //            }
    //            strcpy(lac, dest[2]); //lac
    //            strcpy(cell, dest[3]); //cell
    //            printf("\n");
}

void getMncMcc(unsigned char *mcc, unsigned char *mnc){
    int i = 0;
    int j = 0;
    for(i = 11, j = 0; i < 14; i++, j++){
        mcc[j] = RxBuffer[i];
    }

    for(i = 14, j = 0; i < 16; i++, j++){
        mnc[j] = RxBuffer[i];
    }
}

void getZteMncMcc(unsigned char *mcc, unsigned char *mnc){
    int i = 0;
    int j = 0;
    for(i = 10, j = 0; j < 3; i++, j++){
        mcc[j] = RxBuffer[i];
    }

    for(i = 13, j = 0; j < 2; i++, j++){
        mnc[j] = RxBuffer[i];
    }
}

void getZteSingStrength(unsigned char *sing){
    unsigned char tmp_sing[10] = {0};
    int i = 0;
    int posit[4];
    int len = 0;
    int intSing = 0;
    len = strlen(RxBuffer);
    for(i = 0; i < len; i++){
        if(RxBuffer[i] == ','){
            posit[0] = i;
        }else if(RxBuffer[i] == ':'){
           posit[1] = i; 
        }
    }

    for(i = 0; i < (posit[0] - posit[1] - 2); i++){
        tmp_sing[i] = RxBuffer[posit[1] + 2 + i];
    }

    intSing = atoi(tmp_sing);
    printf("intSing:%d  str:%s\n", intSing, tmp_sing);
    if(intSing  >= 23){
        strcpy(sing, "5");
    }else if( 13 <= intSing < 23){
        strcpy(sing, "4");
    }else if( 10 <= intSing < 13){
        strcpy(sing, "3");
    }else if( 5 <= intSing < 10){
        strcpy(sing, "2");
    }else if( 1 <= intSing < 5){
        strcpy(sing, "1");
    }else
        strcpy(sing, "0");
}

void getSingStrength(unsigned char *sing, unsigned char *singMode){
    int len = 0;
    int i = 0, j = 0, k = 0;
    int posit[10];
    int modPosit[10];
    int intSing = 0;
    unsigned char tmp_sing[10] = {0}, tmp_singMode[15] = {0};
    len = strlen(RxBuffer);
    for(i = 0, j = 0, k = 0; i < len; i++){
        if(RxBuffer[i] == ','){
            posit[j] = i;  //save , position
            j++;
        }
        if(RxBuffer[i] == '"'){
            modPosit[k] = i;
            k++;
        }
    }

    for(i = 0; i < (posit[1] - posit[0] -1); i++){
        tmp_sing[i] = RxBuffer[posit[0] + 1 + i];   //save sing numbe
    }

    for(i = 0; i < (modPosit[1] - modPosit[0] - 1); i++){
        tmp_singMode[i] = RxBuffer[modPosit[0] + 1 + i]; //save sing mode
    }
    //printf("sing strength is : %s\n", tmp_sing);
    //printf("sing mode is : %s\n", tmp_singMode);
    if(strcmp(tmp_singMode, "WCDMA") == 0){
        strcpy(singMode, "6"); //联通3G
    }else if(strcmp(tmp_singMode, "TD-LTE") == 0 || strcmp(tmp_singMode, "FDD-LTE") == 0){
        strcpy(singMode, "3");   //联通4G
    }else if(strcmp(tmp_singMode, "TD-CDMA") == 0){
        strcpy(singMode, "7");  //移动3G
    }else if(strcmp(tmp_singMode, "GSM") == 0){
        strcpy(singMode, "9");  //2G网络
    }

    intSing = atoi(tmp_sing);
    printf("intger sing is : %d\n", intSing);
    if( (120 - intSing)  < 85){  //信号强度转换
        strcpy(sing, "5");
    }else if( 86 <= (120 - intSing) < 90){
        strcpy(sing, "4");
    }else if( 90 <= (120 - intSing) < 95){
        strcpy(sing, "3");
    }else if( 95 <= (120 - intSing) < 100){
        strcpy(sing, "2");
    }else if( 100 <= (120 - intSing) < 105){
        strcpy(sing, "1");
    }else if((120 - intSing) > 105){
        strcpy(sing, "0");
    }

}

int getLocation(struct ModuleInfo *MInfo, char *tty)
{
    int nset, IccidFd;
    size_t len = 0, retf = 0;
    int i = 0, recvDataLen = 0, j = 0;
    //char *tty = "/dev/ttyUSB2";
    int length = 0;
    unsigned short serialTemp = 0;
    char *setCommand = "AT+CGREG=2\r\n";
    char *getLacCellCommand = "AT+CGREG?\r\n";
    char *getMncMccCommand = "AT+CIMI\r\n";
    int flag = 0, k = 0;
    char ICCID[22];

    fd1 = open(tty, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd1 < 0){
        perror("open uart");
        exit(1);
    }
    printf("open  %s success!!\n", tty);

    nset = set_opt(fd1, 115200, 8, 'N', 1);
    if (nset == -1)
        exit(1);
    printf("SET %s success!!\n", tty);

    memset(RxBuffer, 0, 1024);
    send_data(fd1, "AT\r\n", strlen("AT\r\n"));
    recvDataLen = dataRecv(fd1);
    for( i = 0; i < recvDataLen; i++){
        if((RxBuffer[i] == 'O') && (RxBuffer[++i] == 'K'))
            flag = 1;
        printf("%c", RxBuffer[i]);
    }

    if(flag == 1){
        memset(RxBuffer, 0, 1024);
        flag = 0;
        printf("AT command is OK!\n");
        send_data(fd1, "AT+CGREG=2\r\n", strlen("AT+CGREG=2\r\n"));
        recvDataLen = dataRecv(fd1);
        for( i = 0; i < recvDataLen; i++){
            if((RxBuffer[i] == 'O') && (RxBuffer[++i] == 'K'))
                flag = 1;
            printf("%c", RxBuffer[i]);
        }
        if(flag == 1){
            memset(RxBuffer, 0, 1024);
            printf("set CGREG to 2 is OK!\n");
            send_data(fd1, "AT+CGREG?\r\n", strlen("AT+CGREG?\r\n"));  //get cell lac
            recvDataLen = dataRecv(fd1);
            for( i = 0; i < recvDataLen; i++){
                printf("%c", RxBuffer[i]);
            }
            if(1 == moduleFlage){
                getCellLac(MInfo->lac, MInfo->cell);
                printf("HUAWEI Get cell lac OK!");
            }else if(2 == moduleFlage || 3 == moduleFlage){
                getZteCellLac(MInfo->lac, MInfo->cell, recvDataLen);
                printf("ZTE Get cell lac OK!");
            }

            //get sim card ICCID
            memset(RxBuffer, 0, 1024);
            if(2 == moduleFlage || 3 == moduleFlage){  //ZTE
                send_data(fd1, "AT+ZGETICCID\r\n", strlen("AT+ZGETICCID\r\n"));
                recvDataLen = dataRecv(fd1);
                //for( i = 0; i < recvDataLen; i++){
                //    printf(" %d: %c", i, RxBuffer[i]);
                //}
                memset(ICCID, 0, 20);
                for(i = 26, k = 0; i <= 45; i++, k++){
                    ICCID[k] = RxBuffer[i];
                }
                ICCID[k] = '\0';
                printf("ICCIDT:%s\n", ICCID);
            }else if(1 == moduleFlage){ //HUAWEI
                send_data(fd1, "AT^ICCID?\r\n", strlen("AT^ICCID?\r\n"));
                recvDataLen = dataRecv(fd1);
                //for( i = 0; i < recvDataLen; i++){
                //    printf(" %d: %c", i, RxBuffer[i]);
                //}
                memset(ICCID, 0, 22);
                for(i = 21, k = 0; i <= 40; i++, k++){
                    ICCID[k] = RxBuffer[i];
                }
                ICCID[k] = '\0';
                printf("ICCIDH:%s\n", ICCID);
            }

            //write to file
            IccidFd = open("/etc/iccid", O_RDWR|O_CREAT|O_TRUNC);
            if (IccidFd < 0)
            {
                printf("Can't open config file to read mac\n");
                return 0;
            }
            write(IccidFd, ICCID, strlen(ICCID));
            memset(RxBuffer, 0, 1024);
            send_data(fd1, "AT+CIMI\r\n", strlen("AT+CIMI\r\n"));  //get mnc mcc
            recvDataLen = dataRecv(fd1);
            for( i = 0; i < recvDataLen; i++){
                printf("%c", RxBuffer[i]);
            }
            if(1 == moduleFlage){
                getMncMcc(MInfo->mcc, MInfo->mnc);
                printf("HWAWEI Get mnc mcc OK!");
            }else if(2 == moduleFlage || 3 == moduleFlage){
                getZteMncMcc(MInfo->mcc, MInfo->mnc);
                printf("ZTE Get mnc mcc OK!");
            }

            if(1 == moduleFlage){
                memset(RxBuffer, 0, 1024);
                send_data(fd1, "AT^HCSQ?\r\n", strlen("AT^HCSQ?\r\n"));
                recvDataLen = dataRecv(fd1);
                for( i = 0; i < recvDataLen; i++){
                    printf("%c", RxBuffer[i]);
                }
                getSingStrength(MInfo->sing, MInfo->singMode);
                printf("Got sing strength\n");
            }else if(2 == moduleFlage || 3 == moduleFlage){
                memset(RxBuffer, 0, 1024);
                send_data(fd1, "AT+CSQ\r\n", strlen("AT+CSQ\r\n"));
                recvDataLen = dataRecv(fd1);
                for(i = 0; i < recvDataLen; i++){
                    printf("%c", RxBuffer[i]);
                }
                getZteSingStrength(MInfo->sing);
                printf("ZTE Get sing OK!");
                strcpy(MInfo->singMode, "6"); //联通3G
            }

            memset(RxBuffer, 0, 1024);
            send_data(fd1, "AT+CGSN\r\n", strlen("AT+CGSN\r\n"));
            recvDataLen = dataRecv(fd1);
            for(i = 0; i < recvDataLen; i++){
                printf("%c", RxBuffer[i]);
            }
            if(1 == moduleFlage){
                j = 0;
                while(j < 15){
                    printf("%c", RxBuffer[j + 11]);
                    MInfo->imei[j] = RxBuffer[j + 11];
                    j++; 
                }
            }else if(3 == moduleFlage){
                j = 0;
                while(j < 15){
                    printf("%c", RxBuffer[j + 16]);
                    MInfo->imei[j] = RxBuffer[j + 16];
                    j++; 
                }
            }else if(2 == moduleFlage){
                j = 0;
                while(j < 15){
                    printf("%c", RxBuffer[j + 10]);
                    MInfo->imei[j] = RxBuffer[j + 10];
                    j++; 
                }
            }

            setIMEI(MInfo->imei);
            printf("\nIMEI done %s\n", MInfo->imei);
        }
    }

    printf("Resolve: lac: %s, cell: %s, mcc: %s, mnc: %s\n", MInfo->lac, MInfo->cell, MInfo->mcc, MInfo->mnc);

    close(fd1);
    return 1;
}

int getAdaptorID(unsigned char *adaptname, unsigned char *controluuid)
{
    int fdr;
    char mac[20] = "";
    char buf[50] = "";

    memset(buf, 0, strlen(buf));

#if 0
    fdr = open("/data/bin/mac", O_RDONLY);
    if(fdr < 0)
    {
        printf("Can't open config file to read mac\n");
        return 0;      
    }

    read(fdr, buf, 50);
    strcpy(mac, buf);
    strcpy(adaptname, "01_TX3251_");
    strcat(adaptname, mac);
    close(fdr);
#endif

    char buf1[10], buf2[10];
    char bufUid[20];
    int fd = 0, i = 0, j = 0;

    memset(bufUid, 0, sizeof(bufUid));

    fd = open("/sys/fsl_otp/HW_OCOTP_CFG0", O_RDONLY);
    if(fd < 0){
        perror("Open /sys/fsl_otp/HW_OCOTP_CFG0");
        return 0;
    }

    read(fd, buf1, 10);
    for(i = 0, j = 2; j < 10; i++, j++){
        if(buf2[j] != '\n'){
            bufUid[i] = buf1[j];
        }else 
            break;
    }
    //printf("read id  1 is :%s\n", buf1);
    close(fd);


    fd = open("/sys/fsl_otp/HW_OCOTP_CFG1", O_RDONLY);
    if(fd < 0){
        perror("Open /sys/fsl_otp/HW_OCOTP_CFG1");
        return 0;
    }

    read(fd, buf2, 10);
    for(j = 2; j < 10; i++, j++){
        if(buf2[j] != '\n'){
            bufUid[i] = buf2[j];
        }else
            break;
    }
    bufUid[i] = '\0';
    //printf("read id  2 is :%s\n", buf2);
    printf("read cpuID   is :%s\n", bufUid);
    close(fd);

    strcpy(adaptname, "01_TX3251_");
    strcat(adaptname, bufUid);


    memset(buf, 0, strlen(buf));
    fdr = open("/data/bin/udid", O_RDONLY);
    if(fdr < 0)
    {
        printf("Can't open config file to read udid\n");
        return 0;      
    }
    read(fdr, buf, 50);
    strcpy(controluuid, buf);
    close(fdr);

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
            printf("find HUAWEI MU709s-2!\n");
            moduleFlage = 1;
            break;
        }else if(strncmp(idBuf[i], "19d2:1476", 9) == 0){ //ZTE ME3620
            printf("find ZTE ME3620!\n");
            moduleFlage = 2;
            break;
        }else if(strncmp(idBuf[i], "19d2:ffeb", 9) == 0){ //ZTE MW3650
            printf("find ZTE MW3650!\n");
            moduleFlage = 3;
            break;
        }else if(0 == i)
            printf("No module insert!\n");
    }
    pclose(fp);
}

int main(int argc, char **argv)
{
    int sockfd, ret, i, h;
    struct sockaddr_in servaddr;
    struct hostent *twHost;
    char str1[4096], str2[4096], buf[BUFSIZE], *str;
    socklen_t len;
    fd_set   t_set1;
    struct timeval  tv;
    char **pptr;
    char straddr[32] = "Server ip";
    struct ModuleInfo mInfo;
    unsigned char adaptname[30] = {0}, controluuid[30] = {0};
    //strcpy(adaptname, "01_TX3251_001122334455");
    //strcpy(controluuid, "987654321012");
    char hex[10] = {0}; 
    judgementModule(); //maching module
    getAdaptorID(adaptname, controluuid);
    strcpy(controluuid, "1234567654321");
    printf("%d \n", moduleFlage);
    if(1 == moduleFlage){ //HUAWEI MU709s-2
        getLocation(&mInfo, "/dev/ttyUSB2");
    }else if(2 == moduleFlage){
        getLocation(&mInfo, "/dev/ttyUSB1"); //ZTE ME3620 
    }else if(3 == moduleFlage){ //ZTE MW3650 
        getLocation(&mInfo, "/dev/ttyUSB0");
    }

    strcpy(hex, "16");

    //if((twHost = gethostbyname(twHostName)) == NULL){
    //    printf("gethostbyname error for host:%s\n", twHostName);
    //    return 0;
    //}

    //printf("official hostname: %s\n", twHost->h_name);
    //pptr = twHost->h_addr_list;
    //printf("address:%s\n", inet_ntop(twHost->h_addrtype, *pptr, straddr, sizeof(straddr)));

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        printf("创建网络连接失败,本线程即将终止---socket error!\n");
        exit(0);
    };

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, straddr, &servaddr.sin_addr) <= 0 ){
        printf("创建网络连接失败,本线程即将终止--inet_pton error!\n");
        exit(0);
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
        printf("连接到服务器失败,connect error!\n");
        exit(0);
    }
    printf("与远端建立了连接\n");

    //发送数据
    memset(str2, 0, 4096);
    //strcat(str2, "Just test string");
    sprintf(str2, "{\"mnc\":\"1\", \"lac\":\"%s\", \"cell\":\"%s\", \"hex\":\"%s\", \"adaptorName\":\"%s\", \"controllerUDID\":\"%s\", \"signalSource\":\"%s\", \"signalStrength\":\"%s\"}\r\n", mInfo.lac, mInfo.cell, hex, adaptname, controluuid, mInfo.singMode, mInfo.sing);
    str=(char *)malloc(128);
    len = strlen(str2);
    sprintf(str, "%d", len);

    memset(str1, 0, 4096);
    strcat(str1, "POST some url you and set HTTP/1.1\r\n");
    strcat(str1, "Host: xf.tandatech.com:5000\r\n");
    strcat(str1, "Accept: application/json\r\n");
    strcat(str1, "appKey: 111111111111111111111111\r\n");
    strcat(str1, "Content-Type: application/json\r\n");
    strcat(str1, "Content-Length: ");
    strcat(str1, str);
    strcat(str1, "\r\n\r\n");

    strcat(str1, str2);
    strcat(str1, "\r\n\r\n");
    printf("%s\n",str1);

    ret = write(sockfd, str1, strlen(str1));
    if (ret < 0) {
        printf("发送失败！错误代码是%d，错误信息是'%s'\n",errno, strerror(errno));
        exit(0);
    }else{
        printf("消息发送成功，共发送了%d个字节！\n\n", ret);
    }

    FD_ZERO(&t_set1);
    FD_SET(sockfd, &t_set1);

    while(1){
        sleep(2);
        tv.tv_sec= 6;
        tv.tv_usec= 0;
        h= 0;
        printf("--------------->1");
        h= select(sockfd +1, &t_set1, NULL, NULL, &tv);
        printf("--------------->2");

        if (h == 0) {
	    return -1;
        }
        if (h < 0) {
            close(sockfd);
            printf("在读取数据报文时SELECT检测到异常，该异常导致线程终止！\n");
            return -1;
        }

        if (h > 0){
            memset(buf, 0, 4096);
            i= read(sockfd, buf, 4095);
            if (i==0){
                close(sockfd);
                printf("读取数据报文时发现远端关闭，该线程终止！\n");
                return -1;
            }

            printf("%s\n", buf);
            break;
        }
    }
    close(sockfd);


    return 0;
}
