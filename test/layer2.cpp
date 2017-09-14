// This file is built with libzt.a via `make tests`

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#if defined(__APPLE__)
#include <net/ethernet.h>
#endif
#if defined(__linux__)
#include <netinet/ether.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#endif

#include "libzt.h"

unsigned short csum(unsigned short *buf, int nwords)
{
	unsigned long sum;
	for(sum=0; nwords>0; nwords--)
		sum += *buf++;
	sum = (sum >> 16) + (sum &0xffff);
	sum += (sum >> 16);
	return (unsigned short)(~sum);
}

int main(int argc , char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "usage: layer2 <zt_home_dir> <nwid>\n");
		return 1;
	}

	// initialize library
	printf("Starting libzt...\n");
	zts_simple_start(argv[1], argv[2]);
	char device_id[11];
	zts_get_device_id(device_id);
	fprintf(stderr, "Complete. I am %s\n", device_id);

	// create socket
	int fd;
	if ((fd = zts_socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) < 0) {
		printf("There was a problem creating the raw socket\n");
		exit(-1);
	}
	fprintf(stderr, "Created raw socket (%d)\n", fd);

#if defined(__APPLE__)
	fprintf(stderr, "SOCK_RAW not supported on mac builds yet. exiting");
	exit(0);
#endif
#if defined(__linux__) // The rest of this file isn't yet supported on non-linux platforms
	// get interface index to bind on
	struct ifreq if_idx;
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, "libzt0", IFNAMSIZ-1);
	if (zts_ioctl(fd, SIOCGIFINDEX, &if_idx) < 0) {
		perror("SIOCGIFINDEX");
		exit(-1);
	}
	fprintf(stderr, "if_idx.ifr_ifindex=%d\n", if_idx.ifr_ifindex);

	// get MAC address of interface to send on
	struct ifreq if_mac;
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, "libzt0", IFNAMSIZ-1);
	if (zts_ioctl(fd, SIOCGIFHWADDR, &if_mac) < 0) {
		perror("SIOCGIFHWADDR");
		exit(-1);
	}
	const unsigned char* mac=(unsigned char*)if_mac.ifr_hwaddr.sa_data;
	fprintf(stderr, "hwaddr=%02X:%02X:%02X:%02X:%02X:%02X\n", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

	// get IP address of interface to send on
	struct ifreq if_ip;
	memset(&if_ip, 0, sizeof(struct ifreq));
	strncpy(if_ip.ifr_name, "libzt0", IFNAMSIZ-1);
	if (zts_ioctl(fd, SIOCGIFADDR, &if_ip) < 0) {
		perror("SIOCGIFADDR");
		exit(-1);
	}
	char ipv4_str[INET_ADDRSTRLEN];
	struct sockaddr_in *in4 = (struct sockaddr_in *)&if_ip.ifr_addr;
	inet_ntop(AF_INET, (const void *)&in4->sin_addr.s_addr, ipv4_str, INET_ADDRSTRLEN);
	fprintf(stderr, "addr=%s", ipv4_str);

	// construct ethernet header
	int tx_len = 0;
	char sendbuf[1024];
	struct ether_header *eh = (struct ether_header *) sendbuf;
	memset(sendbuf, 0, 1024);
	
	// Ethernet header
	eh->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
	eh->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
	eh->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
	eh->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
	eh->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
	eh->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];

	// set destination MAC
	int MY_DEST_MAC0 = 0x72;
	int MY_DEST_MAC1 = 0x92;
	int MY_DEST_MAC2 = 0xd4;
	int MY_DEST_MAC3 = 0xfd;
	int MY_DEST_MAC4 = 0x43;
	int MY_DEST_MAC5 = 0x45;

	eh->ether_dhost[0] = MY_DEST_MAC0;
	eh->ether_dhost[1] = MY_DEST_MAC1;
	eh->ether_dhost[2] = MY_DEST_MAC2;
	eh->ether_dhost[3] = MY_DEST_MAC3;
	eh->ether_dhost[4] = MY_DEST_MAC4;
	eh->ether_dhost[5] = MY_DEST_MAC5;
	eh->ether_type = htons(ETH_P_IP);
	tx_len += sizeof(struct ether_header);


	// Construct the IP header
	int ttl = 64;
	struct iphdr *iph = (struct iphdr *) (sendbuf + sizeof(struct ether_header));
	iph->ihl = 5;
	iph->version = 4;
	iph->tos = 16; // Low delay
	iph->id = htons(54321);
	iph->ttl = ttl; // hops
	iph->protocol = 17; // UDP
	// Source IP address, can be spoofed
	iph->saddr = inet_addr(inet_ntoa(((struct sockaddr_in *)&if_ip.ifr_addr)->sin_addr));
	// iph->saddr = inet_addr("192.168.0.112");
	// Destination IP address
	iph->daddr = inet_addr("10.7.7.1");
	tx_len += sizeof(struct iphdr);

	// Construct UDP header
	struct udphdr *udph = (struct udphdr *) (sendbuf + sizeof(struct iphdr) + sizeof(struct ether_header));
	udph->source = htons(3423);
	udph->dest = htons(5342);
	udph->check = 0; // skip
	tx_len += sizeof(struct udphdr);

	// Fill in UDP payload
	sendbuf[tx_len++] = 0xde;
	sendbuf[tx_len++] = 0xad;
	sendbuf[tx_len++] = 0xbe;
	sendbuf[tx_len++] = 0xef;

	// Fill in remaining header info
	// Length of UDP payload and header
	udph->len = htons(tx_len - sizeof(struct ether_header) - sizeof(struct iphdr));
	// Length of IP payload and header
	iph->tot_len = htons(tx_len - sizeof(struct ether_header));
	// Calculate IP checksum on completed header
	iph->check = csum((unsigned short *)(sendbuf+sizeof(struct ether_header)), sizeof(struct iphdr)/2);

	// Send packet
	// Destination address
	struct sockaddr_ll socket_address;
	// Index of the network device
	socket_address.sll_ifindex = if_idx.ifr_ifindex;
	// Address length
	socket_address.sll_halen = ETH_ALEN;
	// Destination MAC
	socket_address.sll_addr[0] = MY_DEST_MAC0;
	socket_address.sll_addr[1] = MY_DEST_MAC1;
	socket_address.sll_addr[2] = MY_DEST_MAC2;
	socket_address.sll_addr[3] = MY_DEST_MAC3;
	socket_address.sll_addr[4] = MY_DEST_MAC4;
	socket_address.sll_addr[5] = MY_DEST_MAC5;

	while(1)
	{
		usleep(10000);
		// Send packet
		if (zts_sendto(fd, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
			fprintf(stderr, "Send failed\n");
	}

	// dismantle all zt virtual taps
	zts_stop();
#endif
	return 0;
}