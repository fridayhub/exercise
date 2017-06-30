#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>  //inet_addr
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in server;
    char message[1000], server_reply[2000];
    if(argc != 3){
        printf("Usage: %s serverIP port\n", argv[0]);
        exit(0);
    }

    //Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1){
        perror("create socket");
    }
    puts("Socket created");

    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[2]));

    //Connect to remote server
    if(connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0){
        perror("connect fialed");
        return 1;
    }

    puts("Connected\n");

    //keep communicating with server
    while(1)
    {
        printf("Enter message:");
        scanf("%s", message);

        //Send some data
        if(send(sock, message, strlen(message), 0) < 0){
            puts("Send failed");
            return 1;
        }

        //Recvive a relay from the server
        if(recv(sock, server_reply, 2000, 0) < 0)
        {
            puts("recv failed");
            break;
        }

        puts("server reply:");
        puts(server_reply);
    }

    close(sock);
    return 0;
}
