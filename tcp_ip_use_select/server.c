/*

FileName:

Author:

Description:

Version:

FunctionList:

History:
    <author>  <time>  <version>  <desc>
 
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include "Power_Checker.h"
#include "Driver.h"

#define MYPORT 12345    // the port users will be connecting to

#define MAX_CONNET 50      // how many pending connections queue will hold

#define BUF_SIZE 200

struct FD_LOCK
{
    int fd;              //connection fd
    pthread_rwlock_t rwlock;  //the read/write lock of fd
};

//int fd_A[MAX_CONNET];    // accepted connection fd
struct FD_LOCK fd_A[MAX_CONNET]; 
int conn_amount;      // current connection amount

void showclient()
{
    int i;

    printf("client amount: %d\n", conn_amount);

    for ( i = 0; i < MAX_CONNET; i++ ) 
    {
        printf("[%d]:%d  ", i, fd_A[i].fd);
    }

    printf("\n\n");
}

void do_alarm(struct FD_LOCK *fd_lock, int size)
{
    
    char buf[BUF_SIZE];
    printf("%s start fd size:%d\n", __func__, size);
    int i;

    //led and beep on
    Red_led(1);
    Green_led(1);
    Beep_alarm(1);

    for( i = 0; i < size; i++ )
    {

        if( fd_lock[i].fd != 0 )
	    {
            printf("send power_off to fd:%d \n", fd_lock[i].fd);
            pthread_rwlock_wrlock(&fd_lock[i].rwlock);
                strcpy(buf, "alarm");
		        send(fd_lock[i].fd, buf, strlen(buf), 0);
                //send(fd_lock[i].fd, "alarm", 6, 0);
            pthread_rwlock_unlock(&fd_lock[i].rwlock);
	    usleep(10000);
        }

    }

    printf("%s end\n", __func__);

}

void* fn_check(void* data)
{
	
    int power_fd = *(int *)data;

     while(1)
    {
       usleep(5000);	
       //for power checker
       volatile char val = do_check(power_fd);
       //printf("%s val_1 = %x\n", __func__, val);
       //printf("val = %x\n", val);
       if(val == 0x00)
       {
	   usleep(1000);
	   val = do_check(power_fd);
	   //printf("%s val_2 = %x\n", __func__, val);
	   if(0x00 == val){
               do_alarm(fd_A, sizeof(fd_A)/sizeof(fd_A[0]));		
           }
       }
        //printf("kkkk\n");
    }
}

int  server_init(void)
{
    int sock_fd ;                      // listen on sock_fd, new connection on new_fd
    struct sockaddr_in server_addr;    // server address information
    socklen_t sin_size;                // socklen_t，和int 有点像，用来表示长度
    int yes = 1;
    char buf[BUF_SIZE];

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        perror("setsockopt");
        exit(1);
    }
    
    server_addr.sin_family = AF_INET;         // host byte order
    server_addr.sin_port = htons(MYPORT);     // short, network byte order
    server_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
    memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));

    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) 
    {
        perror("bind");
        exit(1);
    }

    if (listen(sock_fd, MAX_CONNET) == -1) 
    {
        perror("listen");
        exit(1);
    }

    printf("listen port %d\n", MYPORT);
	
    return sock_fd;
}

int main(void)
{
    int new_fd, sock_fd;                        // listen on sock_fd, new connection on new_fd
    struct sockaddr_in client_addr;    // connector's address information
    char buf[BUF_SIZE];
    int ret;
    int i;
    socklen_t sin_size;                // socklen_t，和int 有点像，用来表示长度
    //for power checker
    int power_fd = init();
    //led and beep init
    driver_initial();

    pthread_t id;
    pthread_attr_t id_attr;
    pthread_attr_init(&id_attr);
    pthread_attr_setdetachstate(&id_attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&id, NULL, &fn_check, &power_fd);

    //socket init
    sock_fd = server_init();


    fd_set fdsr;
    int maxsock;
    struct timeval tv;

    conn_amount = 0;
    sin_size = sizeof(client_addr);
    maxsock = sock_fd;
    
    while (1) 
    {
        // initialize file descriptor set
        FD_ZERO(&fdsr);
        FD_SET(sock_fd, &fdsr);

         // timeout setting
        tv.tv_sec = 10;
        tv.tv_usec = 0;

        // add active connection to fd set
        for (i = 0; i < MAX_CONNET; i++) 
	    {
            if (fd_A[i].fd != 0)
	        {
                FD_SET(fd_A[i].fd, &fdsr);
            }
        }

        ret = select(maxsock + 1, &fdsr, NULL, NULL, &tv);
        if (ret < 0) 
	    {
            perror("select");
            break;
        } else if (ret == 0) 
	    {
            printf("timeout\n");
	    printf("client numbers:%d\n", conn_amount);
            continue;
        }

        // check every fd in the set
        for (i = 0; i < MAX_CONNET; i++) 
        {
            if (FD_ISSET(fd_A[i].fd, &fdsr)) 
	        {
                pthread_rwlock_rdlock(&fd_A[i].rwlock);
                ret = recv(fd_A[i].fd, buf, sizeof(buf), 0);
                pthread_rwlock_unlock(&fd_A[i].rwlock);
                if (ret <= 0) 
		        {   // client close
                    printf("client[%d] close\n", i);
                    close(fd_A[i].fd);
                    FD_CLR(fd_A[i].fd, &fdsr);
                    fd_A[i].fd = 0;
		            conn_amount--;  //减少客户端连接数
                } else 
		        {    // receive data
                    if (ret < BUF_SIZE)
                    memset(&buf[ret], '\0', 1);
                    printf("client[%d] send:%s\n", i, buf);
		            if(!strcmp(buf, "ping"))
			        {
		                strcpy(buf, "pang");
                        pthread_rwlock_wrlock(&fd_A[i].rwlock);
		    	        send(fd_A[i].fd, buf, strlen(buf), 0);
                        pthread_rwlock_unlock(&fd_A[i].rwlock);		    
		            }
                }
            }
        }

        // check whether a new connection comes
        if (FD_ISSET(sock_fd, &fdsr)) 
	    {
            new_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &sin_size);
            if (new_fd <= 0) 
	        {
                perror("accept");
                continue;
            }

            // add to fd queue
            if (conn_amount < MAX_CONNET) 
	        {
                //fd_A[conn_amount++] = new_fd;
		    for(i = 0; i < MAX_CONNET; i++)
		    {
	            if(fd_A[i].fd == 0)  //find seat to add new fd
		     {
			fd_A[i].fd = new_fd;
			conn_amount++;
			break;  //add one fd and exit
		     }
		    }
		
		//conn_amount++;

                printf("new connection client[%d] %s:%d\n", conn_amount,
                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                if (new_fd > maxsock)
                    maxsock = new_fd;
            }
            else {
                    printf("max connections arrive, exit\n");
                    send(new_fd, "bye", 4, 0);
                    close(new_fd);
                   // break;
                }
        }

        showclient();

    }

    printf("socket exit!\n");
    // close other connections
    for (i = 0; i < MAX_CONNET; i++) 
    {
        if (fd_A[i].fd != 0) 
	    {
            close(fd_A[i].fd);
        }
    }

    exit(0);
}
