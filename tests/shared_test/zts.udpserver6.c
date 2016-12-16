// UDP Server test program (IPV6)

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

#include <cstdlib>
#include "sdk.h"

#define MAXBUF 65536

int main(int argc, char *argv[])
{
   if(argc < 3) {
     printf("usage: client <port> <netpath> <nwid>\n");
     return 1;
   }
   zts_init_rpc(argv[2],argv[3]);

   int sock;
   int n;
   struct sockaddr_in6 sin6;
   socklen_t sin6len;
   char buffer[MAXBUF];

   sock = socket(PF_INET6, SOCK_DGRAM,0);
   sin6len = sizeof(struct sockaddr_in6);
   memset(&sin6, 0, sin6len);

   sin6.sin6_port = htons(atoi(argv[1]));
   sin6.sin6_family = AF_INET6;
   sin6.sin6_addr = in6addr_any;

   n = bind(sock, (struct sockaddr *)&sin6, sin6len);
   if(-1 == n)
     perror("bind"), exit(1);

   //n = getsockname(sock, (struct sockaddr *)&sin6, &sin6len);
   //printf("%d\n", ntohs(sin6.sin6_port));

   while (1) {
      sleep(1);
      n = recvfrom(sock, buffer, MAXBUF, 0, (struct sockaddr *)&sin6, &sin6len);
      printf("n = %d, buffer : %s\n", n, buffer);
   }

   shutdown(sock, 2);
   close(sock);
   return 0;
}