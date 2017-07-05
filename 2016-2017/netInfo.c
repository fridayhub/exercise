#include <stdio.h> 
#include <unistd.h>
#include <sys/types.h> 
#include <sys/param.h> 
#include <sys/ioctl.h> 
#include <sys/socket.h> 
#include <net/if.h> 
#include <netinet/in.h> 
#include <net/if_arp.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#define MAXINTERFACES 16 
int main() 
{ 
    int fd, intrface, retn = 0; 
    struct ifreq buf[MAXINTERFACES]; 
    struct ifconf ifc; 
    if ((fd = socket (AF_INET, SOCK_DGRAM, 0)) >= 0) 
    { 
        ifc.ifc_len = sizeof buf; 
        ifc.ifc_buf = (caddr_t) buf; 
        if (!ioctl (fd, SIOCGIFCONF, (char *) &ifc)) 
        { 
            //获取接口信息

            intrface = ifc.ifc_len / (int)sizeof(struct ifreq); 
            printf("interface num is intrface=%d\n\n\n",intrface); 
            //根据借口信息循环获取设备IP和MAC地址

            while (intrface-- > 0) 
            { 
                //获取设备名称
                printf ("net device %s\n", buf[intrface].ifr_name); 

                //判断网卡类型 
                if (!(ioctl (fd, SIOCGIFFLAGS, (char *) &buf[intrface]))) 
                { 
                    if (buf[intrface].ifr_flags & IFF_PROMISC) 
                    { 
                        puts ("the interface is PROMISC" ); 
                        retn++; 
                    } 
                } 
                else 
                { 
                    char str[256]; 
                    sprintf (str, "cpm: ioctl device %s", buf[intrface].ifr_name); 
                    perror (str); 
                } 
                //判断网卡状态 
                if (buf[intrface].ifr_flags & IFF_RUNNING) 
                { 
                    puts("the interface status is UP" ); 
                } 
                else 
                { 
                    puts("the interface status is DOWN" ); 
                } 
                //获取当前网卡的IP地址 
                if (!(ioctl (fd, SIOCGIFADDR, (char *) &buf[intrface]))) 
                { 
                    puts ("IP address is:" ); 
                    puts(inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr)); 
                    //puts (buf[intrface].ifr_addr.sa_data); 
                } 
                else 
                { 
                    char str[256]; 
                    sprintf (str, "cpm: ioctl device %s", buf[intrface].ifr_name); 
                    perror (str); 
                } 
                /* this section can't get Hardware Address,I don't know whether the reason is module driver*/ 
                if (!(ioctl (fd, SIOCGIFHWADDR, (char *) &buf[intrface]))) 
                { 
                    puts ("HW address is:" ); 
                    printf("%02x:%02x:%02x:%02x:%02x:%02x\n", 
                            (unsigned char)buf[intrface].ifr_hwaddr.sa_data[0], 
                            (unsigned char)buf[intrface].ifr_hwaddr.sa_data[1], 
                            (unsigned char)buf[intrface].ifr_hwaddr.sa_data[2], 
                            (unsigned char)buf[intrface].ifr_hwaddr.sa_data[3], 
                            (unsigned char)buf[intrface].ifr_hwaddr.sa_data[4], 
                            (unsigned char)buf[intrface].ifr_hwaddr.sa_data[5]); 
                    puts("" ); 
                    puts("" ); 
                } 
                else 
                { 
                    char str[256]; 
                    sprintf (str, "cpm: ioctl device %s", buf[intrface].ifr_name); 
                    perror (str); 
                } 
            } //while

        } else 
            perror ("cpm: ioctl" ); 
    } else 
        perror ("cpm: socket" ); 
    close(fd); 
    return retn; 
}
