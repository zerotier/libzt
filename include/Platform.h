/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2017  ZeroTier, Inc.  https://www.zerotier.com/
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

#ifndef LIBZT_PLATFORM_H
#define LIBZT_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Used to perform a common action upon a failure in the VirtualSocket/VirtualTap layer.
 *
 * @usage For internal use only.
 * @return
 */
void handle_general_failure();

/**
 * @brief Returns the thread-id. Used in debug traces.
 *
 * @usage For internal use only.
 * @return
 */
inline unsigned int gettid();

#ifdef __cplusplus
}
#endif

#endif // _H
