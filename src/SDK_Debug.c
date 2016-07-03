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

#ifndef _COMMON_DEBUG_
#define _COMMON_DEBUG_

#include <stdio.h>
#include <netdb.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/syscall.h>

// Set during make (e.g. make SDK_DEBUG=2)
#define DEBUG_LEVEL     5

#define MSG_TRANSFER    1 // RX/TX specific statements
#define MSG_ERROR       2 // Errors
#define MSG_INFO        3 // Information which is generally useful to any user
#define MSG_DEBUG       4 // Information which is only useful to someone debugging
#define MSG_DEBUG_EXTRA 5 // If nothing in your world makes sense

char *debug_logfile = (char*)0;
void dwr(int level, const char *fmt, ... );

void dwr(int level, const char *fmt, ... )
{
#if defined(SDK_DEBUG)
  if(level > DEBUG_LEVEL)
      return;
  int saveerr;
  saveerr = errno;
  va_list ap;
  va_start(ap, fmt);
  char timestring[20];
  time_t timestamp;
  timestamp = time(NULL);
  strftime(timestring, sizeof(timestring), "%H:%M:%S", localtime(&timestamp));
#if defined(__ANDROID__)
  pid_t tid = gettid();
#elif defined(__linux__)
  pid_t tid = 5;//syscall(SYS_gettid);
#elif defined(__APPLE__)
  pid_t tid = pthread_mach_thread_np(pthread_self());
#endif
  #if defined(SDK_DEBUG_LOG_TO_FILE)
    if(!debug_logfile) { // Try to get logfile from env
      debug_logfile = getenv("ZT_SDK_LOGFILE");
    }
    if(debug_logfile) {
      FILE *file = fopen(debug_logfile,"a");
      fprintf(file, "%s [tid=%7d] ", timestring, tid);
      vfprintf(file, fmt, ap);
      fclose(file);
      va_end(ap);
    }
  #endif
  va_start(ap, fmt);
  fprintf(stderr, "%s [tid=%7d] ", timestring, tid);
  vfprintf(stderr, fmt, ap);
  fflush(stderr);
  errno = saveerr;
  va_end(ap);
#endif // _SDK_DEBUG
}

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

#endif