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

use std::ffi::{c_void, CString};
use std::io;
use std::io::{Read, Write};
use std::net::{Shutdown, SocketAddr, ToSocketAddrs};
use std::os::raw::c_int;
use std::time::Duration;
use std::{cmp, mem};

use crate::socket::Socket;
use crate::utils::*;

//----------------------------------------------------------------------------//
// TcpStream                                                                  //
//----------------------------------------------------------------------------//

pub struct TcpStreamImpl {
    inner: Socket,
}

impl TcpStreamImpl {
    pub fn connect(addr: io::Result<&SocketAddr>) -> io::Result<TcpStreamImpl> {
        let addr = addr?;
        let socket = Socket::new(addr, ZTS_SOCK_STREAM as i32)?;
        //let (addrp, len) = addr.into_inner();
        unsafe {
            // TODO: Find a better way to split this address string
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
            zts_connect(
                *socket.as_inner(),
                CString::new(addr_str).unwrap().as_ptr(),
                port,
                timeout_ms,
            );
        }
        Ok(TcpStreamImpl { inner: socket })
    }
    pub fn write(&self, buf: &[u8]) -> io::Result<usize> {
        let len = cmp::min(buf.len(), <size_t>::MAX as usize) as size_t;
        // TODO: Handle native error code, consider cvt?
        let ret =
            unsafe { zts_bsd_write(*self.inner.as_inner(), buf.as_ptr() as *const c_void, len) };
        Ok(ret as usize)
    }
    /*
        pub fn connect_timeout(addr: &SocketAddr, timeout: Duration) -> io::Result<TcpStream> {
            init();

            let sock = Socket::new(addr, c::SOCK_STREAM)?;
            sock.connect_timeout(addr, timeout)?;
            Ok(TcpStream { inner: sock })
        }
    */
    pub fn socket(&self) -> &Socket {
        &self.inner
    }

    pub fn into_socket(self) -> Socket {
        self.inner
    }

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

    pub fn peek(&self, buf: &mut [u8]) -> io::Result<usize> {
        self.inner.peek(buf)
    }

    pub fn read(&self, buf: &mut [u8]) -> io::Result<usize> {
        self.inner.read(buf)
    }
    /*
        pub fn read_vectored(&self, bufs: &mut [IoSliceMut<'_>]) -> io::Result<usize> {
            self.inner.read_vectored(bufs)
        }

        #[inline]
        pub fn is_read_vectored(&self) -> bool {
            self.inner.is_read_vectored()
        }

        pub fn write(&self, buf: &[u8]) -> io::Result<usize> {
            let len = cmp::min(buf.len(), <wrlen_t>::MAX as usize) as wrlen_t;
            let ret = cvt(unsafe {
                zts_bsd_send(*self.inner.as_inner(), buf.as_ptr() as *const c_void, len, ZTS_MSG_NOSIGNAL)
            })?;
            Ok(ret as usize)
        }

        pub fn write_vectored(&self, bufs: &[IoSlice<'_>]) -> io::Result<usize> {
            self.inner.write_vectored(bufs)
        }

        #[inline]
        pub fn is_write_vectored(&self) -> bool {
            self.inner.is_write_vectored()
        }
    */
    pub fn peer_addr(&self) -> io::Result<SocketAddr> {
        sockname(|buf, len| unsafe { zts_bsd_getpeername(*self.inner.as_inner(), buf, len) })
    }

    pub fn socket_addr(&self) -> io::Result<SocketAddr> {
        sockname(|buf, len| unsafe { zts_bsd_getsockname(*self.inner.as_inner(), buf, len) })
    }

    pub fn shutdown(&self, how: Shutdown) -> io::Result<()> {
        self.inner.shutdown(how)
    }

    pub fn set_nodelay(&self, nodelay: bool) -> io::Result<()> {
        self.inner.set_nodelay(nodelay)
    }

    pub fn nodelay(&self) -> io::Result<bool> {
        self.inner.nodelay()
    }

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
}

pub struct TcpStream(TcpStreamImpl);

