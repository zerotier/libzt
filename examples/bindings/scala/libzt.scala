package zerotier;

class ZeroTier {
  
  @native def ztjni_start(path: String): Int
  @native def ztjni_startjoin(path: String, nwid: String): Int
  @native def ztjni_stop(): Unit
  @native def ztjni_running(): Int
  @native def ztjni_join(nwid: String): Unit
  @native def ztjni_leave(nwid: String): Unit
  //@native def ztjni_path():
  //@native def ztjni_id(): Int
  //@native def ztjni_get_6plane_addr(): Unit
  //@native def ztjni_get_rfc4193_addr(): Unit
  @native def ztjni_socket(socket_family: Int, socket_type: Int, protocol: Int): Int  
  @native def ztjni_connect(fd: Int, addr: Object, addrlen: Int): Int
  @native def ztjni_bind(fd: Int, addr: Object, addrlen: Int): Int
  @native def ztjni_listen(fd: Int, backlog: Int): Int
  @native def ztjni_accept(fd: Int, addr: Object, addrlen: Int): Int
  @native def ztjni_accept4(fd: Int, addr: Object, addrlen: Int, flags: Int): Int
  @native def ztjni_setsockopt(fd: Int, level: Int, optname: Int, optval: Object, optlen: Int): Int
  @native def ztjni_getsockopt(fd: Int, level: Int, optname: Int, optval: Object, optlen: Int): Int
  //@native def ztjni_getsockname(): Int
  //@native def ztjni_getpeername(): Int
  //@native def ztjni_gethostname(): Int
  //@native def ztjni_sethostname(): Int
  //@native def ztjni_gethostbyname(): Object
  @native def ztjni_close(fd: Int): Int
  //@native def ztjni_poll(): Int
  //@native def ztjni_select(): Int
  @native def ztjni_fcntl(fd: Int, cmd: Int, flags: Int): Int
  @native def ztjni_ioctl(fd: Int, request: Long, argp: Object): Int
  @native def ztjni_send(fd: Int, buf: Object, len: Int, flags: Int): Int
  @native def ztjni_sendto(fd: Int, buf: Object, len: Int, addr: Object, addrlen: Int): Int
  @native def ztjni_sendmsg(fd: Int, msg: Object, flags: Int): Int
  @native def ztjni_recv(fd: Int, buf: Object, len: Int, flags: Int): Int
  @native def ztjni_recvfrom(fd: Int, buf: Object, len: Int, addr: Object, addrlen: Int): Int
  @native def ztjni_recvmsg(fd: Int, msg: Object, flags: Int): Int
  @native def ztjni_read(fd: Int, buf: Object, len: Int): Int
  @native def ztjni_write(fd: Int, buf: Object, len: Int): Int
  @native def ztjni_shutdown(fd: Int, how: Int): Int
  //@native def ztjni_add_dns(): Int
  //@native def ztjni_del_dns(): Int
}
