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
#include <time.h>

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

int is_online = 0;
int has_ip4 = 0;
int has_ip6 = 0;

//----------------------------------------------------------------------------//
// Event Handler                                                              //
//----------------------------------------------------------------------------//

void on_zts_event(void *msgPtr)
{
	struct zts_callback_msg *msg = (struct zts_callback_msg *)msgPtr;
	fprintf(stderr, "event=%d\n", msg->eventCode);
	if (msg->eventCode == ZTS_EVENT_NODE_ONLINE) {
		fprintf(stderr, "ZTS_EVENT_NODE_ONLINE\n");
		is_online = 1;
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_READY_IP4) {
		fprintf(stderr, "ZTS_EVENT_NETWORK_READY_IP4\n");
		has_ip4 = 1;
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_READY_IP6) {
		fprintf(stderr, "ZTS_EVENT_NETWORK_READY_IP6\n");
		has_ip6 = 1;
	}
}

void api_value_arg_test(
	int8_t i8, int16_t i16, int32_t i32, int64_t i64, void* nullable)
{
	//fprintf(stderr, "%d, %d, %d, %lld, %p\n", i8, i16, i32, i64, nullable);
	int res = ZTS_ERR_OK;

//----------------------------------------------------------------------------//
// Test uninitialized Network Stack API usage                                 //
//----------------------------------------------------------------------------//
/*
	res = zts_get_all_stats((struct zts_stats *)nullable);
	assert(("pre-init call to zts_get_all_stats(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	res = zts_get_protocol_stats(i32, nullable);
	assert(("pre-init call to zts_get_protocol_stats(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
*/
	res = zts_dns_set_server(i8, (const zts_ip_addr *)nullable);
	assert(("pre-init call to zts_add_dns_nameserver(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));
	const zts_ip_addr *res_ptr = zts_dns_get_server(i8);
	assert(("pre-init call to zts_del_dns_nameserver(): res != ZTS_ERR_SERVICE",
		res == ZTS_ERR_SERVICE));

//----------------------------------------------------------------------------//
// Test uninitialized Node API usage                                          //
//----------------------------------------------------------------------------//

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

//----------------------------------------------------------------------------//
// Test uninitialized Socket API usage                                        //
//----------------------------------------------------------------------------//

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

void test_pre_service()
{

//----------------------------------------------------------------------------//
// Test service-related API functions before initializing service             //
//----------------------------------------------------------------------------//

	// Test null values
	api_value_arg_test(0,0,0,0,NULL);

	// Test wild values
	for (int i=0; i<4096; i++) {
		int8_t   i8 =  (uint8_t)random64();
		int16_t i16 = (uint16_t)random64();
		int32_t i32 = (uint32_t)random64();
		int64_t i64 = (uint64_t)random64();
		int x;
		void* nullable = &x;
		api_value_arg_test(i8,i16,i32,i64,nullable);
	}

//----------------------------------------------------------------------------//
// Test non-service helper functions                                          //
//----------------------------------------------------------------------------//

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
}

void test_service()
{
	int res = ZTS_ERR_OK;

//----------------------------------------------------------------------------//
// Test simplified API, proxy for setsockopt/getsockopt/ioctl etc             //
//----------------------------------------------------------------------------//

	int s4 = zts_socket(ZTS_AF_INET6, ZTS_SOCK_STREAM, 0);
	assert(s4 >= 0);

	// TCP_NODELAY

	// Check value before doing anything
	res = zts_get_no_delay(s4);
	assert(res == 0);
	// Turn on
	res = zts_set_no_delay(s4, 1);
	assert(res == ZTS_ERR_OK);
	res = zts_get_no_delay(s4);
	// Should return value instead of error code
	assert(res == 1);
	// Turn off
	res = zts_set_no_delay(s4, 0);
	assert(res == ZTS_ERR_OK);
	res = zts_get_no_delay(s4);
	assert(res == ZTS_ERR_OK);
	assert(res == 0);

	// SO_LINGER

	// Check value before doing anything
	res = zts_get_linger_enabled(s4);
	assert(res == 0);
	res = zts_get_linger_value(s4);
	assert(res == 0);
	// Turn on, set to 7 seconds
	res = zts_set_linger(s4, 1, 7);
	res = zts_get_linger_enabled(s4);
	assert(res == 1);
	res = zts_get_linger_value(s4);
	assert(res == 7);
	res = zts_set_linger(s4, 0, 0);
	// Turn off
	res = zts_get_linger_enabled(s4);
	assert(res == 0);
	res = zts_get_linger_value(s4);
	assert(res == 0);

	// SO_REUSEADDR

	// Check value before doing anything
	res = zts_get_reuse_addr(s4);
	assert(res == 0);
	// Turn on
	res = zts_set_reuse_addr(s4, 1);
	assert(res == ZTS_ERR_OK);
	res = zts_get_reuse_addr(s4);
	// Should return value instead of error code
	assert(res == 1);
	// Turn off
	res = zts_set_reuse_addr(s4, 0);
	assert(res == ZTS_ERR_OK);
	res = zts_get_reuse_addr(s4);
	assert(res == ZTS_ERR_OK);
	assert(res == 0);

	// SO_RCVTIMEO

	// Check value before doing anything
	res = zts_get_recv_timeout(s4);
	assert(res == 0);
	// Set to value
	res = zts_set_recv_timeout(s4, 3, 0);
	res = zts_get_recv_timeout(s4);
	assert(res == 3);
	res = zts_set_recv_timeout(s4, 0, 0);
	// Set to zero
	res = zts_get_recv_timeout(s4);
	assert(res == 0);

	// SO_SNDTIMEO

	// Check value before doing anything
	res = zts_get_send_timeout(s4);
	assert(res == 0);
	// Set to value
	res = zts_set_send_timeout(s4, 4, 0);
	res = zts_get_send_timeout(s4);
	assert(res == 4);
	res = zts_set_send_timeout(s4, 0, 0);
	// Set to zero
	res = zts_get_send_timeout(s4);
	assert(res == 0);

	// SO_SNDBUF

	// Check value before doing anything
	res = zts_get_send_buf_size(s4);
	assert(res == -1); // Unimplemented as of writing of test
	// Set to 7 seconds
	res = zts_set_send_buf_size(s4, 1024);
	res = zts_get_send_buf_size(s4);
	assert(res == -1); // Unimplemented as of writing of test
	res = zts_set_send_buf_size(s4, 0);
	// Set to zero
	res = zts_get_send_buf_size(s4);
	assert(res == -1); // Unimplemented as of writing of test

	// SO_RCVBUF

	// Check value before doing anything
	res = zts_get_recv_buf_size(s4);
	assert(res > 0);
	// Set to value
	res = zts_set_recv_buf_size(s4, 1024);
	res = zts_get_recv_buf_size(s4);
	assert(res == 1024);
	res = zts_set_recv_buf_size(s4, 0);
	// Set to zero
	res = zts_get_recv_buf_size(s4);
	assert(res == 0);

	// IP_TTL

	// Check value before doing anything
	res = zts_get_ttl(s4);
	assert(res == 255); // Defaults to max
	// Set to value
	res = zts_set_ttl(s4, 128);
	res = zts_get_ttl(s4);
	assert(res == 128);
	res = zts_set_ttl(s4, 0);
	// Set to zero
	res = zts_get_ttl(s4);
	assert(res == 0);

	// O_NONBLOCK

	// Check value before doing anything
	res = zts_get_blocking(s4);
	assert(res == 1);
	// Turn off (non-blocking)
	res = zts_set_blocking(s4, 0);
	assert(res == ZTS_ERR_OK);
	res = zts_get_blocking(s4);
	// Should return value instead of error code
	assert(res == 0);
	// Turn off
	res = zts_set_blocking(s4, 1);
	assert(res == ZTS_ERR_OK);
	res = zts_get_blocking(s4);
	assert(res == 1);

	// SO_KEEPALIVE

	// Check value before doing anything
	res = zts_get_keepalive(s4);
	assert(res == 0);
	// Turn on
	res = zts_set_keepalive(s4, 1);
	assert(res == ZTS_ERR_OK);
	res = zts_get_keepalive(s4);
	// Should return value instead of error code
	assert(res == 1);
	// Turn off
	res = zts_set_keepalive(s4, 0);
	assert(res == ZTS_ERR_OK);
	res = zts_get_keepalive(s4);
	assert(res == ZTS_ERR_OK);
	assert(res == 0);

//----------------------------------------------------------------------------//
// Test DNS client functionality                                              //
//----------------------------------------------------------------------------//

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

//----------------------------------------------------------------------------//
// Test shutting down the service                                             //
//----------------------------------------------------------------------------//

	zts_stop();
	s4 = zts_socket(ZTS_AF_INET, ZTS_SOCK_STREAM, 0);
	assert(("s4 != ZTS_ERR_SERVICE, not shut down", s4 == ZTS_ERR_SERVICE));
}

//----------------------------------------------------------------------------//
// Server                                                                     //
//----------------------------------------------------------------------------//

#define MAX_CONNECT_TIME 60 // outer re-attempt loop
#define CONNECT_TIMEOUT 30 // zts_connect_easy, ms
#define BUFLEN 128
char *msg = "welcome to the machine";

void start_server_app(uint16_t port4, uint16_t port6)
{
	int err = ZTS_ERR_OK;
	int bytes_read = 0;
	int bytes_sent = 0;

	int msglen = strlen(msg);
	char dstbuf[BUFLEN];
	int buflen = BUFLEN;

	struct timespec start, now;
	int time_diff = 0;

	//
	// IPv4 test
	//

	fprintf(stderr, "server4: will listen on: 0.0.0.0:%d\n", port4);
	int s4 = zts_socket(ZTS_AF_INET, ZTS_SOCK_STREAM, 0);
	assert(s4 == 0 && zts_errno == 0);

	err = zts_bind_easy(s4, ZTS_AF_INET, "0.0.0.0", port4);
	assert(err == ZTS_ERR_OK && zts_errno == 0);

	err = zts_listen(s4, 1);
	assert(err == ZTS_ERR_OK && zts_errno == 0);

	struct zts_sockaddr_in in4;
	zts_socklen_t addrlen4 = sizeof(in4);

	int acc4 = -1;
	clock_gettime(CLOCK_MONOTONIC, &start);
	do {
		fprintf(stderr, "server4: accepting...\n");
		acc4 = zts_accept(s4, &in4, &addrlen4);
		zts_delay_ms(250);
		clock_gettime(CLOCK_MONOTONIC, &now);
		time_diff = (now.tv_sec - start.tv_sec);
	} while (err < 0 && time_diff < MAX_CONNECT_TIME);

	assert(acc4 == 1 && zts_errno == 0);

	// Read message
	memset(dstbuf, 0, buflen);
	bytes_read = zts_read(acc4, dstbuf, buflen);
	fprintf(stderr, "server4: read (%d) bytes\n", bytes_read);
	assert(bytes_read == msglen && zts_errno == 0);

	// Send message
	bytes_sent = zts_write(acc4, msg, msglen);
	fprintf(stderr, "server4: wrote (%d) bytes\n", bytes_sent);
	assert(bytes_sent == msglen && zts_errno == 0);

	zts_close(s4);
	assert(err == ZTS_ERR_OK && zts_errno == 0);

	zts_close(acc4);
	assert(err == ZTS_ERR_OK && zts_errno == 0);

	assert(bytes_sent == bytes_read);
	if (bytes_sent == bytes_read) {
		fprintf(stderr, "server4: Test OK\n");
	} else {
		fprintf(stderr, "server4: Test FAIL\n");
	}

	//
	// IPv6 test
	//

	fprintf(stderr, "server: will listen on: [::]:%d\n", port6);
	int s6 = zts_socket(ZTS_AF_INET6, ZTS_SOCK_STREAM, 0);
	assert(s6 == 0 && zts_errno == 0);

	err = zts_bind_easy(s6, ZTS_AF_INET6, "::", port6);
	assert(err == ZTS_ERR_OK && zts_errno == 0);

	err = zts_listen(s6, 1);
	assert(err == ZTS_ERR_OK && zts_errno == 0);

	struct zts_sockaddr_in6 in6;
	zts_socklen_t addrlen6 = sizeof(in6);

	int acc6 = -1;
	clock_gettime(CLOCK_MONOTONIC, &start);
	do {
		fprintf(stderr, "server6: accepting...\n");
		acc6 = zts_accept(s6, &in6, &addrlen6);
		zts_delay_ms(250);
		clock_gettime(CLOCK_MONOTONIC, &now);
		time_diff = (now.tv_sec - start.tv_sec);
	} while (err < 0 && time_diff < MAX_CONNECT_TIME);

	fprintf(stderr, "server6: accepted connection (fd=%d)\n", acc6);
	assert(acc6 == 1 && zts_errno == 0);

	// Read message
	memset(dstbuf, 0, buflen);
	bytes_read = zts_read(acc6, dstbuf, buflen);
	fprintf(stderr, "server6: read (%d) bytes\n", bytes_read);
	assert(bytes_read == msglen && zts_errno == 0);

	// Send message
	bytes_sent = zts_write(acc6, msg, msglen);
	fprintf(stderr, "server6: wrote (%d) bytes\n", bytes_sent);
	assert(bytes_sent == msglen && zts_errno == 0);

	zts_close(s6);
	assert(err == ZTS_ERR_OK && zts_errno == 0);

	zts_close(acc6);
	assert(err == ZTS_ERR_OK && zts_errno == 0);

	zts_stop();
	assert(err == ZTS_ERR_OK && zts_errno == 0);
	int s = zts_socket(ZTS_AF_INET, ZTS_SOCK_STREAM, 0);
	assert(("s != ZTS_ERR_SERVICE, not shut down", s == ZTS_ERR_SERVICE));

	assert(bytes_sent == bytes_read);
	if (bytes_sent == bytes_read) {
		fprintf(stderr, "server6: Test OK\n");
	} else {
		fprintf(stderr, "server6: Test FAIL\n");
	}
}

//----------------------------------------------------------------------------//
// Client                                                                     //
//----------------------------------------------------------------------------//

void start_client_app(char *ip4, uint16_t port4, char *ip6, uint16_t port6)
{
	int err = ZTS_ERR_OK;
	int bytes_read = 0;
	int bytes_sent = 0;

	int msglen = strlen(msg);
	char dstbuf[BUFLEN];
	int buflen = BUFLEN;

	struct timespec start, now;
	int time_diff = 0;

	//
	// IPv4 test
	//

	err = ZTS_ERR_OK;
	int s4 = zts_socket(ZTS_AF_INET, ZTS_SOCK_STREAM, 0);
	assert(err == ZTS_ERR_OK && zts_errno == 0);

	zts_set_blocking(s4, 1);
	assert(err == ZTS_ERR_OK && zts_errno == 0);

	clock_gettime(CLOCK_MONOTONIC, &start);
	do {
		fprintf(stderr, "client4: connecting to: %s:%d\n", ip4, port4);
		err = zts_connect_easy(s4, ZTS_AF_INET, ip4, port4, CONNECT_TIMEOUT);
		zts_delay_ms(500);
		clock_gettime(CLOCK_MONOTONIC, &now);
		time_diff = (now.tv_sec - start.tv_sec);
	} while (err < 0 && time_diff < MAX_CONNECT_TIME);
	assert(err == ZTS_ERR_OK && zts_errno == 0);

	fprintf(stderr, "client4: connected\n");
	// Send message
	bytes_sent = zts_write(s4, msg, msglen);
	fprintf(stderr, "client4: wrote (%d) bytes\n", bytes_sent);
	assert(bytes_sent == msglen && zts_errno == 0);

	// Read message
	memset(dstbuf, 0, buflen);
	bytes_read = zts_read(s4, dstbuf, buflen);
	assert(bytes_read == msglen && zts_errno == 0);

	fprintf(stderr, "client4: read (%d) bytes\n", bytes_read);
	assert(bytes_sent == bytes_read && zts_errno == 0);

	zts_close(s4);
	assert(err == ZTS_ERR_OK && zts_errno == 0);

	assert(bytes_sent == bytes_read);
	if (bytes_sent == bytes_read) {
		fprintf(stderr, "client4: Test OK\n");
	} else {
		fprintf(stderr, "client4: Test FAIL\n");
	}

	//
	// IPv6 test
	//

	err = ZTS_ERR_OK;
	int s6 = zts_socket(ZTS_AF_INET6, ZTS_SOCK_STREAM, 0);
	assert(err == ZTS_ERR_OK && zts_errno == 0);

	zts_set_blocking(s6, 1);
	assert(err == ZTS_ERR_OK && zts_errno == 0);

	clock_gettime(CLOCK_MONOTONIC, &start);
	do {
		fprintf(stderr, "client6: connecting to: %s:%d\n", ip6, port6);
		err = zts_connect_easy(s6, ZTS_AF_INET6, ip6, port6, CONNECT_TIMEOUT);
		zts_delay_ms(500);
		clock_gettime(CLOCK_MONOTONIC, &now);
		time_diff = (now.tv_sec - start.tv_sec);
	} while (err < 0 && time_diff < MAX_CONNECT_TIME);
	assert(err == ZTS_ERR_OK && zts_errno == 0);

	fprintf(stderr, "client6: connected\n");
	// Send message
	bytes_sent = zts_write(s6, msg, msglen);
	fprintf(stderr, "client6: wrote (%d) bytes\n", bytes_sent);
	assert(bytes_sent == msglen && zts_errno == 0);

	// Read message
	memset(dstbuf, 0, buflen);
	bytes_read = zts_read(s6, dstbuf, buflen);
	assert(bytes_read == msglen && zts_errno == 0);

	fprintf(stderr, "client6: read (%d) bytes\n", bytes_read);
	assert(bytes_sent == bytes_read && zts_errno == 0);

	zts_close(s6);
	assert(err == ZTS_ERR_OK && zts_errno == 0);

	zts_stop();
	assert(err == ZTS_ERR_OK && zts_errno == 0);
	int s = zts_socket(ZTS_AF_INET, ZTS_SOCK_STREAM, 0);
	assert(("s != ZTS_ERR_SERVICE, not shut down", s == ZTS_ERR_SERVICE));

	assert(bytes_sent == bytes_read);
	if (bytes_sent == bytes_read) {
		fprintf(stderr, "client6: Test OK\n");
	} else {
		fprintf(stderr, "client6: Test FAIL\n");
	}
}

//----------------------------------------------------------------------------//
// Start node                                                                 //
//----------------------------------------------------------------------------//

void start_node(char *path, uint64_t nwid)
{
	struct timespec start, now;
	int time_diff = 0;

	fprintf(stderr, "starting node...\n");
	clock_gettime(CLOCK_MONOTONIC, &start);
	int res = zts_start(path, &on_zts_event, 0);
	assert(("error starting service: res != ZTS_ERR_OK", res == ZTS_ERR_OK));
	do {
		zts_delay_ms(25);
		clock_gettime(CLOCK_MONOTONIC, &now);
		time_diff = (now.tv_sec - start.tv_sec);
	} while (!is_online && (time_diff < MAX_CONNECT_TIME));
	if (!is_online) {
		fprintf(stderr, "node failed to come online\n");
		exit(-1);
	}

	fprintf(stderr, "joining: %llx\n", nwid);
	clock_gettime(CLOCK_MONOTONIC, &start);
	if (nwid) {
		zts_join(nwid);
		do {
			zts_delay_ms(25);
			clock_gettime(CLOCK_MONOTONIC, &now);
			time_diff = (now.tv_sec - start.tv_sec);
		} while ((!has_ip4 || !has_ip6) && (time_diff < MAX_CONNECT_TIME));
		if (!has_ip4 || !has_ip6) {
			fprintf(stderr, "node failed to receive assigned addresses\n");
			exit(-1);
		}
	}
}

//----------------------------------------------------------------------------//
// Main                                                                       //
//----------------------------------------------------------------------------//

int main(int argc, char **argv)
{
	if (argc != 1 && argc != 5 && argc != 7) {
		fprintf(stderr, "Invalid number of arguments.\n");
		exit(-1);
	}

	//
	// API fuzz test
	//

	test_pre_service();

	//
	// Default test
	//

	// selftest
	if (argc == 1) {
		srand(time(NULL));
		// Store identities in cwd, join 0x0
		start_node(".",0x0);
		test_service();
		exit(0);
	}

	// Default test (single node)
	// selftest <id-path>
	/*
	if (argc == 2) {
		srand(time(NULL));
		start_node(argv[1],0x0);
		test_service();
		exit(0);
	}*/

	//
	// Client/Server communication test
	//

	// Server test
	if (argc == 5) {
		//fprintf(stderr, "server.path  = %s\n", argv[1]);
		//fprintf(stderr, "server.nwid  = %s\n", argv[2]);
		//fprintf(stderr, "server.port4 = %s\n", argv[3]);
		//fprintf(stderr, "server.port6 = %s\n", argv[4]);
		uint64_t nwid = strtoull(argv[2],NULL,16);
		int port4 = atoi(argv[3]);
		int port6 = atoi(argv[4]);
		start_node(argv[1],nwid);
		start_server_app(port4, port6);
		exit(0);
	}
	// Client test
	if (argc == 7) {
		//fprintf(stderr, "client.path  = %s\n", argv[1]);
		//fprintf(stderr, "client.nwid  = %s\n", argv[2]);
		//fprintf(stderr, "client.port4 = %s\n", argv[3]);
		//fprintf(stderr, "client.ip4   = %s\n", argv[4]);
		//fprintf(stderr, "client.port6 = %s\n", argv[5]);
		//fprintf(stderr, "client.ip6   = %s\n", argv[6]);
		uint64_t nwid = strtoull(argv[2],NULL,16);
		int port4 = atoi(argv[3]);
		int port6 = atoi(argv[5]);
		start_node(argv[1],nwid);
		start_client_app(argv[4], port4, argv[6], port6);
		exit(0);
	}
	return 0;
}
