[ZeroTier](https://www.zerotier.com) SDK
=====

Connect physical devices, virtual devices, and application instances as if everything is on a single LAN.

ZeroTier brings your network into user-space. We've paired our network hypervisor core with a TCP/UDP/IP stack [(lwIP)](https://en.wikipedia.org/wiki/LwIP) to provide your application with an exclusive and private virtual network interface. All traffic on this interface is end-to-end encrypted between each peer and we provide an easy-to-use socket interface similar to [Berkeley Sockets](https://en.wikipedia.org/wiki/Berkeley_sockets). Since we aren't using the kernel's IP stack that means no drivers, no root, and no host configuration requirements.

 - Website: https://www.zerotier.com/
 - ZeroTier Manual: https://www.zerotier.com/manual/
 - ZeroTier Repo: https://github.com/zerotier/zerotierone
 - SDK Repo: https://github.com/zerotier/libzt
 - Forum: https://discuss.zerotier.com

## 1.3.3-alpha.2 Release Notes

### New namespace structure:
- `ZeroTier.Core` (API to control a ZeroTier Node)
    - `class ZeroTier.Core.Node`
    - `class ZeroTier.Core.Event`
- `ZeroTier.Sockets`
    - `class ZeroTier.Sockets.Socket`
    - `class ZeroTier.Sockets.SocketException`
- `ZeroTier.Central` (upcoming)

### Added to Socket API
 - `Socket.ReceiveTimeout`, `Socket.SendTimeout`, etc.

### Bugs
 - Fixed memory leak caused by unmanaged resources not being released.