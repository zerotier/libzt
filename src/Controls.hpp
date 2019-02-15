/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2019  ZeroTier, Inc.  https://www.zerotier.com/
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
 * Header for ZeroTier service controls
 */

#ifndef LIBZT_CONTROLS_HPP
#define LIBZT_CONTROLS_HPP

namespace ZeroTier {

//////////////////////////////////////////////////////////////////////////////
// ZeroTier Internal Service Controls                                       //
//////////////////////////////////////////////////////////////////////////////

/**
 * Add a callback event message to the queue. This can be safely called
 * from other threads since a lock-free queue is used.
 *
 * @param eventCode The event ID for this event
 * @param msg Pointer to a structure of pointers to other message-relevant
 * data structures.
 */
void postEvent(int eventCode, void *arg);

/**
 * Add a callback event message to the queue. This can be safely called
 * from other threads since a lock-free queue is used. Note: For use in
 * situations when no additional information needs to be conveyed to the
 * user application.
 *
 * @param eventCode The event ID for this event
 */
void postEvent(int eventCode);

/**
 * Free whatever was allocated to contain the callback message
 *
 * @param msg Message to be freed
 */
void freeEvent(struct zts_callback_msg *msg);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Starts a ZeroTier service in the background
 *
 * @usage For internal use only.
 * @param
 * @return
 */
#if defined(_WIN32)
DWORD WINAPI _zts_run_service(LPVOID thread_id);
#else
void *_zts_run_service(void *thread_id);
#endif

/**
 * @brief [Should not be called from user application] This function must be surrounded by 
 * ZT service locks. It will determine if it is currently safe and allowed to operate on 
 * the service.
 * @usage Can be called at any time
 * @return 1 or 0
 */
int __zts_can_perform_service_operation();

/**
 * @brief [Should not be called from user application] Returns whether or not the node is 
 * online.
 * @usage Can be called at any time
 * @return 1 or 0
 */
int __zts_node_online();

/**
 * @brief [Should not be called from user application] Adjusts the delay multiplier for the
 * network stack driver thread.
 * @usage Can be called at any time
 */
void _hibernate_if_needed();

#ifdef __cplusplus
}
#endif

} // namespace ZeroTier

#endif // _H