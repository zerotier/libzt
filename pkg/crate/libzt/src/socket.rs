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
use std::ffi::c_void;
use std::io::{Error, ErrorKind};
use std::net::{Shutdown, SocketAddr};
use std::os::raw::c_int;
use std::time::Duration;
use std::{io, mem};

type time_t = i64;

use crate::utils::*;

// Note: FileDesc and c_int in libc are private so we can't use that. Use i32 instead
pub struct Socket(c_int);

impl Socket {
    pub fn new(addr: &SocketAddr, sock_type: c_int) -> io::Result<Socket> {
        let family = match *addr {
            SocketAddr::V4(..) => ZTS_AF_INET,
            SocketAddr::V6(..) => ZTS_AF_INET6,
        };
        Socket::new_raw(family as i32, sock_type)
    }

    pub fn new_raw(family: c_int, sock_type: c_int) -> io::Result<Socket> {
        unsafe {
            // TODO: Set O_CLOEXEC (this is done in the Rust netc implementation)
            let fd = zts_bsd_socket(family, sock_type, 0);
            Ok(Socket(fd))
        }
    }

    pub fn accept(
        &self,
        storage: *mut zts_sockaddr,
        len: *mut zts_socklen_t,
    ) -> io::Result<Socket> {
        let fd = unsafe { zts_bsd_accept(self.0, storage, len) };
        Ok(Socket(fd))
    }

    pub fn as_inner(&self) -> &c_int {
        &self.0
        //.as_inner()
    }

    /*
        pub fn connect_timeout(&self, addr: &SocketAddr, timeout: Duration) -> io::Result<()> {
            self.set_nonblocking(true)?;
            let r = unsafe {
                let (addrp, len) = addr.into_inner();
                cvt(zts_bsd_connect(self.0, addrp, len))
            };
            self.set_nonblocking(false)?;

            match r {
                Ok(_) => return Ok(()),
                // there's no ErrorKind for EINPROGRESS :(
                Err(ref e) if e.raw_os_error() == Some(ZTS_EINPROGRESS) => {}
                Err(e) => return Err(e),
            }

            let mut pollfd = zts_pollfd { fd: self.0, events: ZTS_POLLOUT, revents: 0 };

            if timeout.as_secs() == 0 && timeout.subsec_nanos() == 0 {
                return Err(io::Error::new_const(
                    io::ErrorKind::InvalidInput,
                    &"cannot set a 0 duration timeout",
                ));
            }

            let start = Instant::now();

            loop {
                let elapsed = start.elapsed();
                if elapsed >= timeout {
                    return Err(io::Error::new_const(io::ErrorKind::TimedOut, &"connection timed out"));
                }

                let timeout = timeout - elapsed;
                let mut timeout = timeout
                    .as_secs()
                    .saturating_mul(1_000)
                    .saturating_add(timeout.subsec_nanos() as u64 / 1_000_000);
                if timeout == 0 {
                    timeout = 1;
                }

                let timeout = cmp::min(timeout, c_int::MAX as u64) as c_int;

                match unsafe { zts_bsd_poll(&mut pollfd, 1, timeout) } {
                    -1 => {
                        let err = io::Error::last_os_error();
                        if err.kind() != io::ErrorKind::Interrupted {
                            return Err(err);
                        }
                    }
                    0 => {}
                    _ => {
                        // linux returns POLLOUT|POLLERR|POLLHUP for refused connections (!), so look
                        // for POLLHUP rather than read readiness
                        if pollfd.revents & ZTS_POLLHUP != 0 {
                            let e = self.take_error()?.unwrap_or_else(|| {
                                io::Error::new_const(
                                    io::ErrorKind::Other,
                                    &"no error set after POLLHUP",
                                )
                            });
                            return Err(e);
                        }

                        return Ok(());
                    }
                }
            }
        }
    */

    fn recv_with_flags(&self, buf: &mut [u8], flags: c_int) -> io::Result<usize> {
        unsafe {
            let raw = zts_bsd_recv(
                self.0,
                buf.as_mut_ptr() as *mut c_void,
                (buf.len() as usize).try_into().unwrap(),
                flags,
            );
            Ok(raw as usize)
        }
    }

    pub fn read(&self, buf: &mut [u8]) -> io::Result<usize> {
        self.recv_with_flags(buf, 0)
    }

    pub fn peek(&self, buf: &mut [u8]) -> io::Result<usize> {
        self.recv_with_flags(buf, ZTS_MSG_PEEK as i32)
    }
    /*
        pub fn read_vectored(&self, bufs: &mut [IoSliceMut<'_>]) -> io::Result<usize> {
            self.0.read_vectored(bufs)
        }
    */
    #[inline]
    pub fn is_read_vectored(&self) -> bool {
        // TODO: In principle this is possible but is not hooked up yet
        return false;
    }

