#include <stdio.h>
#include <unistd.h>
#include <libgen.h>

#include "SDK.h"


int tcp_client(struct sockaddr_in *remote)
{
    printf("\t\t\tperforming TCP CLIENT test\n");

    if(argc < 3) {
        printf("usage: client <addr> <port>\n");
        return 1;
    }
   
    int sock, port = atoi(argv[2]);
    struct sockaddr_in server;
    char message[1000] , server_reply[2000];
     
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1) {
        printf("could not create socket");
    }     
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_family = AF_INET;
    server.sin_port = htons( port );
 
    printf("connecting...\n");
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
        perror("connect failed. Error");
        return 1;
    }
    printf("connected\n");

    char *msg = "welcome to the machine!";
    // TX         
    if(send(sock, msg, strlen(msg), 0) < 0) {
        printf("send failed");
        return 1;
    }
    else {
        printf("sent message: %s\n", msg);
        printf("len = %ld\n", strlen(msg));
    }
    close(sock);
}

int tcp_server(struct sockaddr_in *local)
{
    printf("\t\t\tperforming TCP SERVER test\n");
if(argc < 2) {
	   printf("usage: tcp_server <port>\n");
       return 0;
    }
   
    int socket_desc, client_sock, c, read_size, port = atoi(argv[1]);
    struct sockaddr_in server , client;
    char client_message[2000];
     
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1) {
        printf("could not create socket");
        return 0;
    }
     
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
     
    printf("binding on port %d\n", port);
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
        perror("bind failed. Error");
        return 0;
    }
    printf("listening\n");
    listen(socket_desc , 3); 
    printf("waiting to accept\n");
    c = sizeof(struct sockaddr_in);
    
    client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
    if (client_sock < 0) {
        perror("accept failed");
        return 0;
    }
    printf("connection accepted\n reading...\n");
     
    // RX

    int msglen = 1024;
    unsigned long count = 0;
    while(1)
    {
        count++;
       int bytes_read = read(client_sock, client_message, msglen);
       printf("[%lu] RX = (%d): ", count, bytes_read);
       for(int i=0; i<bytes_read; i++) {
          printf("%c", client_message[i]);
       }

       // TX
       int bytes_written = write(client_sock, "Server here!", 12); 
       printf("\t\nTX = %d\n", bytes_written);
    }
    return 0;
}

int udp_client(struct sockaddr_in *remote)
{
    printf("\t\t\tperforming UDP CLIENT test\n");
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
        usleep(10000);
        //bzero(buf, BUFSIZE);
        //printf("\nPlease enter msg: ");
        //fgets(buf, BUFSIZE, stdin);

        /* send the message to the server */
        serverlen = sizeof(serveraddr);
        printf("A\n");
        n = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&serveraddr, serverlen);
        printf("B\n");
        //if (n < 0) 
        //    error("ERROR in sendto");
        
        /* print the server's reply */
        printf("C\n");
        memset(buf, 0, sizeof(buf));
        printf("D\n");
        n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, (socklen_t *)&serverlen);
        printf("E\n");
        //if (n < 0) 
        //    printf("ERROR in recvfrom: %d", n);
        printf("Echo from server: %s", buf);
    }
    return 0;
}

int udp_server(struct sockaddr_in *local)
{
    printf("\t\t\tperforming UDP SERVER test\n");

}


int udp_tests(struct sockaddr_in *local, struct sockaddr_in *local) {
    printf("\t\tperforming UDP tests\n");
    udp_client(remote);
    udp_server(local);
}
int tcp_tests(struct sockaddr_in *local, struct sockaddr_in *local) {
    printf("\t\tperforming TCP tests\n");
    tcp_client(remote);
    tcp_server(local)
}
int test(struct sockaddr_in *local, struct sockaddr_in *local) {
    printf("\tperforming tests\n");
    udp_tests(local, remote);
    tcp_tests(local, remote);
}

int main()
{
    printf("Starting ZeroTier...\n");
    // where you want the zerotier identity/config files for this app to reside
    char *path = "/Users/Joseph/code/ZeroTierSDK/build/simple_app/zerotier";
    char *nwid = "565799d8f612388c";
    
    // start ZeroTier in separate thread
    pid_t pid = fork();
    if(pid)
        zt_start_service(path, nwid);
    
    printf("started!");

    //zt_join_network(nwid);
    //zt_leave_network(nwid);
    test(); // run unit tests

    return 0;
}