// TCP Client test program

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
 
#include <cstdlib>
#include "sdk.h"

int atoi(const char *str);
int close(int filedes);

#define MSG_SZ 128

int main(int argc , char *argv[])
{
    zts_init_rpc("/root/dev/ztest5","565799d8f612388c");

    if(argc < 3) {
        printf("usage: client <addr> <port>\n");
        return 1;
    }
   
    int sock, port = atoi(argv[2]);
    struct sockaddr_in server;
    char message[MSG_SZ] , server_reply[MSG_SZ];
     
    sock = zts_socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1) {
        printf("could not create socket");
    }     
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_family = AF_INET;
    server.sin_port = htons( port );
 
    printf("connecting...\n");
    if (zts_connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
        perror("connect failed. Error");
        return 1;
    }
    printf("connected\n");

    char *msg = "welcome to the machine!";
    
    while(1)
    {
        // TX         
        if(send(sock, msg, strlen(msg), 0) < 0) {
            printf("send failed");
            return 1;
        }
        else {
            printf("TX: %s\n", msg);
            printf("len = %ld\n", strlen(msg));

            int bytes_read = read(sock, server_reply, MSG_SZ);
            if(bytes_read < 0)
                printf("\tRX: Nothing\n");
            else
                printf("\tRX = (%d bytes): %s\n", bytes_read, server_reply);
        }
    }
    close(sock);
    return 0;
}
