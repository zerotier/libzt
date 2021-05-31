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

#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!(concat!(env!("OUT_DIR"), "/libzt.rs"));

use std::ffi::c_void;
use std::io::{Error, ErrorKind};
use std::net::{Ipv4Addr, Ipv6Addr, SocketAddr, SocketAddrV4, SocketAddrV6, ToSocketAddrs};
use std::os::raw::c_int;
use std::{io, mem};

use crate::socket::Socket;

//----------------------------------------------------------------------------//
// Utilities                                                                  //
//----------------------------------------------------------------------------//

/*

The following utility functions were lifted directly from the Rust standard
library and tweaked to accommodate ZeroTier sockets. See their original
implementations here:

    - https://doc.rust-lang.org/src/std/net/tcp.rs.html#95-97
    - https://github.com/rust-lang/rust/blob/master/library/std/src/sys_common/net.rs
    - https://github.com/rust-lang/rust/blob/db492ecd5ba6bd82205612cebb9034710653f0c2/library/std/src/net/mod.rs
*/

pub trait IsMinusOne {
    fn is_minus_one(&self) -> bool;
}

macro_rules! impl_is_minus_one {
    ($($t:ident)*) => ($(impl IsMinusOne for $t {
        fn is_minus_one(&self) -> bool {
            *self == -1
        }
    })*)
}

impl_is_minus_one! { i8 i16 i32 i64 isize }

pub fn cvt<T: IsMinusOne>(t: T) -> std::io::Result<T> {
    if t.is_minus_one() {
        Err(std::io::Error::last_os_error())
    } else {
        Ok(t)
    }
}

pub fn cvt_r<T, F>(mut f: F) -> std::io::Result<T>
where
    T: IsMinusOne,
    F: FnMut() -> T,
{
    loop {
        match cvt(f()) {
            Err(ref e) if e.kind() == ErrorKind::Interrupted => {}
            other => return other,
        }
    }
}

pub fn setsockopt<T>(sock: &Socket, opt: c_int, val: c_int, payload: T) -> io::Result<()> {
    unsafe {
        let payload = &payload as *const T as *const c_void;
        cvt(zts_bsd_setsockopt(
            *sock.as_inner(),
            opt,
            val,
            payload,
            mem::size_of::<T>() as zts_socklen_t,
        ))?;
        Ok(())
    }
}

pub fn getsockopt<T: Copy>(sock: &Socket, opt: c_int, val: c_int) -> io::Result<T> {
    unsafe {
        let mut slot: T = mem::zeroed();
        let mut len = mem::size_of::<T>() as zts_socklen_t;
        cvt(zts_bsd_getsockopt(
            *sock.as_inner(),
            opt,
            val,
            &mut slot as *mut _ as *mut _,
            &mut len,
        ))?;
        Ok(slot)
    }
}

pub trait AsInner<Inner: ?Sized> {
    fn as_inner(&self) -> &Inner;
}

pub trait AsInnerMut<Inner: ?Sized> {
    fn as_inner_mut(&mut self) -> &mut Inner;
}

pub trait IntoInner<Inner> {
    fn into_inner(self) -> Inner;
}

pub trait FromInner<Inner> {
    fn from_inner(inner: Inner) -> Self;
}

pub fn each_addr<A: ToSocketAddrs, F, T>(addr: A, mut f: F) -> io::Result<T>
where
    F: FnMut(io::Result<&SocketAddr>) -> io::Result<T>,
{
    let addrs = match addr.to_socket_addrs() {
        Ok(addrs) => addrs,
        Err(e) => return f(Err(e)),
    };
    let mut last_err = None;
    for addr in addrs {
        match f(Ok(&addr)) {
            Ok(l) => return Ok(l),
            Err(e) => last_err = Some(e),
        }
    }
    // TODO: Should ideally use new_const as is used in std::net to avoid allocations
    Err(last_err.unwrap_or_else(|| {
        Error::new(
            ErrorKind::InvalidInput,
            "Could not resolve to any addresses",
        )
    }))
}

pub const fn ntohs(i: u16) -> u16 {
    u16::from_be(i)
}

pub const fn htons(i: u16) -> u16 {
    i.to_be()
}

pub fn ipv4_addr(addr: zts_in_addr) -> u32 {
    (addr.s_addr as u32).to_be()
}

// Copied from: https://docs.rs/pnet_sys/0.28.0/src/pnet_sys/unix.rs.html#162-201
pub fn sockaddr_to_addr(storage: &zts_sockaddr_storage, len: usize) -> io::Result<SocketAddr> {
    // See: https://github.com/rust-lang/rust/issues/76191
    match storage.ss_family as c_int {
        // ZTS_AF_INET
        0x2 => {
            assert!(len as usize >= mem::size_of::<zts_sockaddr_in>());
            let storage: &zts_sockaddr_in = unsafe { mem::transmute(storage) };
            let ip = ipv4_addr(storage.sin_addr);
            let a = (ip >> 24) as u8;
            let b = (ip >> 16) as u8;
            let c = (ip >> 8) as u8;
            let d = ip as u8;
            let sockaddrv4 = SocketAddrV4::new(Ipv4Addr::new(a, b, c, d), ntohs(storage.sin_port));
            Ok(SocketAddr::V4(sockaddrv4))
        }
        // ZTS_AF_INET6
        0xA => {
            assert!(len as usize >= mem::size_of::<zts_sockaddr_in6>());
            let storage: &zts_sockaddr_in6 = unsafe { mem::transmute(storage) };
            let arr: [u16; 8] = unsafe { mem::transmute(storage.sin6_addr.un.u8_addr) };
            let a = ntohs(arr[0]);
            let b = ntohs(arr[1]);
            let c = ntohs(arr[2]);
            let d = ntohs(arr[3]);
            let e = ntohs(arr[4]);
            let f = ntohs(arr[5]);
            let g = ntohs(arr[6]);
            let h = ntohs(arr[7]);
            let ip = Ipv6Addr::new(a, b, c, d, e, f, g, h);
            Ok(SocketAddr::V6(SocketAddrV6::new(
                ip,
                ntohs(storage.sin6_port),
                u32::from_be(storage.sin6_flowinfo),
                storage.sin6_scope_id,
            )))
        }
        _ => Err(io::Error::new(
            io::ErrorKind::InvalidData,
            "expected IPv4 or IPv6 socket",
        )),
    }
}

pub fn sockname<F>(f: F) -> io::Result<SocketAddr>
where
    F: FnOnce(*mut zts_sockaddr, *mut zts_socklen_t) -> c_int,
{
    unsafe {
        let mut storage: zts_sockaddr_storage = mem::zeroed();
        let mut len = mem::size_of_val(&storage) as zts_socklen_t;
        f(&mut storage as *mut _ as *mut _, &mut len);
        sockaddr_to_addr(&storage, len as usize)
    }
}
