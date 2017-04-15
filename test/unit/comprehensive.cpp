// Comprehensive stress test for socket-like API

#include <stdio.h>

/****************************************************************************/
/* Test Functions                                                           */
/****************************************************************************/

int test_for_correctness()
{
	return 0;
}

int ipv4_udp_server()
{
	return 0;
}

int ipv6_udp_server()
{
	return 0;
}

int ipv4_tcp_server()
{
	return 0;
}

int ipv6_tcp_server()
{
	return 0;
}

/****************************************************************************/
/* main                                                                     */
/****************************************************************************/

int main()
{
	int test_all = 1;

	if(test_all)
	{
		printf("Testing API calls for correctness\n");
		test_for_correctness();

		printf("Testing as IPv4 UDP Server\n");
		ipv4_udp_server();

		printf("Testing as IPv6 UDP Server\n");
		ipv6_udp_server();

		printf("Testing as IPv4 TCP Server\n");
		ipv4_udp_server();

		printf("Testing as IPv6 TCP Server\n");
		ipv6_udp_server();

		printf("Testing \n");
		printf("\n");
		printf("\n");
	}
	return 0;
}