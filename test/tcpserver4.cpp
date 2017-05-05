#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h>  
#include <stdlib.h>
 
int atoi(const char *str);

int main(int argc , char *argv[])
{
    if(argc < 2) {
       printf("usage: tcp_server <port>\n");
       return 0;
    }

    int comm_fd, sock, client_sock, c, read_size, port = atoi(argv[1]);
    char client_message[2000];

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

    int msglen = 1024;
    unsigned long count = 0;
    while(1)
    {
      count++;
      int bytes_read = read(client_sock, client_message, msglen);
      printf("[%lu] RX = (%d): %s\n", count, bytes_read, client_message);

       /*
       for(int i=0; i<bytes_read; i++) {
          printf("%c", client_message[i]);
       }
       */
       // TX
       //int bytes_written = write(client_sock, "Server here!", 12); 
       //printf("\t\nTX = %d\n", bytes_written);
    }
    return 0;
}
