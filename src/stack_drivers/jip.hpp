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

#ifndef SDK_JIPSTACK_H
#define SDK_JIPSTACK_H

#include "Mutex.hpp"
#include "OSUtils.hpp"
#include "debug.h"

#include <stdio.h>
#include <dlfcn.h>

#ifdef D_GNU_SOURCE
#define _GNU_SOURCE
#endif

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
    class jip_stack
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

        //void (*_netif_init)(void);

        
        Mutex _lock;        
        Mutex _lock_mem;

        jip_stack(const char* path) :
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

            //_netif_init = (void(*)(void))&netif_init;

#endif
            
#ifdef __DYNAMIC_LWIP__ // Use dynamically-loaded symbols (for use in normal desktop applications)
            
            if(_libref == NULL)
                DEBUG_ERROR("dlerror(): %s", dlerror());

            //_netif_init = (void(*)(void))dlsym(_libref, "netif_init");

#endif
        }
        
        ~jip_stack()
        {
            if (_libref)
                dlclose(_libref);
        }
        
        //inline void __netif_init(void) throw() { Mutex::Lock _l(_lock); _netif_init(); }

};
    
} // namespace ZeroTier

#endif
