import zerotier.ZeroTier

object ExampleApp extends App {

  System.loadLibrary("zt")

  val libzt = new ZeroTier
  libzt.ztjni_startjoin("/Users/joseph/op/zt/libzt/ztjni", "1212121212121212")
  val fd = libzt.ztjni_socket(2, 1, 0)
  println(s"zts_socket(): $fd")
}