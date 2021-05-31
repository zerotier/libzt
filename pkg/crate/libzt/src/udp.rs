/*
 * Copyright (c)2013-2021 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2026-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

use std::convert::TryInto;
use std::ffi::{c_void, CString};
use std::io::{self /*, Error, ErrorKind*/};
use std::net::{/*Ipv4Addr, Ipv6Addr,*/ SocketAddr, ToSocketAddrs};
use std::os::raw::c_int;
use std::time::Duration;
//use std::cmp;

use crate::socket::Socket;
use crate::utils::*;

//----------------------------------------------------------------------------//
// UdpSocketImpl                                                              //
//----------------------------------------------------------------------------//

pub struct UdpSocketImpl {
    inner: Socket,
}

impl UdpSocketImpl {
    pub fn bind(addr: io::Result<&SocketAddr>) -> io::Result<UdpSocketImpl> {
        let addr = addr?;
        let socket = Socket::new(addr, ZTS_SOCK_DGRAM as i32)?;
        // TODO: Possibly set SO_REUSEADDR
        //let (addrp, len) = addr.into_inner();
        unsafe {
            //zts_bsd_bind(*socket.as_inner(), addrp, len as _);
            // TODO: Find a better way to split this address string
            let full_str = addr.to_string();
            let full_vec = full_str.split(":");
            let lvec: Vec<&str> = full_vec.collect();
            let addr_str = lvec[0];
            let port = addr.port();
            // TODO: Handle native error code, consider cvt?
            // This is a false-positive by the linter
            // See: https://github.com/rust-lang/rust/issues/78691
            #[allow(temporary_cstring_as_ptr)]
            zts_bind(
                *socket.as_inner(),
                CString::new(addr_str).unwrap().as_ptr(),
                port,
            );
        }
        Ok(UdpSocketImpl { inner: socket })
    }

    pub fn socket(&self) -> &Socket {
        &self.inner
    }

    pub fn into_socket(self) -> Socket {
        self.inner
    }

    pub fn peer_addr(&self) -> io::Result<SocketAddr> {
        sockname(|buf, len| unsafe { zts_bsd_getpeername(*self.inner.as_inner(), buf, len) })
    }

    pub fn socket_addr(&self) -> io::Result<SocketAddr> {
        sockname(|buf, len| unsafe { zts_bsd_getsockname(*self.inner.as_inner(), buf, len) })
    }

    pub fn recv_from(&self, buf: &mut [u8]) -> io::Result<(usize, SocketAddr)> {
        self.inner.recv_from(buf)
    }

    pub fn peek_from(&self, buf: &mut [u8]) -> io::Result<(usize, SocketAddr)> {
        self.inner.peek_from(buf)
    }

    /*
        pub fn send_to(&self, buf: &[u8], dst: &SocketAddr) -> io::Result<usize> {
            let len = cmp::min(buf.len(), <size_t>::MAX as usize) as size_t;
            let (dstp, dstlen) = dst.into_inner();
            let ret = cvt(unsafe {
                zts_bsd_sendto(
                    *self.inner.as_inner(),
                    buf.as_ptr() as *const c_void,
                    len,
                    ZTS_MSG_NOSIGNAL,
                    dstp,
                    dstlen,
                )
            })?;
            Ok(ret as usize)
        }
    */

    pub fn set_read_timeout(&self, dur: Option<Duration>) -> io::Result<()> {
        self.inner.set_timeout(dur, ZTS_SO_RCVTIMEO as i32)
    }

    pub fn set_write_timeout(&self, dur: Option<Duration>) -> io::Result<()> {
        self.inner.set_timeout(dur, ZTS_SO_SNDTIMEO as i32)
    }

    pub fn read_timeout(&self) -> io::Result<Option<Duration>> {
        self.inner.timeout(ZTS_SO_RCVTIMEO as i32)
    }

    pub fn write_timeout(&self) -> io::Result<Option<Duration>> {
        self.inner.timeout(ZTS_SO_SNDTIMEO as i32)
    }

    pub fn set_broadcast(&self, broadcast: bool) -> io::Result<()> {
        setsockopt(
            &self.inner,
            ZTS_SOL_SOCKET as i32,
            ZTS_SO_BROADCAST as i32,
            broadcast as c_int,
        )
    }

