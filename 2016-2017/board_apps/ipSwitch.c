#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#define LOCKFILE "/var/run/ipchange.pid"
#define LOCKMODE (S_IRUSR | S_IWOTH | S_IRGRP | S_IROTH)

int moduleFlage = 0;
volatile int net_flag;
char *eth = "eth0"; //name need read of network card

static sigjmp_buf jmpbuf;
static void alarm_func()
{
    siglongjmp(jmpbuf, 1);
}


int checkConnetc(char *interface)
{
    char portStr[10];
    int port = 80;
    int rval;
    struct addrinfo hints, *result, *rp;

    struct ifreq ifr;
    char *host[] = {"www.baidu.com", "www.jzxfyun.com", "www.qq.com"};
    int sfd;
    struct sockaddr_in server;

    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    int i = 0;

    snprintf(portStr, 10, "%u", port);

    for(i = 0; i < 3; i++){
        bzero(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        signal(SIGALRM, alarm_func);
        if(sigsetjmp(jmpbuf, 1) != 0){  //if not eq 0, return from siglongjum
            alarm(0);
            signal(SIGALRM, SIG_IGN);
            printf("time out Net error!\n");
            return -1;
        }
        alarm(10);
       // printf("Start check to %s\n", host[i]);
        rval = getaddrinfo(host[i], portStr, &hints, &result);
        if (rval != 0) {
            printf("Get ip from dns error\n");
            return -1;
        }
        signal(SIGALRM, SIG_IGN);
        //printf("Get ip from dns ok \n");

        int k;
        for(rp = result; rp != NULL; rp = rp->ai_next){
            /*check ipaddr*/
            //printf("-->addr len:%d\n", rp->ai_addrlen);
            int tmp_ip[rp->ai_addrlen]; //tmp save ip addr
            memset(tmp_ip, 0, rp->ai_addrlen); //set to zero
            for(k = 0; k < rp->ai_addrlen; k++){
                if(rp->ai_addr->sa_data[k] < 0){
                    tmp_ip[k] = (255 + rp->ai_addr->sa_data[k]);
                }else{
                    tmp_ip[k] = rp->ai_addr->sa_data[k];
                }
                //printf("%d ", tmp_ip[k]);
            }
            //printf("\n");

            for(k = 0; k < rp->ai_addrlen; k++){
                if(tmp_ip[k] == 192 && tmp_ip[k+1] == 168){ //192.168...
                    printf("\n*******The Fuck local_network!\n");
                    return -1;
                } 
                if(tmp_ip[k] == 172 && (tmp_ip[k+1] < 32)){ //172.16...
                    printf("\n*******The Fuck local_network!\n");
                    return -1;
                }
                if(tmp_ip[2] == 10){ //10.....
                    printf("\n*******The Fuck local_network!\n");                 
                    return -1;
                }
            }

            /*create socket */
            sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if(sfd == -1){
                perror("create socket");
                continue;
            }
        //    printf("Socket created\n");

            /*set socket to eth0*/
            memset(&ifr, 0, sizeof(ifr));
            strncpy(ifr.ifr_name, interface, IFNAMSIZ-1);
            if(setsockopt(sfd, SOL_SOCKET, SO_BINDTODEVICE, (char*)&ifr, sizeof(ifr))<0){
                close(sfd);
                perror("set socke bind to deive opt");
                continue;
            }
         //   printf("set sock opt ok\n");

            if(setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(timeout)) < 0){
                close(sfd);
                perror("set socke rcv timeout opt");
                continue;
            }

           // printf("set timeout ok, start to connect\n");
            if(setsockopt(sfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(timeout)) < 0){
                close(sfd);
                perror("set socke rcv timeout opt");
                continue;
            }

            /*try to server*/
            if(connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1){
                printf("Connect to %s ok\n", host[i]);
                return 1;
                //break;
            }
            perror("Connect faild");
            close(sfd);
        }

        /*free rp and result*/
        if(rp == NULL){
            fprintf(stderr, "Clould not connect\n");
            //return -1;
            //exit(EXIT_FAILURE);
        }
        freeaddrinfo(result); /* No longer needed */
        printf("Check done\n");
    }
    return -1;

}

/* List net device name */
int listNetDeviceName(char netName[5][10])
{
    struct ifreq ifr;
    struct ifconf ifc;
    char buf[2048];

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == -1) {
        printf("socket error\n");
        return -1;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) {
        printf("ioctl error\n");
        return -1;
    }

    struct ifreq* it = ifc.ifc_req;
    const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));
    int i = 0;
    //int count = 0;
    for (; it != end; ++it, i++) {
        strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
            if (! (ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                    //count ++ ;
                    //unsigned char * ptr ;
                    //ptr = (unsigned char  *)&ifr.ifr_ifru.ifru_hwaddr.sa_data[0];
                    //snprintf(szMac,64,"%02X:%02X:%02X:%02X:%02X:%02X",*ptr,*(ptr+1),*(ptr+2),*(ptr+3),*(ptr+4),*(ptr+5));
                    //printf("%d,Interface name : %s , Mac address : %s \n",count,ifr.ifr_name,szMac);
                    strcpy(netName[i], ifr.ifr_name);
                }
            }
        }else{
            printf("get mac info error\n");
            return -1;
        }
    }
    return i;
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

