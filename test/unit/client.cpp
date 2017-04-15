// Comprehensive stress test for socket-like API

#include <stdio.h>

/****************************************************************************/
/* Test Functions                                                           */
/****************************************************************************/

int test_for_correctness()
{
	return 0;
}

int ipv4_udp_client()
{
	return 0;
}

int ipv6_udp_client()
{
	return 0;
}

int ipv4_tcp_client()
{
	return 0;
}

int ipv6_tcp_client()
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

		printf("Testing as IPv4 UDP Client\n");
		ipv4_udp_client();

		printf("Testing as IPv6 UDP Client\n");
		ipv6_udp_client();

		printf("Testing as IPv4 TCP Client\n");
		ipv4_udp_client();

		printf("Testing as IPv6 TCP Client\n");
		ipv6_udp_client();

		printf("Testing \n");
		printf("\n");
		printf("\n");
	}
	return 0;
}