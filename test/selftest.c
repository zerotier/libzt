/**
 * Selftest. To be run for every commit.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include <ZeroTierSockets.h>

#pragma GCC diagnostic ignored "-Wunused-value"

int random32() {
	const int BITS_PER_RAND = (int)(log2(RAND_MAX/2 + 1) + 1.0);
	int ret = 0;
	for (int i = 0; i < sizeof(int) * CHAR_BIT; i += BITS_PER_RAND) {
		ret <<= BITS_PER_RAND;
		ret |= rand();
	}
	return ret;
}

uint64_t random64() {
	return ((uint64_t)random32() << 32) | random32();
}

void api_value_arg_test(int8_t i8, int16_t i16, int32_t i32, int64_t i64, void* nullable)
{
	//fprintf(stderr, "%d, %d, %d, %lld, %p\n", i8, i16, i32, i64, nullable);
	int res = ZTS_ERR_OK;

//------------------------------------------------------------------------------
// Test uninitialized Network Stack API usage                                  |
//------------------------------------------------------------------------------

	res = zts_get_all_stats((struct zts_stats *)nullable);
	assert(("pre-init call to zts_get_all_stats(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_get_protocol_stats(i32, nullable);
	assert(("pre-init call to zts_get_protocol_stats(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_dns_set_server(i8, (const zts_ip_addr *)nullable);
	assert(("pre-init call to zts_add_dns_nameserver(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	const zts_ip_addr *res_ptr = zts_dns_get_server(i8);
	assert(("pre-init call to zts_del_dns_nameserver(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));

//------------------------------------------------------------------------------
// Test uninitialized Node API usage                                           |
//------------------------------------------------------------------------------

	res = zts_stop();
	assert(("pre-init call to zts_stop(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_restart();
	assert(("pre-init call to zts_restart(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_free();
	assert(("pre-init call to zts_free(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_join(i64);
	assert(("pre-init call to zts_join(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_leave(i64);
	assert(("pre-init call to zts_leave(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_orbit(i64,i64);
	assert(("pre-init call to zts_orbit(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_deorbit(i64);
	assert(("pre-init call to zts_deorbit(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));

//------------------------------------------------------------------------------
// Test uninitialized Socket API usage                                         |
//------------------------------------------------------------------------------

	res = zts_socket(i32,i32,i32);
	assert(("pre-init call to zts_socket(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_connect(i32, (const struct zts_sockaddr *)nullable, i32);
	assert(("pre-init call to zts_connect(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_bind(i32, (const struct zts_sockaddr *)nullable, i32);
	assert(("pre-init call to zts_bind(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_listen(i32, i32);
	assert(("pre-init call to zts_listen(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_accept(i32, (struct zts_sockaddr *)nullable, (zts_socklen_t *)nullable);
	assert(("pre-init call to zts_accept(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_setsockopt(i32, i32, i32, nullable, i32);
	assert(("pre-init call to zts_setsockopt(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_getsockopt(i32, i32, i32, nullable, (zts_socklen_t *)nullable);
	assert(("pre-init call to zts_getsockopt(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_getsockname(i32, (struct zts_sockaddr *)nullable, (zts_socklen_t *)nullable);
	assert(("pre-init call to zts_getsockname(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_getpeername(i32, (struct zts_sockaddr *)nullable, (zts_socklen_t *)nullable);
	assert(("pre-init call to zts_getpeername(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_close(i32);
	assert(("pre-init call to zts_close(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_select(i32, (zts_fd_set *)nullable, (zts_fd_set *)nullable, (zts_fd_set *)nullable, (struct zts_timeval *)nullable);
	assert(("pre-init call to zts_select(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_fcntl(i32, i32, i32);
	assert(("pre-init call to zts_fcntl(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_poll((struct zts_pollfd *)nullable, i32, i32);
	assert(("pre-init call to zts_poll(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_ioctl(i32, i64, nullable);
	assert(("pre-init call to zts_ioctl(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_send(i32, nullable, i32, i32);
	assert(("pre-init call to zts_send(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_sendto(i32, nullable, i32, i32, (const struct zts_sockaddr *)nullable, i32);
	assert(("pre-init call to zts_sendto(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_sendmsg(i32, (const struct zts_msghdr *)nullable, i32);
	assert(("pre-init call to zts_sendmsg(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_recv(i32, nullable, i32, i32);
	assert(("pre-init call to zts_recv(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_recvfrom(i32, nullable, i32, i32, (struct zts_sockaddr *)nullable, (zts_socklen_t *)nullable);
	assert(("pre-init call to zts_recvfrom(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_recvmsg(i32, (struct zts_msghdr *)nullable, i32);
	assert(("pre-init call to zts_recvmsg(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_read(i32, nullable, i32);
	assert(("pre-init call to zts_read(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_readv(i32, (const struct zts_iovec *)nullable, i32);
	assert(("pre-init call to zts_readv(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_write(i32, nullable, i32);
	assert(("pre-init call to zts_write(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_writev(i32, (const struct zts_iovec *)nullable, i32);
	assert(("pre-init call to zts_writev(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_shutdown(i32, i32);
	assert(("pre-init call to zts_shutdown(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
}

int main()
{
	srand(time(NULL));

//------------------------------------------------------------------------------
// Test service-related API functions before initializing service              |
//------------------------------------------------------------------------------

	// Test null values
	api_value_arg_test(0,0,0,0,NULL);

	// Test wild values
	for (int i=0; i<1024; i++) {
		int8_t   i8 =  (uint8_t)random64();
		int16_t i16 = (uint16_t)random64();
		int32_t i32 = (uint32_t)random64();
		int64_t i64 = (uint64_t)random64();
		int x;
		void* nullable = &x;
		api_value_arg_test(i8,i16,i32,i64,nullable);
	}

//------------------------------------------------------------------------------
// Test non-service helper functions                                           |
//------------------------------------------------------------------------------

	// (B) Test zts_inet_ntop

	char ipstr[ZTS_INET6_ADDRSTRLEN];
	int16_t port = 0;
	struct zts_sockaddr_in in4;

	in4.sin_port = htons(8080);
#if defined(_WIN32)
	zts_inet_pton(ZTS_AF_INET, "192.168.22.1", &(in4.sin_addr.S_addr));
#else
	zts_inet_pton(ZTS_AF_INET, "192.168.22.1", &(in4.sin_addr.s_addr));
#endif

	in4.sin_family = ZTS_AF_INET;

	struct zts_sockaddr *sa = (struct zts_sockaddr *)&in4;
	if (sa->sa_family == ZTS_AF_INET) {
		struct zts_sockaddr_in *in4 = (struct zts_sockaddr_in*)sa;
		zts_inet_ntop(ZTS_AF_INET, &(in4->sin_addr),
			ipstr, ZTS_INET_ADDRSTRLEN);
		port = ntohs(in4->sin_port);
	}
	if (sa->sa_family == ZTS_AF_INET6) {
		struct zts_sockaddr_in6 *in6 = (struct zts_sockaddr_in6*)sa;
		zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr),
			ipstr, ZTS_INET6_ADDRSTRLEN);
	}

	assert(("zts_inet_ntop(): port != 8080", port == 8080));
	assert(("zts_inet_ntop(): strcmp(ipstr, \"192.168.22.1\") != 0",
		!strcmp(ipstr, "192.168.22.1")));

	// (C) Test zts_inet_pton

	uint8_t buf[sizeof(struct zts_in6_addr)];
	char str[ZTS_INET6_ADDRSTRLEN];

	zts_inet_pton(ZTS_AF_INET, "192.168.22.2", buf);
	zts_inet_ntop(ZTS_AF_INET, buf, str, ZTS_INET6_ADDRSTRLEN);
	assert(("zts_inet_pton(): strcmp(ipstr, \"192.168.22.2\") != 0",
		!strcmp(str, "192.168.22.2")));

//------------------------------------------------------------------------------
// Start the service and test a few things                                     |
//------------------------------------------------------------------------------

	// zts_start();

//------------------------------------------------------------------------------
// Test DNS client functionality                                               |
//------------------------------------------------------------------------------

/*
	// Set first nameserver

	char *ns1_addr_str = "FCC5:205E:4FF5:5311:DFF0::1";
	zts_ip_addr ns1;
	zts_ipaddr_aton(ns1_addr_str, &ns1);
	zts_dns_set_server(0, &ns1);

	// Get first nameserver

	const zts_ip_addr *ns1_result;
	ns1_result = zts_dns_get_server(0);
	printf("dns1 = %s\n", zts_ipaddr_ntoa(ns1_result));

	// Set second nameserver

	char *ns2_addr_str = "192.168.22.1";
	zts_ip_addr ns2;
	zts_ipaddr_aton(ns2_addr_str, &ns2);
	zts_dns_set_server(1, &ns2);

	// Get second nameserver

	const zts_ip_addr *ns2_result;
	ns2_result = zts_dns_get_server(1);
	printf("dns1 = %s\n", zts_ipaddr_ntoa(ns2_result));

	// Check that each nameserver address was properly set and get

	assert(("zts_dns_get_server(): Address mismatch", !strcmp(ns1_addr_str, zts_ipaddr_ntoa(ns1_result))));
	assert(("zts_dns_get_server(): Address mismatch", !strcmp(ns2_addr_str, zts_ipaddr_ntoa(ns2_result))));
*/

	return 0;
}
