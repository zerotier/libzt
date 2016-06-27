// TCP Client test program

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
 
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
        printf("Could not create socket");
    }     
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_family = AF_INET;
    server.sin_port = htons( port );
 
    printf("connecting...\n");
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
        perror("connect failed. Error");
        return 1;
    }
    puts("Connected\n");
     
    while(1) {
        printf("Enter message : ");
        scanf("%s" , message);

        // TX         
        if(send(sock , "welcome to the machine!" ,24 , 0) < 0) {
            puts("Send failed");
            return 1;
        }
    	else {
    		printf("len = %d\n", strlen(message));
    	}

        // RX
        if(recv(sock , server_reply , 2000 , 0) < 0) {
            puts("recv failed");
            break;
        }
        puts("Server reply :");
        puts(server_reply);
    }
    close(sock);
    return 0;
}
