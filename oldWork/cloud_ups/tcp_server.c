/*

FileName:

Author:

Description:

Version:

FunctionList:

History:
    <author>  <time>  <version>  <desc>
 
*/

#include "tcp_server.h"

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
#define ALARM_MSG "alarm"

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

int do_tcp_alarm(void)
{
    int size = sizeof(fd_A)/sizeof(fd_A[0]);
    printf("%s start fd size:%d\n", __func__, size);
	int ret = 0;
    int i;

    for( i = 0; i < size; i++ )
    {

        if( fd_A[i].fd != 0 )
	    {
            printf("send power_off to fd:%d \n", fd_A[i].fd);
            pthread_rwlock_wrlock(&fd_A[i].rwlock);
            ret = send(fd_A[i].fd, ALARM_MSG, sizeof(ALARM_MSG), 0);
            pthread_rwlock_unlock(&fd_A[i].rwlock);
	        usleep(1000);
        }

    }

    printf("%s end\n", __func__);
	return ret;

}


int  server_init(void)
{
    int sock_fd ;                      // listen on sock_fd, new connection on new_fd
    struct sockaddr_in server_addr;    // server address information
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

int do_heartbeat(struct FD_LOCK* fd_lock, fd_set* fdsr)
{
    int ret;
    char buf[BUF_SIZE];
    printf("<<<%s fd:%d\n", __func__, fd_lock->fd);

    pthread_rwlock_rdlock(&fd_lock->rwlock);
    ret = recv(fd_lock->fd, buf, sizeof(buf), 0);
    pthread_rwlock_unlock(&fd_lock->rwlock);

    if (ret <= 0) 
    {   // client close
        close(fd_lock->fd);
        FD_CLR(fd_lock->fd, fdsr);
        fd_lock->fd = 0;
	conn_amount--;  //减少客户端连接数
    } 
    else 
    {    // receive data

        if (ret < BUF_SIZE)
	{
		memset(&buf[ret], '\0', 1);
	}

        printf("client[%d] send:%s\n", fd_lock->fd, buf);

	if(!strcmp(buf, "ping"))
	{
		strcpy(buf, "pang");
		pthread_rwlock_wrlock(&fd_lock->rwlock);
		send(fd_lock->fd, buf, strlen(buf), 0);
		pthread_rwlock_unlock(&fd_lock->rwlock);		    
	}
    }

    return ret;
}

int add_client(int sock_fd, int* maxsock)
{
    int new_fd;                        // new connection on new_fd
    int i;
    struct sockaddr_in client_addr;    // connector's address information
    socklen_t sin_size;                // socklen_t，和int 有点像，用来表示长度
    sin_size = sizeof(client_addr);

    new_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &sin_size);
    if (new_fd <= 0) 
    {
        perror("accept");
        //continue;
	return new_fd;
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
	    if (new_fd > *maxsock)
	    {
		    *maxsock = new_fd;
	    }
    }
    else 
    {
	    printf("max connections arrive, exit\n");
	    send(new_fd, "bye", 4, 0);
	    close(new_fd);
	    // break;
    }

    return 0;

}

int run_tcp_server(void)
{

    int sock_fd;      // listen on sock_fd
    int ret;
    int i;

    //socket init
    sock_fd = server_init();

    fd_set fdsr;
    int maxsock;
    struct timeval tv;

    conn_amount = 0;
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
        } 
	else if (ret == 0) 
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
		    ret = do_heartbeat(&fd_A[i], &fdsr);
		    if (ret <= 0) 
		    {   // client close
			    printf("client[%d] close ret:%d\n", i, ret);
		    }
	    }
	}

        // check whether a new connection comes
        if (FD_ISSET(sock_fd, &fdsr)) 
	{
		ret = add_client(sock_fd, &maxsock);
		if (ret <= 0)
		{
			continue;
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

