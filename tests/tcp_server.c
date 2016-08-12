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
	   printf("usage: tcp_server <port>\n");
       return 0;
    }
   
    int socket_desc, client_sock, c, read_size, port = atoi(argv[1]);
    struct sockaddr_in server , client;
    char client_message[2000];
     
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1) {
        printf("could not create socket");
        return 0;
    }
     
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
     
    printf("binding on port %d\n", port);
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
        perror("bind failed. Error");
        return 0;
    }
    printf("listening\n");
    listen(socket_desc , 3); 
    printf("waiting to accept\n");
    c = sizeof(struct sockaddr_in);
    
    client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
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
