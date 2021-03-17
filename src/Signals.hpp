/*
 * Copyright (c)2013-2021 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2025-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

/**
 * @file
 *
 * Custom signal handler
 */

#ifndef SIGNALS_HPP
#define SIGNALS_HPP

#ifdef ZTS_ENABLE_PYTHON
	#define ZTS_ENABLE_CUSTOM_SIGNAL_HANDLERS 1
#endif

#ifdef ZTS_ENABLE_CUSTOM_SIGNAL_HANDLERS

/**
 *
 */
void _signal_handler(int signal);

/**
 *
 */
void _install_signal_handlers();

#endif // ZTS_ENABLE_CUSTOM_SIGNAL_HANDLERS

#endif // _H
