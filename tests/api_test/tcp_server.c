// TCP Server test program

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h>  
 
int atoi(const char *str);

int main(int argc , char *argv[])
{
    if(argc < 2) {
       printf("usage: tcp_server <4/6> <port>\n");
       printf("\t - where 4/6 represent IP version\n");
       return 0;
    }
   

    int sock, client_sock, c, read_size, ipv = atoi(argv[1]), port = atoi(argv[2]);
    char client_message[2000];
     
    struct sockaddr_storage server, client;
    struct sockaddr_in6 *server6 = (struct sockaddr_in6 *)&server;
    struct sockaddr_in *server4 = (struct sockaddr_in *)&server;

    // IPV4
    if(ipv == 4)
    {
        printf("ipv4 mode\n");
        if((sock = socket(AF_INET, SOCK_STREAM , 0)) < 0) {
            printf("could not create socket");
            return 0;
        }
        server4->sin_family = AF_INET;
        server4->sin_addr.s_addr = INADDR_ANY;
        server4->sin_port = htons(port);
    }
    // IPV6
    if(ipv == 6)
    {
        printf("ipv6 mode\n");
        if((sock = socket(AF_INET6, SOCK_STREAM , 0)) < 0) {
            printf("could not create socket");
            return 0;
        }
        server6->sin6_family = AF_INET6;
        server6->sin6_addr = in6addr_any;
        server6->sin6_port = htons(port);
    }
     
    printf("binding on port %d\n", port);
    if( bind(sock,(struct sockaddr *)&server , sizeof(server)) < 0) {
        perror("bind failed. Error");
        return 0;
    }
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
