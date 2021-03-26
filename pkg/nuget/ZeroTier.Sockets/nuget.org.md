[ZeroTier](https://www.zerotier.com)
=====

Securely connect application instances, physical devices, and virtual devices as if everything is on a single LAN. ZeroTier brings your network into user-space. No root, and no host configuration requirements.

We've paired our network hyper-visor core with a TCP/UDP/IP stack [(lwIP)](https://en.wikipedia.org/wiki/LwIP) to provide your application with an exclusive and private virtual network interface. All traffic on this interface is end-to-end encrypted between each peer and we provide an easy-to-use socket interface similar to [Berkeley Sockets](https://en.wikipedia.org/wiki/Berkeley_sockets).

 - Website: https://www.zerotier.com/
 - ZeroTier Manual: https://www.zerotier.com/manual/
 - ZeroTier Repo: https://github.com/zerotier/zerotierone
 - SDK Repo: https://github.com/zerotier/libzt
 - Forum: https://discuss.zerotier.com

## 1.3.4 Release Notes

### Added:
 - IPv6 Support
 - `Socket.ReceiveTimeout`
 - `Socket.SendTimeout`
 - `Socket.ConnectTimeout`
 - `Socket.SendBufferSize`
 - `Socket.ReceiveBufferSize`
 - `Socket.Ttl`
 - `Socket.LingerState`
 - `Socket.KeepAlive`
 - `Socket.NoDelay`
 - `Socket.Blocking`

### Bugfixes:
 - Minor C API fixes
