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
 * Misc utilities
 */

#ifndef LIBZT_UTILITIES_H
#define LIBZT_UTILITIES_H

#include <stdio.h>

namespace ZeroTier {
	struct InetAddress;
}

#if defined(_WIN32_FALSE)

#define NS_INADDRSZ  4
#define NS_IN6ADDRSZ 16
#define NS_INT16SZ   2

int inet_pton4(const char *src, void *dst);
int inet_pton6(const char *src, void *dst);
int inet_pton(int af, const char *src, void *dst);
#endif

/**
 * @brief Convert protocol numbers to human-readable strings
 *
 * @usage For internal use only.
 * @param proto
 * @return
 */
char *beautify_eth_proto_nums(int proto);

/**
 * @brief Convert a raw MAC address byte array into a human-readable string
 *
 * @usage For internal use only.
 * @param macbuf
 * @param len
 * @param addr
 * @return
 */
void mac2str(char *macbuf, int len, unsigned char* addr);

#endif // _H
