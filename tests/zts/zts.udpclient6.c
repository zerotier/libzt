// UDP Client test program (IPV6)

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <cstdlib>
#include <fcntl.h>

#include "sdk.h"

#define MAXBUF 65536

int main(int argc, char* argv[])
{
  int status, sock, portno, n;
  struct addrinfo sainfo, *psinfo;
  struct hostent *server;
  char buffer[MAXBUF];
  struct sockaddr_in6 serv_addr;

  if(argc < 3) {
    printf("usage: client <addr> <port> <netpath> <nwid>\n");
    return 1;
  }

  /* Starts ZeroTier core service in separate thread, loads user-space TCP/IP stack
  and sets up a private AF_UNIX socket between ZeroTier library and your app. Any 
  subsequent zts_* socket API calls (shown below) are mediated over this hidden AF_UNIX 
  socket and are spoofed to appear as AF_INET sockets. The implementation of this API
  is in src/sockets.c */
  zts_init_rpc(argv[3],argv[4]);

  sock = zts_socket(AF_INET6, SOCK_DGRAM,0);
  portno = atoi(argv[2]);
  server = gethostbyname2(argv[1],AF_INET6);
  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin6_flowinfo = 0;
  serv_addr.sin6_family = AF_INET6;
  memmove((char *) &serv_addr.sin6_addr.s6_addr, (char *) server->h_addr, server->h_length);
  serv_addr.sin6_port = htons(portno);

  sprintf(buffer,"Ciao");

  fcntl(sock, F_SETFL, O_NONBLOCK);   
  while(1)
  {
    sleep(1);
    status = zts_sendto(sock, buffer, strlen(buffer), 0, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    printf("Sent : %s \t%d\n", buffer, status);
  }

  close(sock);
  return 0;
}