    pub fn broadcast(&self) -> io::Result<bool> {
        let raw: c_int = getsockopt(&self.inner, ZTS_SOL_SOCKET as i32, ZTS_SO_BROADCAST as i32)?;
        Ok(raw != 0)
    }
    /*
        pub fn set_multicast_loop_v4(&self, multicast_loop_v4: bool) -> io::Result<()> {
            setsockopt(
                &self.inner,
                ZTS_IPPROTO_IP,
                ZTS_IP_MULTICAST_LOOP,
                multicast_loop_v4 as IpV4MultiCastType,
            )
        }

        pub fn multicast_loop_v4(&self) -> io::Result<bool> {
            let raw: IpV4MultiCastType = getsockopt(&self.inner, ZTS_IPPROTO_IP, ZTS_IP_MULTICAST_LOOP)?;
            Ok(raw != 0)
        }

        pub fn set_multicast_ttl_v4(&self, multicast_ttl_v4: u32) -> io::Result<()> {
            setsockopt(
                &self.inner,
                ZTS_IPPROTO_IP,
                ZTS_IP_MULTICAST_TTL,
                multicast_ttl_v4 as IpV4MultiCastType,
            )
        }

        pub fn multicast_ttl_v4(&self) -> io::Result<u32> {
            let raw: IpV4MultiCastType = getsockopt(&self.inner, ZTS_IPPROTO_IP, ZTS_IP_MULTICAST_TTL)?;
            Ok(raw as u32)
        }

        pub fn set_multicast_loop_v6(&self, multicast_loop_v6: bool) -> io::Result<()> {
            setsockopt(&self.inner, ZTS_IPPROTO_IPV6, ZTS_IPV6_MULTICAST_LOOP, multicast_loop_v6 as c_int)
        }

        pub fn multicast_loop_v6(&self) -> io::Result<bool> {
            let raw: c_int = getsockopt(&self.inner, ZTS_IPPROTO_IPV6, ZTS_IPV6_MULTICAST_LOOP)?;
            Ok(raw != 0)
        }

        pub fn join_multicast_v4(&self, multiaddr: &Ipv4Addr, interface: &Ipv4Addr) -> io::Result<()> {
            let mreq = zts_ip_mreq {
                imr_multiaddr: multiaddr.into_inner(),
                imr_interface: interface.into_inner(),
            };
            setsockopt(&self.inner, ZTS_IPPROTO_IP, ZTS_IP_ADD_MEMBERSHIP, mreq)
        }

        pub fn join_multicast_v6(&self, multiaddr: &Ipv6Addr, interface: u32) -> io::Result<()> {
            let mreq = zts_ipv6_mreq {
                ipv6mr_multiaddr: *multiaddr.as_inner(),
                ipv6mr_interface: to_ipv6mr_interface(interface),
            };
            setsockopt(&self.inner, ZTS_IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, mreq)
        }

        pub fn leave_multicast_v4(&self, multiaddr: &Ipv4Addr, interface: &Ipv4Addr) -> io::Result<()> {
            let mreq = zts_ip_mreq {
                imr_multiaddr: multiaddr.into_inner(),
                imr_interface: interface.into_inner(),
            };
            setsockopt(&self.inner, ZTS_IPPROTO_IP, ZTS_IP_DROP_MEMBERSHIP, mreq)
        }

        pub fn leave_multicast_v6(&self, multiaddr: &Ipv6Addr, interface: u32) -> io::Result<()> {
            let mreq = zts_ipv6_mreq {
                ipv6mr_multiaddr: *multiaddr.as_inner(),
                ipv6mr_interface: to_ipv6mr_interface(interface),
            };
            setsockopt(&self.inner, ZTS_IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, mreq)
        }
    */
    pub fn set_ttl(&self, ttl: u32) -> io::Result<()> {
        setsockopt(
            &self.inner,
            ZTS_IPPROTO_IP as i32,
            ZTS_IP_TTL as i32,
            ttl as c_int,
        )
    }

