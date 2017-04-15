// UDP Server test program

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <netdb.h>

#define MAXBUF 1024*1024

void echo(int sock) {
  char bufin[MAXBUF];
  struct sockaddr_in remote;
  int n;
  socklen_t len = sizeof(remote);
  long count = 0;

  while (1) {
    sleep(1);
    //usleep(50);
    count++;
    // read a datagram from the socket (put result in bufin)
    n=recvfrom(sock,bufin,MAXBUF,0,(struct sockaddr *)&remote,&len);
    // print out the address of the sender
    printf("DGRAM from %s:%d\n", inet_ntoa(remote.sin_addr), ntohs(remote.sin_port));

    if (n<0) {
      perror("Error receiving data");
    } else {
      printf("GOT %d BYTES (count = %ld)\n", n, count);
      // Got something, just send it back
      // sendto(sock,bufin,n,0,(struct sockaddr *)&remote,len);
      printf("RX = %s\n", bufin);
    }
  }
}

int main(int argc, char *argv[]) {

  if(argc < 2) {
    printf("usage: udp_server <port>\n");
    return 0;
  }
  int sock, port = atoi(argv[1]);
  socklen_t len;
  struct sockaddr_in skaddr;
  struct sockaddr_in skaddr2;

  // Create socket
  if ((sock = socket( PF_INET, SOCK_DGRAM, 0)) < 0) {
    printf("error creating socket\n");
    return 0;
  }
  // Create address
  skaddr.sin_family = AF_INET;
  skaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  skaddr.sin_port = htons(port);
  // Bind to address
  if (bind(sock, (struct sockaddr *) &skaddr, sizeof(skaddr))<0) {
    printf("error binding\n");
    return 0;
  }
  // find out what port we were assigned
  len = sizeof( skaddr2 );
  //if (getsockname(sock, (struct sockaddr *) &skaddr2, &len)<0) {
  //  printf("error getsockname\n");
  //  return 0;
  //}
  // Display address:port to verify it was sent over RPC correctly
  /*
  port = ntohs(skaddr2.sin_port);
  int ip = skaddr2.sin_addr.s_addr;
  unsigned char d[4];
  d[0] = ip & 0xFF;
  d[1] = (ip >>  8) & 0xFF;
  d[2] = (ip >> 16) & 0xFF;
  d[3] = (ip >> 24) & 0xFF;
  printf("bound to address: %d.%d.%d.%d : %d\n", d[0],d[1],d[2],d[3], port);
  */        
  // RX
  echo(sock);
  return(0);
}
