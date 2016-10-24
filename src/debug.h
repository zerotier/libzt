
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


#ifndef _SDK_DEBUG_H_
#define _SDK_DEBUG_H_

#define DEBUG_LEVEL     4 // Set this to adjust what you'd like to see in the debug traces

#define MSG_ERROR       1 // Errors
#define MSG_TRANSFER    2 // RX/TX specific statements
#define MSG_INFO        3 // Information which is generally useful to any developer
#define MSG_EXTRA       4 // If nothing in your world makes sense

#define __SHOW_FILENAMES__    true
#define __SHOW_COLOR__        true

// Colors
#if defined(__APPLE__)
    #include "TargetConditionals.h"
#endif
#if defined(__SHOW_COLOR__) && !defined(__ANDROID__) && !defined(TARGET_OS_IPHONE) && !defined(TARGET_IPHONE_SIMULATOR) && !defined(__APP_FRAMEWORK__)
  #define RED   "\x1B[31m"
  #define GRN   "\x1B[32m"
  #define YEL   "\x1B[33m"
  #define BLU   "\x1B[34m"
  #define MAG   "\x1B[35m"
  #define CYN   "\x1B[36m"
  #define WHT   "\x1B[37m"
  #define RESET "\x1B[0m"
#else
  #define RED
  #define GRN
  #define YEL
  #define BLU
  #define MAG
  #define CYN
  #define WHT
  #define RESET
#endif

// filenames
#if __SHOW_FILENAMES__
  #if __SHOW_FULL_FILENAME_PATH__
    #define __FILENAME__ __FILE__ // show the entire mess
  #else
    #define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__) // shorten
  #endif
#else
  #define __FILENAME__ // omit filename
#endif

#ifdef __cplusplus
extern "C" {
#endif 

#if defined(__ANDROID__)
    #include <jni.h>
    #include <android/log.h>
    #define LOG_TAG "ZTSDK"
#endif

//#if defined(SDK_DEBUG)
 #if DEBUG_LEVEL >= MSG_ERROR
  #define DEBUG_ERROR(fmt, args...) fprintf(stderr, RED "ZT_ERROR: %20s:%4d:%25s: " fmt "\n" RESET, __FILENAME__, __LINE__, __FUNCTION__, ##args)
 #else
  #define DEBUG_ERROR(fmt, args...)
 #endif
 #if DEBUG_LEVEL >= MSG_INFO
  #if defined(__ANDROID__)
    #define DEBUG_INFO(fmt, args...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "ZT_INFO : %20s:%4d:%20s: " fmt "\n", __FILENAME__, __LINE__, __FUNCTION__, ##args))
    #define DEBUG_BLANK(fmt, args...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "ZT_INFO : %20s:%4d:" fmt "\n", __FILENAME__, __LINE__, __FUNCTION__, ##args))
  #else
    #define DEBUG_INFO(fmt, args...) fprintf(stderr, "ZT_INFO : %20s:%4d:%25s: " fmt "\n", __FILENAME__, __LINE__, __FUNCTION__, ##args)
    #define DEBUG_ATTN(fmt, args...) fprintf(stderr, CYN "ZT_INFO : %20s:%4d:%25s: " fmt "\n" RESET, __FILENAME__, __LINE__, __FUNCTION__, ##args)
    #define DEBUG_STACK(fmt, args...) fprintf(stderr, YEL "ZT_STACK: %20s:%4d:%25s: " fmt "\n" RESET, __FILENAME__, __LINE__, __FUNCTION__, ##args)
    #define DEBUG_BLANK(fmt, args...) fprintf(stderr, "ZT_INFO : %20s:%4d:" fmt "\n", __FILENAME__, __LINE__, ##args)
  #endif
 #else
  #define DEBUG_INFO(fmt, args...)
  #define DEBUG_BLANK(fmt, args...)
 #endif
 #if DEBUG_LEVEL >= MSG_TRANSFER
  #if defined(__ANDROID__)
    #define DEBUG_TRANS(fmt, args...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "ZT_TRANS : %20s:%4d:%25s: " fmt "\n", __FILENAME__, __LINE__, __FUNCTION__, ##args))
  #else
    #define DEBUG_TRANS(fmt, args...) fprintf(stderr, GRN "ZT_TRANS: %20s:%4d:%25s: " fmt "\n" RESET, __FILENAME__, __LINE__, __FUNCTION__, ##args)
  #endif
 #else
  #define DEBUG_TRANS(fmt, args...)
 #endif
 #if DEBUG_LEVEL >= MSG_EXTRA
   #if defined(__ANDROID__)
    #define DEBUG_EXTRA(fmt, args...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "ZT_EXTRA : %20s:%4d:%25s: " fmt "\n", __FILENAME__, __LINE__, __FUNCTION__, ##args))
  #else
    #define DEBUG_EXTRA(fmt, args...) fprintf(stderr, "ZT_EXTRA: %20s:%4d:%25s: " fmt "\n", __FILENAME__, __LINE__, __FUNCTION__, ##args)
  #endif
 #else
  #define DEBUG_EXTRA(fmt, args...)
 #endif
//#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _SDK_DEBUG_H_