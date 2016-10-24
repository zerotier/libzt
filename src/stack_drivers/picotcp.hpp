/*
 * ZeroTier One - Network Virtualization Everywhere
 * Copyright (C) 2011-2015  ZeroTier, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --
 *
 * ZeroTier may be used and distributed under the terms of the GPLv3, which
 * are available at: http://www.gnu.org/licenses/gpl-3.0.html
 *
 * If you would like to embed ZeroTier into a commercial application or
 * redistribute it in a modified binary form, please contact ZeroTier Networks
 * LLC. Start here: http://www.zerotier.com/
 */

#ifndef SDK_PICOSTACK_H
#define SDK_PICOSTACK_H

#if defined(SDK_PICOTCP)

#include <stdio.h>
#include <dlfcn.h>

#ifdef D_GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "Utils.hpp"
#include "OSUtils.hpp"
#include "Mutex.hpp"
#include "Constants.hpp"
#include "Phy.hpp"
 
#include "debug.h"
#include "tap.hpp"

#include "pico_stack.h"
#include "pico_ipv4.h"
#include "pico_icmp4.h"
#include "pico_dev_tap.h"
#include "pico_protocol.h"
#include "pico_socket.h"

// picoTCP API function signatures
#define PICO_IPV4_TO_STRING_SIG char *ipbuf, const uint32_t ip
#define PICO_TAP_CREATE_SIG char *name
#define PICO_IPV4_LINK_ADD_SIG struct pico_device *dev, struct pico_ip4 address, struct pico_ip4 netmask
#define PICO_DEVICE_INIT_SIG struct pico_device *dev, const char *name, uint8_t *mac
#define PICO_STACK_RECV_SIG struct pico_device *dev, uint8_t *buffer, uint32_t len
#define PICO_ICMP4_PING_SIG char *dst, int count, int interval, int timeout, int size, void (*cb)(struct pico_icmp4_stats *)
#define PICO_TIMER_ADD_SIG pico_time expire, void (*timer)(pico_time, void *), void *arg
#define PICO_STRING_TO_IPV4_SIG const char *ipstr, uint32_t *ip
#define PICO_STRING_TO_IPV6_SIG const char *ipstr, uint8_t *ip
#define PICO_SOCKET_SETOPTION_SIG struct pico_socket *s, int option, void *value
#define PICO_SOCKET_SEND_SIG struct pico_socket *s, const void *buf, int len
#define PICO_SOCKET_SENDTO_SIG struct pico_socket *s, const void *buf, int len, void *dst, uint16_t remote_port
#define PICO_SOCKET_RECV_SIG struct pico_socket *s, void *buf, int len
#define PICO_SOCKET_OPEN_SIG uint16_t net, uint16_t proto, void (*wakeup)(uint16_t ev, struct pico_socket *s)
#define PICO_SOCKET_BIND_SIG struct pico_socket *s, void *local_addr, uint16_t *port
#define PICO_SOCKET_CONNECT_SIG struct pico_socket *s, const void *srv_addr, uint16_t remote_port
#define PICO_SOCKET_LISTEN_SIG struct pico_socket *s, const int backlog
#define PICO_SOCKET_READ_SIG struct pico_socket *s, void *buf, int len
#define PICO_SOCKET_WRITE_SIG struct pico_socket *s, const void *buf, int len
#define PICO_SOCKET_CLOSE_SIG struct pico_socket *s
#define PICO_SOCKET_SHUTDOWN_SIG struct pico_socket *s, int mode
#define PICO_SOCKET_ACCEPT_SIG struct pico_socket *s, void *orig, uint16_t *port
#define PICO_IPV6_LINK_ADD_SIG struct pico_device *dev, struct pico_ip6 address, struct pico_ip6 netmask

namespace ZeroTier {
    
    /**
     * Loads an instance of picoTCP stack library in a private memory arena
     *
     * This uses dlmopen() to load an instance of the LWIP stack into its
     * own private memory space. This is done to get around the stack's
     * lack of thread-safety or multi-instance support. The alternative
     * would be to massively refactor the stack so everything lives in a
     * state object instead of static memory space.
     */
    class picoTCP_stack
    {
    public:
        
        void *_libref;
        
        void close() {
#if defined(__STATIC__LWIP__)
            return;
#elif defined(__DYNAMIC_LWIP__)
            dlclose(_libref);
#endif
        }

        // SIP-

