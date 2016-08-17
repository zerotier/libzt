
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

#define DEBUG_LEVEL     4 // Set this to adjust what you'd like to see in the debug traces

#define MSG_ERROR       1 // Errors
#define MSG_TRANSFER    2 // RX/TX specific statements
#define MSG_INFO        3 // Information which is generally useful to any user
#define MSG_DEBUG       4 // Information which is only useful to someone debugging
#define MSG_DEBUG_EXTRA 5 // If nothing in your world makes sense

extern char *debug_logfile;

#ifdef __cplusplus
extern "C" {
#endif
  #if __ANDROID__
    #include <jni.h>
    #include <android/log.h>
    #define LOG_TAG "ZTSDK"
    #define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
    #define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
    #define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
    #define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
  #else
    #define LOGV(...) fprintf(stdout, __VA_ARGS__)
    #define LOGI(...) fprintf(stdout, __VA_ARGS__)
    #define LOGD(...) fprintf(stdout, __VA_ARGS__)
    #define LOGE(...) fprintf(stdout, __VA_ARGS__)
  #endif
#ifdef __cplusplus
} // extern "C"
#endif