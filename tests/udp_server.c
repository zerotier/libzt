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

void echo( int sd ) {
  int len,n;
  char bufin[MAXBUF];
  struct sockaddr_in remote;
  len = sizeof(remote);
  long count = 0;

  while (1) {
    usleep(50);
    count++;
    
    /* read a datagram from the socket (put result in bufin) */
    n=recvfrom(sd,bufin,MAXBUF,0,(struct sockaddr *)&remote,&len);

    /* print out the address of the sender */
    printf("Got a datagram from %s port %d\n", inet_ntoa(remote.sin_addr), ntohs(remote.sin_port));

    if (n<0) {
      perror("Error receiving data");
    } else {
      printf("GOT %d BYTES (count = %d)\n",n, count);
      /* Got something, just send it back */
      //sendto(sd,bufin,n,0,(struct sockaddr *)&remote,len);
    }
  }
}

/* server main routine */

int main(int argc, char *argv[]) {
  printf("DGRAM = %d\n", SOCK_DGRAM);
    printf("STREAM = %d\n", SOCK_STREAM);

  /*
  if(argc < 2) {
    printf("usage: udp_server <port>\n");
    exit(0);
  }

  int port = atoi(argv[1]);
  */

  int ld;
  struct sockaddr_in skaddr;

    struct sockaddr_in skaddr2;

  int length;

  // create socket
  if ((ld = socket( PF_INET, SOCK_DGRAM, 0 )) < 0) {
    printf("Problem creating socket\n");
    exit(1);
  }

  // create address
  skaddr.sin_family = AF_INET;
  //skaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  skaddr.sin_addr.s_addr = inet_addr("10.5.5.2");
  skaddr.sin_port = htons(0);

  // bind to address
  if (bind(ld, (struct sockaddr *) &skaddr, sizeof(skaddr))<0) {
    printf("error binding\n");
    exit(0);
  }

  /* find out what port we were assigned and print it out */

  length = sizeof( skaddr2 );
  if (getsockname(ld, (struct sockaddr *) &skaddr2, &length)<0) {
    printf("error getsockname\n");
    exit(1);
  }

  int port = ntohs(skaddr2.sin_port);
  int ip = skaddr2.sin_addr.s_addr;
  unsigned char d[4];
  d[0] = ip & 0xFF;
  d[1] = (ip >>  8) & 0xFF;
  d[2] = (ip >> 16) & 0xFF;
  d[3] = (ip >> 24) & 0xFF;
  printf(" handleBind(): %d.%d.%d.%d : %d -> Assigned: %d\n", d[0],d[1],d[2],d[3], port);
          
  /* echo every datagram */
  echo(ld);
  return(0);
}