        void (*_pico_stack_init)(void);
        void (*_pico_stack_tick)(void);
        int (*_pico_string_to_ipv4)(PICO_STRING_TO_IPV4_SIG);
        int (*_pico_ipv4_to_string)(PICO_IPV4_TO_STRING_SIG);
        struct pico_device* (*_pico_tap_create)(PICO_TAP_CREATE_SIG); 
        int (*_pico_ipv4_link_add)(PICO_IPV4_LINK_ADD_SIG);
        int (*_pico_device_init)(PICO_DEVICE_INIT_SIG);
        int32_t (*_pico_stack_recv)(PICO_STACK_RECV_SIG);
        int (*_pico_icmp4_ping)(PICO_ICMP4_PING_SIG);
        int (*_pico_string_to_ipv6)(PICO_STRING_TO_IPV6_SIG);
        int (*_pico_socket_setoption)(PICO_SOCKET_SETOPTION_SIG);
        uint32_t (*_pico_timer_add)(PICO_TIMER_ADD_SIG);
        int (*_pico_socket_send)(PICO_SOCKET_SEND_SIG);
        int (*_pico_socket_recv)(PICO_SOCKET_RECV_SIG);
        struct pico_socket * (*_pico_socket_open)(PICO_SOCKET_OPEN_SIG);
        int (*_pico_socket_bind)(PICO_SOCKET_BIND_SIG);
        int (*_pico_socket_connect)(PICO_SOCKET_CONNECT_SIG);
        int (*_pico_socket_listen)(PICO_SOCKET_LISTEN_SIG);
        int (*_pico_socket_read)(PICO_SOCKET_READ_SIG);
        int (*_pico_socket_write)(PICO_SOCKET_WRITE_SIG);
        int (*_pico_socket_close)(PICO_SOCKET_CLOSE_SIG);
        int (*_pico_socket_shutdown)(PICO_SOCKET_SHUTDOWN_SIG);
        struct pico_socket *(*_pico_socket_accept)(PICO_SOCKET_ACCEPT_SIG);
        int (*_pico_ipv6_link_add)(PICO_IPV6_LINK_ADD_SIG);
        //pico_err_t (*_get_pico_err)(void);
        
        Mutex _lock;        
        Mutex _lock_mem;