void switch3G(struct ifreq *ifreq)
{
    int i = 0, ret = 0;
    int netFlag = 0, pppFlag = 0;
    char netName[10][10] = {"0"};
    printf("%s is not running \n", ifreq->ifr_name);
    memset(netName, 0, sizeof(netName));
    ret = listNetDeviceName(netName);
    for(i = 0; i < ret; i++){
        printf("name is %s\n", netName[i]);
        if(strcmp(netName[i], "ppp0") == 0)
            pppFlag = 1;

        if(strcmp(netName[i], "eth0:1") == 0){
            system("ifconfig eth0:1 down");
            netFlag = 1;
        }

    }
    /*    if(netFlag){ //if eth0:1 is exist
          printf("switch3G netflag...\n");
          system("ifconfig eth0:1 down");
          }*/

    if(!pppFlag){
        printf("switch    3g...\n");
        judgementModule();
        system("killall pppd");
        system("/sbin/route del default");
        if(1 == moduleFlage){
            system("/sbin/pppd call options.pppoe"); //3G 拨号
            system("cp /etc/ppp/resolv.conf /etc/resolv.conf");
            printf("HUAWEI pppd call once\n");
        }else if(2 == moduleFlage){
            system("cd /etc/ppp/zte_me3620 && /bin/sh ppp-on &");
            printf("ZTE ME3620 pppd call once\n");
        }else if(3 == moduleFlage){
            system("cd /etc/ppp/zte_mw3650 && /bin/sh ppp-on &");
            printf("ZTE MW3650 pppd call once\n");
        }

        //wait pppd finished
        sleep(40);
        system("cp /etc/ppp/resolv.conf /etc/");
    }
    pppFlag = 0;
    netFlag = 0;
    printf("Use 3G net\n");
    exit(1);
}

void switchEternet(struct ifreq *ifreq)
{
    int i = 0, ret = 0;
    int netFlag = 0, pppFlag = 0;
    char netName[10][10] = {"0"};

    ret = listNetDeviceName(netName);
    printf("%s is running \n", ifreq->ifr_name);
    for(i = 0; i < ret; i++){
        printf("name is %s\n", netName[i]);


        if(strcmp(netName[i], "eth0:1") == 0)
            netFlag = 1;
        if(strcmp(netName[i], "ppp0") == 0)
            pppFlag = 1;
        // if(strcmp(netName[i], "eth0:0") == 0)
        //  netFlag = 2;

    }            
    if(!netFlag){ //eth0:1 is down
        system("/sbin/route del default");
        system("ifconfig eth0 down");
        system("ifconfig eth0 up");
        system("udhcpc -i eth0:1"); //插入网线udhcp一下IP地址 
        printf("udhcpc run once\n");
    }

    if(pppFlag){ //网络切换至以太网，并且可用
        printf("switchEternet pppflag....\n");
        system("killall pppd");
    }

    netFlag = 0;
    pppFlag = 0;
    printf("Use ethernet \n");
}

/* read mac id from *eth  */
int netDemo(void)
{
    struct ifreq ifreq;
    int skfd = 0;
    int ret = 0;

    skfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if(skfd < 0)
    {
        perror("error sock");
        return 2;
    }

    printf("\nnetDemo>>>>\n");
    strcpy(ifreq.ifr_name, eth);

    if(ioctl(skfd, SIOCGIFFLAGS, &ifreq) <0 )
    {
        printf("%s:%d IOCTL error!\n", __FILE__, __LINE__);
        printf("Maybe ethernet inferface %s is not valid!", ifreq.ifr_name);
        close(skfd);
        return -1;
    }


    if(ifreq.ifr_flags & IFF_UP)
    {
        printf("IFF_UP..\n");
    }

    if(ifreq.ifr_flags & IFF_RUNNING)
    {
        printf(" switchEternet...\n");
        switchEternet(&ifreq);  
    }else{ //网线被拔掉
        if(checkConnetc("ppp0") == 1){
            printf("3G network test ok!\n");
            return 0;
        }else{
            system("killall pppd");
            printf("switch3G...\n");
            switch3G(&ifreq);
        }
    }

    close(skfd);
    return 0;
}

int lockfile(int fd)
{
    struct flock flk;
    flk.l_type = F_WRLCK; //write lock
    flk.l_start = 0;
    flk.l_whence = SEEK_SET;
    flk.l_len = 0; //lock the whole file

    return(fcntl(fd, F_SETLK, &flk));
}

int check_running(const char *flock)
{
    int fd;
    char buf[16];

    fd = open(flock, O_RDWR | O_CREAT, LOCKMODE);
    if(fd < 0){
        perror("Open lock file");
        exit(1);
    }

    if(lockfile(fd) == -1){  //测试写文件锁
        if(errno == EACCES || errno == EAGAIN){
            printf("file: %s already locked\n", flock);
            printf("\nERR0R == EACCES...\n");
            close(fd);
            return 1;
        }
        printf("can't lock %s: %m\n", *flock);
        exit(1);
    }

    printf("lockfile end......\n");
    ftruncate(fd, 0);  //清空锁文件 写入新的pid
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);
    printf("\nnew getpid = %ld\n",getpid());
    return 0;
}

int main()
{

    printf("net_flag = %d\n",net_flag);

    if(1)
    {
        //		net_flag = 1;
        printf("\ncheckconnet.....\n");
        if(check_running(LOCKFILE)){
            printf("\ncheck_running..... \n");
            return 0;
        }

    }

    //readAdaptorID();
    netDemo();
    printf("\nnetDemo.....\n");
}