impl TcpStream {
    pub fn connect<A: ToSocketAddrs>(addr: A) -> io::Result<TcpStream> {
        each_addr(addr, TcpStreamImpl::connect).map(TcpStream)
    }
    /*
        pub fn connect_timeout(addr: &SocketAddr, timeout: Duration) -> io::Result<TcpStream> {
            TcpStreamImpl::connect_timeout(addr, timeout).map(TcpStream)
        }
    */
    pub fn peer_addr(&self) -> io::Result<SocketAddr> {
        self.0.peer_addr()
    }

    pub fn local_addr(&self) -> io::Result<SocketAddr> {
        self.0.socket_addr()
    }

    pub fn shutdown(&self, how: Shutdown) -> io::Result<()> {
        self.0.shutdown(how)
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

    pub fn peek(&self, buf: &mut [u8]) -> io::Result<usize> {
        self.0.peek(buf)
    }

    pub fn set_nodelay(&self, nodelay: bool) -> io::Result<()> {
        self.0.set_nodelay(nodelay)
    }

    pub fn nodelay(&self) -> io::Result<bool> {
        self.0.nodelay()
    }

    pub fn set_ttl(&self, ttl: u32) -> io::Result<()> {
        self.0.set_ttl(ttl)
    }

    pub fn ttl(&self) -> io::Result<u32> {
        self.0.ttl()
    }

    pub fn take_error(&self) -> io::Result<Option<io::Error>> {
        self.0.take_error()
    }

    pub fn set_nonblocking(&self, nonblocking: bool) -> io::Result<()> {
        self.0.set_nonblocking(nonblocking)
    }
}

impl Read for TcpStream {
    fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        self.0.read(buf)
    }
    /*
        fn read_vectored(&mut self, bufs: &mut [IoSliceMut<'_>]) -> io::Result<usize> {
            self.0.read_vectored(bufs)
        }

        #[inline]
        fn is_read_vectored(&self) -> bool {
            self.0.is_read_vectored()
        }

        #[inline]
        unsafe fn initializer(&self) -> Initializer {
            // SAFETY: Read is guaranteed to work on uninitialized memory
            unsafe { Initializer::nop() }
        }
    */
}

impl Write for TcpStream {
    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        self.0.write(buf)
    }
    /*
        fn write_vectored(&mut self, bufs: &[IoSlice<'_>]) -> io::Result<usize> {
            self.0.write_vectored(bufs)
        }

        #[inline]
        fn is_write_vectored(&self) -> bool {
            self.0.is_write_vectored()
        }
    */
    fn flush(&mut self) -> io::Result<()> {
        Ok(())
    }
}

impl Read for &TcpStream {
    fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        self.0.read(buf)
    }
    /*
    fn read_vectored(&mut self, bufs: &mut [IoSliceMut<'_>]) -> io::Result<usize> {
        self.0.read_vectored(bufs)
    }

    #[inline]
    fn is_read_vectored(&self) -> bool {
        self.0.is_read_vectored()
    }

    #[inline]
    unsafe fn initializer(&self) -> Initializer {
        // SAFETY: Read is guaranteed to work on uninitialized memory
        unsafe { Initializer::nop() }
    }
    */
}

impl Write for &TcpStream {
    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        self.0.write(buf)
    }
    /*
        fn write_vectored(&mut self, bufs: &[IoSlice<'_>]) -> io::Result<usize> {
            self.0.write_vectored(bufs)
        }

        #[inline]
        fn is_write_vectored(&self) -> bool {
            self.0.is_write_vectored()
        }
    */
    fn flush(&mut self) -> io::Result<()> {
        Ok(())
    }
}

impl AsInner<TcpStreamImpl> for TcpStream {
    fn as_inner(&self) -> &TcpStreamImpl {
        &self.0
    }
}

impl FromInner<TcpStreamImpl> for TcpStream {
    fn from_inner(inner: TcpStreamImpl) -> TcpStream {
        TcpStream(inner)
    }
}

impl IntoInner<TcpStreamImpl> for TcpStream {
    fn into_inner(self) -> TcpStreamImpl {
        self.0
    }
}
/*
impl fmt::Debug for TcpStream {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        self.0.fmt(f)
    }
}
*/

//----------------------------------------------------------------------------//
// TcpListener                                                                //
//----------------------------------------------------------------------------//

pub struct TcpListenerImpl {
    inner: Socket,
}

