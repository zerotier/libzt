// TCP Server test program (IPV4)

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h> 
#include <unistd.h>
  
#include <cstdlib>
#include "sdk.h"

int atoi(const char *str);

int main(int argc , char *argv[])
{    
    if(argc < 3) {
        printf("usage: client <port> <netpath> <nwid>\n");
        return 1;
    }
    zts_init_rpc(argv[2],argv[3]);

    int sock, client_sock, c, read_size, port = atoi(argv[1]);
    char client_message[2000];
 
    struct sockaddr_in servaddr;
    struct sockaddr_in client;

    sock = zts_socket(AF_INET, SOCK_STREAM, 0); 
    bzero( &servaddr, sizeof(servaddr));
 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(port);
    zts_bind(sock, (struct sockaddr *) &servaddr, sizeof(servaddr));

    printf("listening\n");
    zts_listen(sock , 3); 
    printf("waiting to accept\n");
    c = sizeof(struct sockaddr_in);
    
    client_sock = zts_accept(sock, (struct sockaddr *)&client, (socklen_t*)&c);
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
       printf("[%lu] RX = (%d): ", count, bytes_read);
       for(int i=0; i<bytes_read; i++) {
          printf("%c", client_message[i]);
       }

       // TX
       int bytes_written = write(client_sock, "Server here!", 12); 
       printf("\t\nTX = %d\n", bytes_written);
    }
    return 0;
}
