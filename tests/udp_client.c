// UDP Client test program

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char * argv[])
{
    printf("cpp_udp_socket_client_test():\n");
    ssize_t n_sent;
    int sock = -1;
    struct sockaddr_in server;
    char buf[64];    



    if(sock == -1) {
        sock = socket(AF_INET , SOCK_DGRAM , 0);
        if (sock == -1) {
            return 1;
        }
    }
    server.sin_addr.s_addr = inet_addr("10.242.142.99");
    server.sin_family = AF_INET;
    server.sin_port = htons(9997);
    
    //char *buf = "Welcome to the Machine\n";
    memcpy(buf, "Welcome to the Machine", sizeof("Welcome to the Machine")); 

   printf("sizeof(buf) = %d\n", sizeof(buf));    
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
        printf("api_test: error while connecting.\n");
        return 1;    
	}


char data[1024];
memset(data, 0, sizeof(data));
int count =0;
while(1)
{
count++;
    usleep(1000);
    n_sent = send(sock,data,sizeof(data),0);

    if (n_sent<0) {
        perror("Problem sending data");
        return 1;
    }
    if (n_sent!=sizeof(buf))
        printf("Sendto sent %d bytes\n",(int)n_sent);
    
    printf("n_sent = %d, count = %d\n", n_sent,count);
} 
   return 1;


    socklen_t recv_addr_len;
    // Clear address info for RX test
    server.sin_addr.s_addr = inet_addr("");
    server.sin_port = htons(-1);
    
    while (1) {
        n_sent=recvfrom(sock,buf,sizeof(buf),0,(struct sockaddr *)&server,&recv_addr_len);
        printf("Got a datagram from %s port %d\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));
        if (n_sent<0) {
            perror("Error receiving data");
        }
        else {
            printf("RXed: %s\n", buf);
        }
    }
    return 1;
}


