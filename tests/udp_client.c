/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h>

#define BUFSIZE 1024

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];

    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
      (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* get a message from the user */
    char *msg = "A message to the server!\0";
    fcntl(sockfd, F_SETFL, O_NONBLOCK); 
    long count = 0;
    while(1)
    {
        count++;
        printf("\nTX(%lu)...\n", count);
        usleep(100000);
        //bzero(buf, BUFSIZE);
        //printf("\nPlease enter msg: ");
        //fgets(buf, BUFSIZE, stdin);

        /* send the message to the server */
        serverlen = sizeof(serveraddr);
        printf("A\n");
        n = sendto(sockfd, msg, strlen(msg), 0, &serveraddr, serverlen);
        printf("B\n");
        //if (n < 0) 
        //    error("ERROR in sendto");
        
        /* print the server's reply */
        printf("C\n");
        memset(buf, 0, sizeof(buf));
        printf("D\n");
        n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
        printf("E\n");
        //if (n < 0) 
        //    printf("ERROR in recvfrom: %d", n);
        printf("Echo from server: %s", buf);
    }
    return 0;
}
