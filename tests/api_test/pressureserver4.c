// TCP Server test program

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h>  
 

#define MSGSZ 1024

int atoi(const char *str);

int main(int argc , char *argv[])
{
    if(argc < 2) {
       printf("usage: tcp_server <port>\n");
       return 0;
    }

    int sock, client_sock, c, read_size, port = atoi(argv[1]);
    char message[MSGSZ];
     
    char str[MSGSZ];
    int comm_fd;
 
    struct sockaddr_in servaddr;
    struct sockaddr_in client;

    sock = socket(AF_INET, SOCK_STREAM, 0); 
    bzero( &servaddr, sizeof(servaddr));
 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(port);
    bind(sock, (struct sockaddr *) &servaddr, sizeof(servaddr));

    printf("listening\n");
    listen(sock , 3); 
    printf("waiting to accept\n");
    c = sizeof(struct sockaddr_in);
    
    client_sock = accept(sock, (struct sockaddr *)&client, (socklen_t*)&c);
    if (client_sock < 0) {
        perror("accept failed");
        return 0;
    }
    printf("connection accepted\n reading...\n");
     
    // RX

    unsigned long count = 0;
    while(1)
    {
       count++;
       int bytes_read = read(client_sock, message, MSGSZ);
       printf("[%lu] RX = (%d): ", count, bytes_read);
       //for(int i=0; i<bytes_read; i++) {
       //   printf("%c", message[i]);
       //}

       // TX
       int bytes_written = write(client_sock, message, MSGSZ); 
       printf("\t\nTX = %d\n", bytes_written);
    }
    return 0;
}
