
class ZeroTier {
  @native def ztjni_socket(socket_family: Int, socket_type: Int, protocol: Int): Int
}

object ZeroTier extends App {

  System.loadLibrary("zt")

  val libzt = new ZeroTier
  val fd = libzt.ztjni_socket(2, 1, 0)
  println(s"zts_socket(): $fd")
}