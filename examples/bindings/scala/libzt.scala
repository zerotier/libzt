package zerotier;

class ZeroTier {
  @native def ztjni_socket(socket_family: Int, socket_type: Int, protocol: Int): Int
  @native def ztjni_startjoin(path: String, nwid: String): Int
}
