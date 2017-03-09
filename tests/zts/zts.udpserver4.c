// UDP Server test program (IPV4)

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <netdb.h>
#include <cstdlib>

#include "sdk.h"

#define MAXBUF 1024*1024

void echo(int sock) {
  char bufin[MAXBUF];
  struct sockaddr_in remote;
  int n;
  socklen_t len = sizeof(remote);
  long count = 0;

  while(1) {
    sleep(1);
    count++;
    // read a datagram from the socket
    n=zts_recvfrom(sock,bufin,MAXBUF,0,(struct sockaddr *)&remote,&len);
    // print out the address of the sender
    printf("DGRAM from %s:%d\n", inet_ntoa(remote.sin_addr), ntohs(remote.sin_port));

    if (n<0) {
      perror("Error receiving data");
    } else {
      // sendto(sock,bufin,n,0,(struct sockaddr *)&remote,len);
      printf("RX (%d bytes) = %s\n", n, bufin);
    }
  }
}

int main(int argc, char *argv[]) {
  if(argc < 3) {
    printf("usage: client <port> <netpath> <nwid>\n");
    return 1;
  }

  /* Starts ZeroTier core service in separate thread, loads user-space TCP/IP stack
  and sets up a private AF_UNIX socket between ZeroTier library and your app. Any 
  subsequent zts_* socket API calls (shown below) are mediated over this hidden AF_UNIX 
  socket and are spoofed to appear as AF_INET sockets. The implementation of this API
  is in src/sockets.c */
  zts_init_rpc(argv[2],argv[3]);
  
  int sock, port = atoi(argv[1]);
  socklen_t len;
  struct sockaddr_in skaddr;
  struct sockaddr_in skaddr2;

  // Create socket
  if ((sock = zts_socket( PF_INET, SOCK_DGRAM, 0)) < 0) {
    printf("error creating socket\n");
    return 0;
  }
  // Create address
  skaddr.sin_family = AF_INET;
  skaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  skaddr.sin_port = htons(port);
  // Bind to address
  if (zts_bind(sock, (struct sockaddr *) &skaddr, sizeof(skaddr))<0) {
    printf("error binding\n");
    return 0;
  }
  // find out what port we were assigned
  len = sizeof( skaddr2 );
      
  // RX
  echo(sock);
  return 0;
}
