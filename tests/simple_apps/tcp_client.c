// TCP Client test program

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
 
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
    char message[1000] , server_reply[2000];
     
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

    char *msg = "welcome to the machine!";
    // TX         
    if(send(sock, msg, strlen(msg), 0) < 0) {
        printf("send failed");
        return 1;
    }
    else {
        printf("sent message: %s\n", msg);
        printf("len = %ld\n", strlen(msg));
    }
    close(sock);
    return 0;
}
