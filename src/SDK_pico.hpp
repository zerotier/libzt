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

#include "Mutex.hpp"
#include "OSUtils.hpp"
#include "SDK_Debug.h"

#include <stdio.h>
#include <dlfcn.h>

#ifdef D_GNU_SOURCE
#define _GNU_SOURCE
#endif


#if defined(SDK_LWIP) 
    #include "SDK_lwip.hpp"
#elif defined(SDK_PICOTCP)
    #include "SDK_pico.hpp"

    #include "pico_stack.h"
    #include "pico_ipv4.h"
    #include "pico_icmp4.h"
    #include "pico_dev_tap.h"
#elif defined(SDK_JIP)
    #include "SDK_jip.hpp"
#endif


#define PICO_STRING_TO_IPV4_SIG const char *ipstr, uint32_t *ip
#define PICO_IPV4_TO_STRING_SIG char *ipbuf, const uint32_t ip
#define PICO_TAP_CREATE_SIG char *name
#define PICO_IPV4_LINK_ADD_SIG struct pico_device *dev, struct pico_ip4 address, struct pico_ip4 netmask
#define PICO_DEVICE_INIT_SIG struct pico_device *dev, const char *name, uint8_t *mac
#define PICO_STACK_RECV_SIG struct pico_device *dev, uint8_t *buffer, uint32_t len
#define PICO_ICMP4_PING_SIG char *dst, int count, int interval, int timeout, int size, void (*cb)(struct pico_icmp4_stats *)

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
            _pico_tap_create = (struct pico_device*(*)(PICO_TAP_CREATE_SIG)&pico_tap_create;
            _pico_string_to_ipv4 = (int(*)(PICO_STRING_TO_IPV4_SIG))&pico_string_to_ipv4;
            _pico_ipv4_to_string = (int(*)(PICO_IPV4_TO_STRING_SIG))&pico_ipv4_to_string;
            _pico_ipv4_link_add = (int(*)(PICO_IPV4_LINK_ADD_SIG))&pico_ipv4_link_add;
            _pico_device_init = (int(*)(PICO_DEVICE_INIT_SIG))&pico_device_init;
            _pico_stack_recv = (int32_t(*)(PICO_STACK_RECV_SIG))&pico_stack_recv;
            _pico_icmp4_ping = (int(*)(PICO_ICMP4_PING_SIG))&pico_icmp4_ping;


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


#endif
        }
        
        ~picoTCP_stack()
        {
            if (_libref)
                dlclose(_libref);
        }
        
        // SIP-
        inline void __pico_stack_init(void) throw() { Mutex::Lock _l(_lock); _pico_stack_init(); }
        inline void __pico_stack_tick(void) throw() { Mutex::Lock _l(_lock); _pico_stack_tick(); }
        inline struct pico_device * __pico_tap_create(PICO_TAP_CREATE_SIG) throw() { Mutex::Lock _l(_lock); return _pico_tap_create(name); }
        inline int __pico_string_to_ipv4(PICO_STRING_TO_IPV4_SIG) throw() { Mutex::Lock _l(_lock); return _pico_string_to_ipv4(ipstr, ip); }
        inline int __pico_ipv4_to_string(PICO_IPV4_TO_STRING_SIG) throw() { Mutex::Lock _l(_lock); return _pico_ipv4_to_string(ipbuf, ip); }
        inline int __pico_ipv4_link_add(PICO_IPV4_LINK_ADD_SIG) throw() { Mutex::Lock _l(_lock); return _pico_ipv4_link_add(dev, address, netmask); }
        inline int __pico_device_init(PICO_DEVICE_INIT_SIG) throw() { Mutex::Lock _l(_lock); return _pico_device_init(dev, name, mac); }
        inline int __pico_stack_recv(PICO_STACK_RECV_SIG) throw() { /*Mutex::Lock _l(_lock);*/ return _pico_stack_recv(dev, buffer, len); }
        inline int __pico_icmp4_ping(PICO_ICMP4_PING_SIG) throw() { Mutex::Lock _l(_lock); return _pico_icmp4_ping(dst, count, interval, timeout, size, cb); }

};
    
} // namespace ZeroTier

#endif