    pub fn ttl(&self) -> io::Result<u32> {
        let raw: c_int = getsockopt(&self.inner, ZTS_IPPROTO_IP as i32, ZTS_IP_TTL as i32)?;
        Ok(raw as u32)
    }

    pub fn take_error(&self) -> io::Result<Option<io::Error>> {
        self.inner.take_error()
    }

    pub fn set_nonblocking(&self, nonblocking: bool) -> io::Result<()> {
        self.inner.set_nonblocking(nonblocking)
    }

    pub fn recv(&self, buf: &mut [u8]) -> io::Result<usize> {
        self.inner.read(buf)
    }

    pub fn peek(&self, buf: &mut [u8]) -> io::Result<usize> {
        self.inner.peek(buf)
    }

    pub fn send(&self, buf: &[u8]) -> io::Result<usize> {
        /*
        let len = cmp::min(buf.len(), <wrlen_t>::MAX as usize) as wrlen_t;
        let ret = cvt(unsafe {
            zts_bsd_send(*self.inner.as_inner(), buf.as_ptr() as *const c_void, len, ZTS_MSG_NOSIGNAL)
        })?;
        Ok(ret as usize)
        */
        unsafe {
            let raw = zts_bsd_write(
                *self.inner.as_inner(),
                buf.as_ptr() as *const c_void,
                buf.len().try_into().unwrap(),
            );
            if raw >= 0 {
                Ok(raw.try_into().unwrap())
            } else {
                Err(io::Error::from_raw_os_error(raw as i32))
            }
        }
    }

    pub fn connect(&self, addr: io::Result<&SocketAddr>) -> io::Result<()> {
        let addr = addr?;
        //let (addrp, len) = addr?.into_inner();
        unsafe {
            let full_str = addr.to_string();
            let full_vec = full_str.split(":");
            let lvec: Vec<&str> = full_vec.collect();
            let addr_str = lvec[0];
            let port = addr.port();
            let timeout_ms = 0;
            // TODO: Handle native error code, consider cvt?
            // This is a false-positive by the linter
            // See: https://github.com/rust-lang/rust/issues/78691
            #[allow(temporary_cstring_as_ptr)]
            cvt(zts_connect(
                *self.inner.as_inner(),
                CString::new(addr_str).unwrap().as_ptr(),
                port,
                timeout_ms,
            ))?;
        }

        Ok(())
    }
}

impl FromInner<Socket> for UdpSocketImpl {
    fn from_inner(socket: Socket) -> UdpSocketImpl {
        UdpSocketImpl { inner: socket }
    }
}

/*
impl fmt::Debug for UdpSocketImpl {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let mut res = f.debug_struct("UdpSocketImpl");

        if let Ok(addr) = self.socket_addr() {
            res.field("addr", &addr);
        }

        let name = if cfg!(windows) { "socket" } else { "fd" };
        res.field(name, &self.inner.as_inner()).finish()
    }
}
*/

//----------------------------------------------------------------------------//
// UdpSocket                                                                  //
//----------------------------------------------------------------------------//

pub struct UdpSocket(UdpSocketImpl);

impl UdpSocket {
    pub fn bind<A: ToSocketAddrs>(addr: A) -> io::Result<UdpSocket> {
        each_addr(addr, UdpSocketImpl::bind).map(UdpSocket)
    }

    pub fn recv_from(&self, buf: &mut [u8]) -> io::Result<(usize, SocketAddr)> {
        self.0.recv_from(buf)
    }

    pub fn peek_from(&self, buf: &mut [u8]) -> io::Result<(usize, SocketAddr)> {
        self.0.peek_from(buf)
    }
    /*
        pub fn send_to<A: ToSocketAddrs>(&self, buf: &[u8], addr: A) -> io::Result<usize> {
            match addr.to_socket_addrs()?.next() {
                Some(addr) => self.0.send_to(buf, &addr),
                None => Err(Error::new(
                    ErrorKind::InvalidInput,
                    "No address to send data to",
                )),
            }
        }
    */
    pub fn peer_addr(&self) -> io::Result<SocketAddr> {
        self.0.peer_addr()
    }

    pub fn local_addr(&self) -> io::Result<SocketAddr> {
        self.0.socket_addr()
    }

