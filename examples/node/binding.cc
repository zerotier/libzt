#include <string>
#include <iostream>

#include "libzt.h"

struct ZT {
  static int running() {
    return zts_running();
  }

  static void simpleStart(const char *path, const char *nwid) {
    zts_simple_start(path, nwid);
  }

  static void stop() {
    zts_stop();
  }

  static char* getDeviceId() {
    char* id = new char [ZT_ID_LEN + 1];
    zts_get_device_id(id);
    return id;
  }

  static char* getIpV4Address(const char *nwid) {
    char* addr_str = new char [ZT_MAX_IPADDR_LEN];
    zts_get_ipv4_address(nwid, addr_str, ZT_MAX_IPADDR_LEN);
    return addr_str;
  }

  static int socket() {
    return zts_socket(AF_INET, SOCK_STREAM, 0);
  }

  static int bind(int sockfd, const char *addrStr, int port) {
		struct sockaddr_in addr;

    addr.sin_addr.s_addr = inet_addr(addrStr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons( port );

    return zts_bind(sockfd, (const struct sockaddr *)&addr, sizeof(addr));
  }

  static int listen(int sockfd) {
    return zts_listen(sockfd, 1);
  }

  static int accept(int sockfd) {
    struct sockaddr_in client;
    int c = sizeof(struct sockaddr_in);

    int accept_fd = zts_accept(sockfd, (struct sockaddr *)&client, (socklen_t*)&c);
    return accept_fd;
  }

};


#include "nbind/nbind.h"

NBIND_CLASS(ZT) {
  method(accept);
  method(bind);
  method(getDeviceId);
  method(getIpV4Address);
  method(running);
  method(simpleStart);
  method(socket);
  method(stop);
  method(listen);
}
