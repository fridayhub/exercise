#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* read mac id from *eth  */
int getThingMac(char *mac)
{
    char *eth = "eth0"; //name need read of network card
    struct ifreq ifreq;
    int sock = 0;
    int i = 0;

    sock = socket(AF_INET, SOCK_STREAM,0);
    if(sock < 0)
    {
        perror("error sock");
    	close(sock);
        return 2;
    }

    strcpy(ifreq.ifr_name, eth);
    if(ioctl(sock, SIOCGIFHWADDR, &ifreq) < 0)
    {
        perror("error ioctl");
    	close(sock);
        return 3;
    }

    for(i = 0; i < 6; i++){
        sprintf(mac+3*i, "%02X:", (unsigned char)ifreq.ifr_hwaddr.sa_data[i]);
    }
    mac[strlen(mac) - 1] = 0;
    close(sock);

    return 0;
}

int readAdaptorID()
{
    int fdr;
    char buf[50] = "";    // 缓冲字符串
    char mac[20] = ""; 
    char execCmd[50];

    memset(buf, 0, strlen(buf));

    fdr = open("/etc/mac", O_RDWR|O_CREAT);
    if (fdr < 0)
    {
        printf("Can't open config file to read mac\n");
        return 0;
    }

    // 读取一行
    read(fdr, buf, 50);

    if(strlen(buf) > 0){  //配置文件中有存mac地址
        strcpy(mac, buf);
    }else{
        getThingMac(mac);
        write(fdr, mac, strlen(mac));
    }
    printf("read mac is %s\n", mac);
    // 清空 读取出来的 key
    //system("ifconfig eth0 down"); 

    system("ifconfig eth0 up");
    strcpy(execCmd, "ifconfig eth0 hw ether ");
    strcat(execCmd, mac);
    printf("cmd is : %s\n", execCmd);
    system(execCmd);
    close(fdr);
    return 1;
}

int main()
{

   readAdaptorID();
}
