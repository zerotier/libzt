#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

#define MAXBUF 65536

int main(int argc, char* argv[])
{
   int status;
   struct addrinfo sainfo, *psinfo;
   struct hostent *server;
   char buffer[MAXBUF];

   int sock, portno, n;
   struct sockaddr_in6 serv_addr;

   if(argc < 2)
     printf("Specify a port number\n"), exit(1);

   sock = socket(PF_INET6, SOCK_DGRAM,0);

    portno = atoi(argv[2]);
    server = gethostbyname2(argv[1],AF_INET6);
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin6_flowinfo = 0;
    serv_addr.sin6_family = AF_INET6;
    memmove((char *) &serv_addr.sin6_addr.s6_addr, (char *) server->h_addr, server->h_length);
    serv_addr.sin6_port = htons(portno);

   sprintf(buffer,"Ciao");

   status = sendto(sock, buffer, strlen(buffer), 0, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
   printf("buffer : %s \t%d\n", buffer, status);

   close(sock);
   return 0;
}