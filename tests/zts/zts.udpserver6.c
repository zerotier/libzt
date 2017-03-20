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
#include <arpa/inet.h>
#include <fcntl.h>

#include "sdk.h"

#define MAXBUF 128

int main(int argc, char *argv[])
{
   if(argc < 3) {
     printf("usage: server <port> <netpath> <nwid>\n");
     return 1;
   }

   /* Starts ZeroTier core service in separate thread, loads user-space TCP/IP stack
   and sets up a private AF_UNIX socket between ZeroTier library and your app. Any 
   subsequent zts_* socket API calls (shown below) are mediated over this hidden AF_UNIX 
   socket and are spoofed to appear as AF_INET sockets. The implementation of this API
   is in src/sockets.c */
   zts_init_rpc(argv[2],argv[3]);

   int sock, n;
   struct sockaddr_in6 sin6;
   socklen_t sin6len;

   char buffer[MAXBUF];
   memset(buffer, 0, MAXBUF);

   sock = zts_socket(PF_INET6, SOCK_DGRAM,0);
   sin6len = sizeof(struct sockaddr_in6);
   memset(&sin6, 0, sin6len);

   sin6.sin6_port = htons(atoi(argv[1]));
   sin6.sin6_family = AF_INET6;
   sin6.sin6_addr = in6addr_any;

   n = zts_bind(sock, (struct sockaddr *)&sin6, sin6len);
   if(-1 == n)
     perror("bind"), exit(1);

   //n = getsockname(sock, (struct sockaddr *)&sin6, &sin6len);
   //printf("%d\n", ntohs(sin6.sin6_port));

   //fcntl(sock, F_SETFL, O_NONBLOCK);  
   while (1) {
      //usleep(50000);
      n = zts_recvfrom(sock, buffer, MAXBUF, 0, (struct sockaddr *)&sin6, &sin6len);
      //if(n > 0)
         printf("recvfrom(): n = %d, buffer : %s\n", n, buffer);
   }

   shutdown(sock, 2);
   close(sock);

   zts_stop(); /* Shut down ZT */
   return 0;
}