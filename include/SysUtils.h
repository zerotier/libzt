/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2018  ZeroTier, Inc.  https://www.zerotier.com/
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
 * You can be released from the requirements of the license by purchasing
 * a commercial license. Buying such a license is mandatory as soon as you
 * develop commercial closed-source software that incorporates or links
 * directly against ZeroTier software without disclosing the source code
 * of your own application.
 */

/**
 * @file
 *
 * Platform-specific implementations of common functions
 */

#ifndef LIBZT_SYSUTILS_H
#define LIBZT_SYSUTILS_H

#include <stdint.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif

/**
 * @brief Returns the thread-id. Used in debug traces.
 *
 * @usage For internal use only.
 * @return
 */
inline unsigned int gettid();

/**
 * @brief Current time in milliseconds since epoch, platform-aware convenience function.
 *
 * @usage For internal use only.
 * @return Current time in integer form
 */
inline uint64_t time_now()
{
#ifdef _WIN32
    FILETIME ft;
    SYSTEMTIME st;
    ULARGE_INTEGER tmp;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st,&ft);
    tmp.LowPart = ft.dwLowDateTime;
    tmp.HighPart = ft.dwHighDateTime;
    return (uint64_t)( ((tmp.QuadPart - 116444736000000000LL) / 10000L) + st.wMilliseconds );
#else
    struct timeval tv;
#ifdef __LINUX__
    syscall(SYS_gettimeofday,&tv,0); /* fix for musl libc broken gettimeofday bug */
#else
    gettimeofday(&tv,(struct timezone *)0);
#endif
    return ( (1000LL * (uint64_t)tv.tv_sec) + (uint64_t)(tv.tv_usec / 1000) );
#endif
}

#endif // _H
