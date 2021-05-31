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
#![allow(unused_variables)]

include!(concat!(env!("OUT_DIR"), "/libzt.rs"));

use std::ffi::{c_void, CStr, CString};
use std::net::{AddrParseError, IpAddr};
use std::str::FromStr;

extern "C" fn native_event_handler(msg: *mut c_void) {
    let event: &mut zts_event_msg_t = unsafe { &mut *(msg as *mut zts_event_msg_t) };
    //println!("event: {}", event.event_code);
    //user_event_handler(event.event_code);
}

pub struct ZeroTierNode {
    // TODO
}

impl ZeroTierNode {
    pub fn init_set_event_handler(&self, user_event_handler: impl Fn(i16) -> ()) -> i32 {
        unsafe {
            return zts_init_set_event_handler(Some(native_event_handler));
        }
    }

    pub fn init_set_port(&self, port: u16) -> i32 {
        unsafe {
            return zts_init_set_port(port);
        }
    }

    pub fn init_from_storage(&self, storage_path: &str) -> i32 {
        unsafe {
            // This is a false-positive by the linter
            // See: https://github.com/rust-lang/rust/issues/78691
            #[allow(temporary_cstring_as_ptr)]
            return zts_init_from_storage(CString::new(storage_path).unwrap().as_ptr());
        }
    }

    pub fn start(&self) -> i32 {
        unsafe {
            return zts_node_start();
        }
    }

    pub fn stop(&self) -> i32 {
        unsafe {
            return zts_node_stop();
        }
    }

    pub fn free(&self) -> i32 {
        unsafe {
            return zts_node_free();
        }
    }

    pub fn net_join(&self, net_id: u64) -> i32 {
        unsafe {
            return zts_net_join(net_id);
        }
    }

    pub fn net_leave(&self, net_id: u64) -> i32 {
        unsafe {
            return zts_net_leave(net_id);
        }
    }

    pub fn net_transport_is_ready(&self, net_id: u64) -> bool {
        unsafe {
            return zts_net_transport_is_ready(net_id) == 1;
        }
    }

    pub fn is_online(&self) -> bool {
        unsafe {
            return zts_node_is_online() == 1;
        }
    }

    pub fn id(&self) -> u64 {
        unsafe {
            return zts_node_get_id();
        }
    }

    pub fn delay(&self, interval_ms: u64) -> () {
        unsafe { zts_util_delay(interval_ms) }
    }

    pub fn addr_get(&self, net_id: u64) -> Result<IpAddr, AddrParseError> {
        unsafe {
            let mut v = vec![0; (ZTS_INET6_ADDRSTRLEN as usize) + 1];
            let ptr = v.as_mut_ptr() as *mut i8;
            zts_addr_get_str(net_id, ZTS_AF_INET, ptr, ZTS_INET6_ADDRSTRLEN);
            let c_str = CStr::from_ptr(ptr);
            return IpAddr::from_str(&c_str.to_string_lossy().into_owned());
        }
    }
}
