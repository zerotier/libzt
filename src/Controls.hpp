/*
 * Copyright (c)2013-2020 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2024-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

/**
 * @file
 *
 * Header for ZeroTier service controls
 */

#ifndef LIBZT_CONTROLS_HPP
#define LIBZT_CONTROLS_HPP

#ifdef _WIN32
#include <Windows.h>
#endif

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
int _zts_can_perform_service_operation();

/**
 * @brief [Should not be called from user application] Returns whether or not the node is 
 * online.
 * @usage Can be called at any time
 * @return 1 or 0
 */
int _zts_node_online();

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