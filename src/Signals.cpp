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

#include "ZeroTierSockets.h"
#include "Signals.hpp"

#ifdef ZTS_ENABLE_CUSTOM_SIGNAL_HANDLERS

#include <signal.h>
#include <execinfo.h>
#include <cstdlib>

void _signal_handler(int signal)
{
	/*
	switch(signal)
	{
	case SIGINT:
		fprintf(stderr, "SIGINT\n");
		break;
	case SIGABRT:
		fprintf(stderr, "SIGABRT\n");
		break;
	case SIGILL:
		fprintf(stderr, "SIGILL\n");
		break;
	case SIGSEGV:
		fprintf(stderr, "SIGSEGV\n");
		break;
	case SIGFPE:
		fprintf(stderr, "SIGFPE\n");
		break;
	case SIGTERM:
	default:
		fprintf(stderr, "SIGTERM\n");
		break;
	}
	*/
	exit(signal);
}

void _install_signal_handlers()
{
	signal(SIGINT, &_signal_handler);
	/*
	signal(SIGABRT, &_signal_handler);
	signal(SIGFPE, &_signal_handler);
	signal(SIGILL, &_signal_handler);
	signal(SIGSEGV, &_signal_handler);
	signal(SIGTERM, &_signal_handler);
	*/
}

#endif