        picoTCP_stack(const char* path) :
        _libref(NULL)
        {
#if defined(__ANDROID__) || defined(__UNITY_3D__)
    #define __STATIC_LWIP__
#elif defined(__linux__)
    #define __DYNAMIC_LWIP__
    // Dynamically load liblwip.so
    _libref = dlmopen(LM_ID_NEWLM, path, RTLD_NOW);
#elif defined(__APPLE__)
    #include "TargetConditionals.h"
    #if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
        #include "node/Mutex.hpp"
        #define __STATIC_LWIP__
        // iOS Simulator or iOS device
        // Do nothing, symbols are statically-linked
    #elif TARGET_OS_MAC && !defined(SDK_BUNDLED)
        #define __DYNAMIC_LWIP__
        // Dynamically load liblwip.so
        _libref = dlopen(path, RTLD_NOW);
    #else
        #define __STATIC_LWIP__    
    #endif
#endif
            
#ifdef __STATIC_LWIP__ // Set static references (for use in iOS)

            // SIP-
            _pico_stack_init = (void(*)(void))&pico_stack_init;
            _pico_stack_tick = (void(*)(void))&pico_stack_tick;
            _pico_tap_create = (struct pico_device*(*)(PICO_TAP_CREATE_SIG))&pico_tap_create;
            _pico_string_to_ipv4 = (int(*)(PICO_STRING_TO_IPV4_SIG))&pico_string_to_ipv4;
            _pico_ipv4_to_string = (int(*)(PICO_IPV4_TO_STRING_SIG))&pico_ipv4_to_string;
            _pico_ipv4_link_add = (int(*)(PICO_IPV4_LINK_ADD_SIG))&pico_ipv4_link_add;
            _pico_device_init = (int(*)(PICO_DEVICE_INIT_SIG))&pico_device_init;
            _pico_stack_recv = (int32_t(*)(PICO_STACK_RECV_SIG))&pico_stack_recv;
            _pico_icmp4_ping = (int(*)(PICO_ICMP4_PING_SIG))&pico_icmp4_ping;
            _pico_string_to_ipv6 = (int(*)(PICO_STRING_TO_IPV6_SIG))&pico_string_to_ipv6;
            _pico_socket_setoption = (int(*)(PICO_SOCKET_SETOPTION_SIG))&pico_socket_setoption;
            _pico_timer_add = (uint32_t(*)(PICO_TIMER_ADD_SIG))&pico_timer_add;
            _pico_socket_send = (int(*)(PICO_SOCKET_SEND_SIG))&pico_socket_send;
            _pico_socket_recv = (int(*)(PICO_SOCKET_RECV_SIG))&pico_socket_recv;
            _pico_socket_open = (struct pico_socket*(*)(PICO_SOCKET_OPEN_SIG))&pico_socket_open;
            _pico_socket_bind = (int(*)(PICO_SOCKET_BIND_SIG))&pico_socket_bind;
            _pico_socket_connect = (int(*)(PICO_SOCKET_CONNECT_SIG))&pico_socket_connect;
            _pico_socket_listen = (int(*)(PICO_SOCKET_LISTEN_SIG))&pico_socket_listen;
            _pico_socket_read = (int(*)(PICO_SOCKET_READ_SIG))&pico_socket_read;
            _pico_socket_write = (int(*)(PICO_SOCKET_WRITE_SIG))&pico_socket_write;
            _pico_socket_close = (int(*)(PICO_SOCKET_CLOSE_SIG))&pico_socket_close;
            _pico_socket_shutdown = (int(*)(PICO_SOCKET_SHUTDOWN_SIG))&pico_socket_shutdown;
            _pico_socket_accept = (struct pico_socket*(*)(PICO_SOCKET_ACCEPT_SIG))&pico_socket_accept;
            _pico_ipv6_link_add = (int(*)(PICO_IPV6_LINK_ADD_SIG))&pico_ipv6_link_add;
            //_get_pico_err = (pico_err_t(*)())&get_pico_err;

#endif
            
#ifdef __DYNAMIC_LWIP__ // Use dynamically-loaded symbols (for use in normal desktop applications)
            
            if(_libref == NULL)
                DEBUG_ERROR("dlerror(): %s", dlerror());

            // SIP-
            _pico_stack_init = (void(*)(void))dlsym(_libref, "pico_stack_init");
            _pico_stack_tick = (void(*)(void))dlsym(_libref, "pico_stack_tick");
            _pico_tap_create = (struct pico_device*(*)(PICO_TAP_CREATE_SIG))dlsym(_libref, "pico_tap_create");
            _pico_string_to_ipv4 = (int(*)(PICO_STRING_TO_IPV4_SIG))dlsym(_libref, "pico_string_to_ipv4");
            _pico_ipv4_to_string = (int(*)(PICO_IPV4_TO_STRING_SIG))dlsym(_libref, "pico_ipv4_to_string");
            _pico_ipv4_link_add = (int(*)(PICO_IPV4_LINK_ADD_SIG))dlsym(_libref, "pico_ipv4_link_add");
            _pico_device_init = (int(*)(PICO_DEVICE_INIT_SIG))dlsym(_libref, "pico_device_init");
            _pico_stack_recv = (int32_t(*)(PICO_STACK_RECV_SIG))dlsym(_libref, "pico_stack_recv");
            _pico_icmp4_ping = (int(*)(PICO_ICMP4_PING_SIG))dlsym(_libref, "pico_icmp4_ping");
            _pico_string_to_ipv6 = (int(*)(PICO_STRING_TO_IPV6_SIG))dlsym(_libref, "pico_string_to_ipv6");
            _pico_socket_setoption = (int(*)(PICO_SOCKET_SETOPTION_SIG))dlsym(_libref, "pico_socket_setoption");
            _pico_timer_add = (uint32_t(*)(PICO_TIMER_ADD_SIG))dlsym(_libref, "pico_timer_add");
            _pico_socket_send = (int(*)(PICO_SOCKET_SEND_SIG))dlsym(_libref, "pico_socket_send");
            _pico_socket_recv = (int(*)(PICO_SOCKET_RECV_SIG))dlsym(_libref, "pico_socket_recv");
            _pico_socket_open = (struct pico_socket*(*)(PICO_SOCKET_OPEN_SIG))dlsym(_libref, "pico_socket_open");
            _pico_socket_bind = (int(*)(PICO_SOCKET_BIND_SIG))dlsym(_libref, "pico_socket_bind");
            _pico_socket_connect = (int(*)(PICO_SOCKET_CONNECT_SIG))dlsym(_libref, "pico_socket_connect");
            _pico_socket_listen = (int(*)(PICO_SOCKET_LISTEN_SIG))dlsym(_libref, "pico_socket_listen");
            _pico_socket_read = (int(*)(PICO_SOCKET_READ_SIG))dlsym(_libref, "pico_socket_read");
            _pico_socket_write = (int(*)(PICO_SOCKET_WRITE_SIG))dlsym(_libref, "pico_socket_write");
            _pico_socket_close = (int(*)(PICO_SOCKET_CLOSE_SIG))dlsym(_libref, "pico_socket_close");
            _pico_socket_shutdown = (int(*)(PICO_SOCKET_SHUTDOWN_SIG))dlsym(_libref, "pico_socket_shutdown");
            _pico_socket_accept = (struct pico_socket*(*)(PICO_SOCKET_ACCEPT_SIG))dlsym(_libref, "pico_socket_accept");
            _pico_ipv6_link_add = (int(*)(PICO_IPV6_LINK_ADD_SIG))dlsym(_libref, "pico_ipv6_link_add");

            //_get_pico_err = (pico_err_t(*)())dlsym(_libref, "get_pico_err");

#endif
        }
        
