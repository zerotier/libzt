// TCP Client test program

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MSGSZ 1024
 
int atoi(const char *str);
int close(int filedes);

int main(int argc , char *argv[])
{
    if(argc < 3) {
        printf("usage: client <addr> <port>\n");
        return 1;
    }
   
    int sock, port = atoi(argv[2]);
    struct sockaddr_in server;
    char message[MSGSZ] , server_reply[MSGSZ];
     
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1) {
        printf("could not create socket");
    }     
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_family = AF_INET;
    server.sin_port = htons( port );
 
    printf("connecting...\n");
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
        perror("connect failed. Error");
        return 1;
    }
    printf("connected\n");

    memset(message, 1, MSGSZ);
    
    while(1)
    {
        //sleep(1);
        // TX         
        if(send(sock, message, MSGSZ, 0) < 0) {
            printf("send failed");
            return 1;
        }
        else {
            printf("TX: %s\n", message);
            printf("len = %ld\n", strlen(message));

            int bytes_read = read(sock, server_reply, MSGSZ);
            if(bytes_read < 0)
                printf("\tRX: Nothing\n");
            else
                printf("\tRX = (%d bytes): %s\n", bytes_read, server_reply);
        }
    }
    close(sock);
    return 0;
}