impl TcpListenerImpl {
    pub fn bind(addr: io::Result<&SocketAddr>) -> io::Result<TcpListenerImpl> {
        let addr = addr?;
        let socket = Socket::new(addr, ZTS_SOCK_STREAM as i32)?;
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
            // TODO: Handle native error code, consider cvt?
            zts_bsd_listen(*socket.as_inner(), 128);
        }
        Ok(TcpListenerImpl { inner: socket })
    }

    pub fn accept(&self) -> io::Result<(TcpStreamImpl, SocketAddr)> {
        let mut storage: zts_sockaddr_storage = unsafe { mem::zeroed() };
        let mut len = mem::size_of_val(&storage) as zts_socklen_t;
        // TODO: Handle native error code, consider cvt?
        let socket = self
            .inner
            .accept(&mut storage as *mut _ as *mut _, &mut len)?;
        let addr = sockaddr_to_addr(&storage, len as usize)?;
        Ok((TcpStreamImpl { inner: socket }, addr))
    }

    pub fn socket(&self) -> &Socket {
        &self.inner
    }

    pub fn into_socket(self) -> Socket {
        self.inner
    }

    pub fn socket_addr(&self) -> io::Result<SocketAddr> {
        sockname(|buf, len| unsafe { zts_bsd_getsockname(*self.inner.as_inner(), buf, len) })
    }

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

    pub fn set_only_v6(&self, only_v6: bool) -> io::Result<()> {
        setsockopt(
            &self.inner,
            ZTS_IPPROTO_IPV6 as i32,
            ZTS_IPV6_V6ONLY as i32,
            only_v6 as c_int,
        )
    }

    pub fn only_v6(&self) -> io::Result<bool> {
        let raw: c_int = getsockopt(&self.inner, ZTS_IPPROTO_IPV6 as i32, ZTS_IPV6_V6ONLY as i32)?;
        Ok(raw != 0)
    }

    pub fn take_error(&self) -> io::Result<Option<io::Error>> {
        self.inner.take_error()
    }

    pub fn set_nonblocking(&self, nonblocking: bool) -> io::Result<()> {
        self.inner.set_nonblocking(nonblocking)
    }
}

pub struct TcpListener(TcpListenerImpl);

pub struct Incoming<'a> {
    listener: &'a TcpListener,
}

impl<'a> Iterator for Incoming<'a> {
    type Item = io::Result<TcpStream>;
    fn next(&mut self) -> Option<io::Result<TcpStream>> {
        Some(self.listener.accept().map(|p| p.0))
    }
}

impl TcpListener {
    pub fn bind<A: ToSocketAddrs>(addr: A) -> io::Result<TcpListener> {
        each_addr(addr, TcpListenerImpl::bind).map(TcpListener)
    }

    pub fn incoming(&self) -> Incoming<'_> {
        Incoming { listener: self }
    }

    pub fn accept(&self) -> io::Result<(TcpStream, SocketAddr)> {
        self.0.accept().map(|(a, b)| (TcpStream(a), b))
    }

    pub fn local_addr(&self) -> io::Result<SocketAddr> {
        self.0.socket_addr()
    }

    pub fn set_ttl(&self, ttl: u32) -> io::Result<()> {
        self.0.set_ttl(ttl)
    }

    pub fn ttl(&self) -> io::Result<u32> {
        self.0.ttl()
    }

    pub fn take_error(&self) -> io::Result<Option<io::Error>> {
        self.0.take_error()
    }

    pub fn set_nonblocking(&self, nonblocking: bool) -> io::Result<()> {
        self.0.set_nonblocking(nonblocking)
    }
}

impl AsInner<TcpListenerImpl> for TcpListener {
    fn as_inner(&self) -> &TcpListenerImpl {
        &self.0
    }
}

impl FromInner<TcpListenerImpl> for TcpListener {
    fn from_inner(inner: TcpListenerImpl) -> TcpListener {
        TcpListener(inner)
    }
}

impl IntoInner<TcpListenerImpl> for TcpListener {
    fn into_inner(self) -> TcpListenerImpl {
        self.0
    }
}
/*
impl fmt::Debug for TcpListener {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        self.0.fmt(f)
    }
}
*/