    pub fn set_read_timeout(&self, dur: Option<Duration>) -> io::Result<()> {
        self.0.set_read_timeout(dur)
    }

    pub fn set_write_timeout(&self, dur: Option<Duration>) -> io::Result<()> {
        self.0.set_write_timeout(dur)
    }

    pub fn read_timeout(&self) -> io::Result<Option<Duration>> {
        self.0.read_timeout()
    }

    pub fn write_timeout(&self) -> io::Result<Option<Duration>> {
        self.0.write_timeout()
    }

    pub fn set_broadcast(&self, broadcast: bool) -> io::Result<()> {
        self.0.set_broadcast(broadcast)
    }

    pub fn broadcast(&self) -> io::Result<bool> {
        self.0.broadcast()
    }
    /*
        pub fn set_multicast_loop_v4(&self, multicast_loop_v4: bool) -> io::Result<()> {
            self.0.set_multicast_loop_v4(multicast_loop_v4)
        }

        pub fn multicast_loop_v4(&self) -> io::Result<bool> {
            self.0.multicast_loop_v4()
        }

        pub fn set_multicast_ttl_v4(&self, multicast_ttl_v4: u32) -> io::Result<()> {
            self.0.set_multicast_ttl_v4(multicast_ttl_v4)
        }

        pub fn multicast_ttl_v4(&self) -> io::Result<u32> {
            self.0.multicast_ttl_v4()
        }

        pub fn set_multicast_loop_v6(&self, multicast_loop_v6: bool) -> io::Result<()> {
            self.0.set_multicast_loop_v6(multicast_loop_v6)
        }

        pub fn multicast_loop_v6(&self) -> io::Result<bool> {
            self.0.multicast_loop_v6()
        }
    */
    pub fn set_ttl(&self, ttl: u32) -> io::Result<()> {
        self.0.set_ttl(ttl)
    }

    pub fn ttl(&self) -> io::Result<u32> {
        self.0.ttl()
    }
    /*
        pub fn join_multicast_v4(&self, multiaddr: &Ipv4Addr, interface: &Ipv4Addr) -> io::Result<()> {
            self.0.join_multicast_v4(multiaddr, interface)
        }

        pub fn join_multicast_v6(&self, multiaddr: &Ipv6Addr, interface: u32) -> io::Result<()> {
            self.0.join_multicast_v6(multiaddr, interface)
        }

        pub fn leave_multicast_v4(&self, multiaddr: &Ipv4Addr, interface: &Ipv4Addr) -> io::Result<()> {
            self.0.leave_multicast_v4(multiaddr, interface)
        }

        pub fn leave_multicast_v6(&self, multiaddr: &Ipv6Addr, interface: u32) -> io::Result<()> {
            self.0.leave_multicast_v6(multiaddr, interface)
        }
    */
    pub fn take_error(&self) -> io::Result<Option<io::Error>> {
        self.0.take_error()
    }

    pub fn connect<A: ToSocketAddrs>(&self, addr: A) -> io::Result<()> {
        each_addr(addr, |addr| self.0.connect(addr))
    }

    pub fn send(&self, buf: &[u8]) -> io::Result<usize> {
        self.0.send(buf)
    }

    pub fn recv(&self, buf: &mut [u8]) -> io::Result<usize> {
        self.0.recv(buf)
    }

    pub fn peek(&self, buf: &mut [u8]) -> io::Result<usize> {
        self.0.peek(buf)
    }

    pub fn set_nonblocking(&self, nonblocking: bool) -> io::Result<()> {
        self.0.set_nonblocking(nonblocking)
    }
}

impl AsInner<UdpSocketImpl> for UdpSocket {
    fn as_inner(&self) -> &UdpSocketImpl {
        &self.0
    }
}

impl FromInner<UdpSocketImpl> for UdpSocket {
    fn from_inner(inner: UdpSocketImpl) -> UdpSocket {
        UdpSocket(inner)
    }
}

impl IntoInner<UdpSocketImpl> for UdpSocket {
    fn into_inner(self) -> UdpSocketImpl {
        self.0
    }
}

/*
impl fmt::Debug for UdpSocket {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        self.0.fmt(f)
    }
}
*/