        ~picoTCP_stack()
        {
            if (_libref)
                dlclose(_libref);
        }
        
        // SIP-
        inline void __pico_stack_init(void) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); _pico_stack_init(); }
        inline void __pico_stack_tick(void) throw() { /*DEBUG_STACK();*/ Mutex::Lock _l(_lock); _pico_stack_tick(); }
        inline struct pico_device * __pico_tap_create(PICO_TAP_CREATE_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _pico_tap_create(name); }
        inline int __pico_ipv4_to_string(PICO_IPV4_TO_STRING_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _pico_ipv4_to_string(ipbuf, ip); }
        inline int __pico_ipv4_link_add(PICO_IPV4_LINK_ADD_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _pico_ipv4_link_add(dev, address, netmask); }
        inline int __pico_device_init(PICO_DEVICE_INIT_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _pico_device_init(dev, name, mac); }
        inline int __pico_stack_recv(PICO_STACK_RECV_SIG) throw() { DEBUG_STACK(); /*Mutex::Lock _l(_lock);*/ return _pico_stack_recv(dev, buffer, len); }
        inline int __pico_icmp4_ping(PICO_ICMP4_PING_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _pico_icmp4_ping(dst, count, interval, timeout, size, cb); }
        inline int __pico_string_to_ipv4(PICO_STRING_TO_IPV4_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _pico_string_to_ipv4(ipstr, ip); }
        inline int __pico_string_to_ipv6(PICO_STRING_TO_IPV6_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _pico_string_to_ipv6(ipstr, ip); }
        inline int __pico_socket_setoption(PICO_SOCKET_SETOPTION_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _pico_socket_setoption(s, option, value); }
        inline uint32_t __pico_timer_add(PICO_TIMER_ADD_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _pico_timer_add(expire, timer, arg); }
        inline int __pico_socket_send(PICO_SOCKET_SEND_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _pico_socket_send(s, buf, len); }
        inline int __pico_socket_recv(PICO_SOCKET_RECV_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _pico_socket_recv(s, buf, len); }
        inline struct pico_socket * __pico_socket_open(PICO_SOCKET_OPEN_SIG) throw() { DEBUG_STACK();  return _pico_socket_open(net, proto, wakeup); }
        inline int __pico_socket_bind(PICO_SOCKET_BIND_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _pico_socket_bind(s, local_addr, port); }
        inline int __pico_socket_connect(PICO_SOCKET_CONNECT_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _pico_socket_connect(s, srv_addr, remote_port); }
        inline int __pico_socket_listen(PICO_SOCKET_LISTEN_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _pico_socket_listen(s, backlog); }
        inline int __pico_socket_read(PICO_SOCKET_READ_SIG) throw() { DEBUG_STACK(); /*Mutex::Lock _l(_lock); */ return _pico_socket_read(s, buf, len); }
        inline int __pico_socket_write(PICO_SOCKET_WRITE_SIG) throw() { DEBUG_STACK(); /*Mutex::Lock _l(_lock);*/ return _pico_socket_write(s, buf, len); }
        inline int __pico_socket_close(PICO_SOCKET_CLOSE_SIG) throw() { DEBUG_STACK(); /*Mutex::Lock _l(_lock);*/ return _pico_socket_close(s); }
        inline int __pico_socket_shutdown(PICO_SOCKET_SHUTDOWN_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _pico_socket_shutdown(s, mode); }
        inline struct pico_socket * __pico_socket_accept(PICO_SOCKET_ACCEPT_SIG) throw() { DEBUG_STACK(); /*Mutex::Lock _l(_lock);*/ return _pico_socket_accept(s, orig, port); }
        inline int __pico_ipv6_link_add(PICO_IPV6_LINK_ADD_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _pico_ipv6_link_add(dev, address, netmask); }
        //inline pico_err_t __get_pico_err(void) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _get_pico_err(); }
    };
    
} // namespace ZeroTier

#endif

#endif // SDK_PICOTCP
