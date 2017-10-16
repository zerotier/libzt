package zerotier;

class ZeroTier {
  
  @native def start(path: String): Int
  @native def startjoin(path: String, nwid: String): Int
  @native def stop(): Unit
  @native def running(): Int
  @native def join(nwid: String): Unit
  @native def leave(nwid: String): Unit
  //@native def path():
  //@native def id(): Int
  //@native def get_6plane_addr(): Unit
  //@native def get_rfc4193_addr(): Unit
  @native def socket(socket_family: Int, socket_type: Int, protocol: Int): Int  
  @native def connect(fd: Int, addr: Object, addrlen: Int): Int
  @native def bind(fd: Int, addr: Object, addrlen: Int): Int
  @native def listen(fd: Int, backlog: Int): Int
  @native def accept(fd: Int, addr: Object, addrlen: Int): Int
  @native def accept4(fd: Int, addr: Object, addrlen: Int, flags: Int): Int
  @native def setsockopt(fd: Int, level: Int, optname: Int, optval: Object, optlen: Int): Int
  @native def getsockopt(fd: Int, level: Int, optname: Int, optval: Object, optlen: Int): Int
  //@native def getsockname(): Int
  //@native def getpeername(): Int
  //@native def gethostname(): Int
  //@native def sethostname(): Int
  //@native def gethostbyname(): Object
  @native def close(fd: Int): Int
  //@native def poll(): Int
  //@native def select(): Int
  @native def fcntl(fd: Int, cmd: Int, flags: Int): Int
  @native def ioctl(fd: Int, request: Long, argp: Object): Int
  @native def send(fd: Int, buf: Object, len: Int, flags: Int): Int
  @native def sendto(fd: Int, buf: Object, len: Int, addr: Object, addrlen: Int): Int
  @native def sendmsg(fd: Int, msg: Object, flags: Int): Int
  @native def recv(fd: Int, buf: Object, len: Int, flags: Int): Int
  @native def recvfrom(fd: Int, buf: Object, len: Int, addr: Object, addrlen: Int): Int
  @native def recvmsg(fd: Int, msg: Object, flags: Int): Int
  @native def read(fd: Int, buf: Object, len: Int): Int
  @native def write(fd: Int, buf: Object, len: Int): Int
  @native def shutdown(fd: Int, how: Int): Int
  //@native def add_dns(): Int
  //@native def del_dns(): Int
}
