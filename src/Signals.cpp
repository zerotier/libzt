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

#ifdef ZTS_ENABLE_PYTHON
/**
 * In some situations (Python comes to mind) a signal may not make its
 * way to libzt, for this reason we make sure to define a custom signal
 * handler that can at least process SIGTERMs
 */
#define ZTS_ENABLE_CUSTOM_SIGNAL_HANDLERS 1
#endif

#ifdef ZTS_ENABLE_CUSTOM_SIGNAL_HANDLERS

#include "Signals.hpp"

#include "ZeroTierSockets.h"

#include <cstdlib>
#include <signal.h>

void zts_signal_handler(int signal)
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

void zts_install_signal_handlers()
{
    signal(SIGINT, &zts_signal_handler);
    /*
    signal(SIGABRT, &zts_signal_handler);
    signal(SIGFPE, &zts_signal_handler);
    signal(SIGILL, &zts_signal_handler);
    signal(SIGSEGV, &zts_signal_handler);
    signal(SIGTERM, &zts_signal_handler);
    */
}

#endif
