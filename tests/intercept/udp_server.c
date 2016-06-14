#include <stdio.h>      /* standard C i/o facilities */
#include <stdlib.h>     /* needed for atoi() */
#include <unistd.h>     /* defines STDIN_FILENO, system calls,etc */
#include <sys/types.h>  /* system data type definitions */
#include <sys/socket.h> /* socket specific definitions */
#include <netinet/in.h> /* INET constants and stuff */
#include <arpa/inet.h>  /* IP address conversion stuff */
#include <netdb.h>      /* gethostbyname */

/* this routine echos any messages (UDP datagrams) received */

#define MAXBUF 1024*10

void echo( int sd ) {
  int len,n;
  char bufin[MAXBUF];
  struct sockaddr_in remote;

  /* need to know how big address struct is, len must be set before the
     call to recvfrom!!! */
  len = sizeof(remote);
  long count = 0;

  while (1) {
    sleep(1);
    count++;

    n=recvfrom(sd,bufin,MAXBUF,0,(struct sockaddr *)&remote,&len);

    /* print out the address of the sender */
    printf("Got a datagram from %s port %d\n",
           inet_ntoa(remote.sin_addr), ntohs(remote.sin_port));

    if (n<0) {
      perror("Error receiving data");
    } else {
      printf("GOT %d BYTES (count = %d)\n",n, count);
      sendto(sd,bufin,n,0,(struct sockaddr *)&remote,len);
    }
  }
}

int main() {
  int ld, length;
  struct sockaddr_in skaddr;

  if ((ld = socket( PF_INET, SOCK_DGRAM, 0 )) < 0) {
    printf("Problem creating socket\n");
    exit(1);
  }

  skaddr.sin_family = AF_INET;
  skaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  skaddr.sin_port = htons(8888);

  if (bind(ld, (struct sockaddr *) &skaddr, sizeof(skaddr))<0) {
    printf("Problem binding\n");
    exit(0);
  }
/*
  length = sizeof( skaddr );
  if (getsockname(ld, (struct sockaddr *) &skaddr, &length)<0) {
    printf("Error getsockname\n");
    exit(1);
  }
  */
  printf("The server UDP port number is %d\n",ntohs(skaddr.sin_port));
  /* Go echo every datagram we get */
  echo(ld);
  return 0;
}
