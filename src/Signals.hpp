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

/**
 * @file
 *
 * Custom signal handler
 */

#ifndef ZTS_SIGNALS_HPP
#define ZTS_SIGNALS_HPP

#ifdef ZTS_ENABLE_PYTHON
#define ZTS_ENABLE_CUSTOM_SIGNAL_HANDLERS 1
#endif

#ifdef ZTS_ENABLE_CUSTOM_SIGNAL_HANDLERS

void zts_signal_handler(int signal);

void zts_install_signal_handlers();

#endif   // ZTS_ENABLE_CUSTOM_SIGNAL_HANDLERS

#endif   // _H