    fn recv_from_with_flags(
        &self,
        buf: &mut [u8],
        flags: c_int,
    ) -> io::Result<(usize, SocketAddr)> {
        let mut storage: zts_sockaddr_storage = unsafe { mem::zeroed() };
        let mut addrlen = mem::size_of_val(&storage) as zts_socklen_t;

        unsafe {
            let n = zts_bsd_recvfrom(
                self.0,
                buf.as_mut_ptr() as *mut c_void,
                buf.len().try_into().unwrap(),
                flags,
                &mut storage as *mut _ as *mut _,
                &mut addrlen,
            );
            Ok((n as usize, sockaddr_to_addr(&storage, addrlen as usize)?))
        }
    }

    pub fn recv_from(&self, buf: &mut [u8]) -> io::Result<(usize, SocketAddr)> {
        self.recv_from_with_flags(buf, 0)
    }

    pub fn recv_msg(&self, msg: &mut zts_msghdr) -> io::Result<usize> {
        unsafe {
            let n = zts_bsd_recvmsg(self.0, msg, 0 /*ZTS_MSG_CMSG_CLOEXEC*/);
            Ok(n as usize)
        }
    }

    pub fn peek_from(&self, buf: &mut [u8]) -> io::Result<(usize, SocketAddr)> {
        self.recv_from_with_flags(buf, ZTS_MSG_PEEK as i32)
    }

    pub fn write(&self, buf: &[u8]) -> io::Result<usize> {
        unsafe {
            let raw = zts_bsd_write(
                self.0,
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
    /*
        pub fn write_vectored(&self, bufs: &[IoSlice<'_>]) -> io::Result<usize> {
            self.0.write_vectored(bufs)
        }
    */
    #[inline]
    pub fn is_write_vectored(&self) -> bool {
        // TODO: In principle this is possible but is not hooked up yet
        return false;
    }

    pub fn send_msg(&self, msg: &mut zts_msghdr) -> io::Result<usize> {
        unsafe {
            let n = zts_bsd_sendmsg(self.0, msg, 0);
            Ok(n as usize)
        }
    }

    pub fn set_timeout(&self, dur: Option<Duration>, kind: c_int) -> io::Result<()> {
        let timeout = match dur {
            Some(dur) => {
                if dur.as_secs() == 0 && dur.subsec_nanos() == 0 {
                    // TODO: Use new_const to avoid allocations
                    return Err(Error::new(
                        ErrorKind::InvalidInput,
                        "Cannot set a 0 duration timeout",
                    ));
                }

                let secs = if dur.as_secs() > time_t::MAX as u64 {
                    time_t::MAX
                } else {
                    dur.as_secs() as time_t
                };
                let mut timeout = zts_timeval {
                    tv_sec: secs,
                    tv_usec: dur.subsec_micros() as std::os::raw::c_long,
                };
                if timeout.tv_sec == 0 && timeout.tv_usec == 0 {
                    timeout.tv_usec = 1;
                }
                timeout
            }
            None => zts_timeval {
                tv_sec: 0,
                tv_usec: 0,
            },
        };
        setsockopt(self, ZTS_SOL_SOCKET as i32, kind, timeout)
    }

    pub fn timeout(&self, kind: c_int) -> io::Result<Option<Duration>> {
        let raw: zts_timeval = getsockopt(self, ZTS_SOL_SOCKET as i32, kind)?;
        if raw.tv_sec == 0 && raw.tv_usec == 0 {
            Ok(None)
        } else {
            let sec = raw.tv_sec as u64;
            let nsec = (raw.tv_usec as u32) * 1000;
            Ok(Some(Duration::new(sec, nsec)))
        }
    }

    pub fn shutdown(&self, how: Shutdown) -> io::Result<()> {
        let how = match how {
            Shutdown::Write => ZTS_SHUT_WR as i32,
            Shutdown::Read => ZTS_SHUT_RD as i32,
            Shutdown::Both => ZTS_SHUT_RDWR as i32,
        };
        unsafe {
            let raw: c_int = zts_bsd_shutdown(self.0, how);
            if raw == 0 {
                Ok(())
            } else {
                Err(io::Error::from_raw_os_error(raw as i32))
            }
        }
    }

    pub fn set_nodelay(&self, nodelay: bool) -> io::Result<()> {
        let nodelay = nodelay as c_int;
        unsafe {
            let raw: c_int = zts_set_no_delay(self.0, nodelay);
            if raw == 0 {
                Ok(())
            } else {
                Err(io::Error::from_raw_os_error(raw as i32))
            }
        }
    }

    pub fn nodelay(&self) -> io::Result<bool> {
        unsafe {
            let raw: c_int = zts_get_no_delay(self.0);
            Ok(raw != 0)
        }
    }

    pub fn set_nonblocking(&self, nonblocking: bool) -> io::Result<()> {
        let nonblocking = nonblocking as c_int;
        unsafe {
            let raw: c_int = zts_set_blocking(self.0, !nonblocking);
            if raw == 0 {
                Ok(())
            } else {
                Err(io::Error::from_raw_os_error(raw as i32))
            }
        }
    }

    pub fn take_error(&self) -> io::Result<Option<io::Error>> {
        unsafe {
            let raw: c_int = zts_get_last_socket_error(self.0);
            if raw == 0 {
                Ok(None)
            } else {
                Err(io::Error::from_raw_os_error(raw as i32))
            }
        }
    }
}
