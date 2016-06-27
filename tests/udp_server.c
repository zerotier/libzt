// UDP Server test program

#include <stdio.h>      /* standard C i/o facilities */
#include <stdlib.h>     /* needed for atoi() */
#include <unistd.h>     /* defines STDIN_FILENO, system calls,etc */
#include <sys/types.h>  /* system data type definitions */
#include <sys/socket.h> /* socket specific definitions */
#include <netinet/in.h> /* INET constants and stuff */
#include <arpa/inet.h>  /* IP address conversion stuff */
#include <netdb.h>      /* gethostbyname */



/* this routine echos any messages (UDP datagrams) received */

#define MAXBUF 1024*1024

void echo( int sd ) {
  int len,n;
  char bufin[MAXBUF];
  struct sockaddr_in remote;

  /* need to know how big address struct is, len must be set before the
     call to recvfrom!!! */

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
  
  /*
  if(argc < 2) {
    printf("usage: udp_server <port>\n");
    exit(0);
  }

  int port = atoi(argv[1]);
  */

  int ld;
  struct sockaddr_in skaddr;
  int length;

  // create socket
  if ((ld = socket( PF_INET, SOCK_DGRAM, 0 )) < 0) {
    printf("Problem creating socket\n");
    exit(1);
  }

  // create address
  skaddr.sin_family = AF_INET;
  skaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  skaddr.sin_port = htons(0);

  // bind to address
  if (bind(ld, (struct sockaddr *) &skaddr, sizeof(skaddr))<0) {
    printf("error binding\n");
    exit(0);
  }

  /* find out what port we were assigned and print it out */

  length = sizeof( skaddr );
  if (getsockname(ld, (struct sockaddr *) &skaddr, &length)<0) {
    printf("error getsockname\n");
    exit(1);
  }
  printf("server UDP port = %d\n",ntohs(skaddr.sin_port));

  /* echo every datagram */
  echo(ld);
  return(0);
}
