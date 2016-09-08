
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
  #define DEBUG_ERROR(fmt, args...) fprintf(stderr, "ZT_ERROR: %s:%d:%s(): " fmt "\n", __FILENAME__, __LINE__, __FUNCTION__, ##args)
 #else
  #define DEBUG_ERROR(fmt, args...)
 #endif
 #if DEBUG_LEVEL >= MSG_INFO
  #if defined(__ANDROID__)
    #define DEBUG_INFO(fmt, args...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "ZT_INFO : %s:%d:%s(): " fmt "\n", __FILENAME__, __LINE__, __FUNCTION__, ##args))
  #else
    #define DEBUG_INFO(fmt, args...) fprintf(stderr, "ZT_INFO : %s:%d:%s(): " fmt "\n", __FILENAME__, __LINE__, __FUNCTION__, ##args)
    #define DEBUG_BLANK(fmt, args...) fprintf(stderr, "ZT_INFO : %s:%d:" fmt "\n", __FILENAME__, __LINE__, ##args)
  #endif
 #else
  #define DEBUG_INFO(fmt, args...)
  #define DEBUG_BLANK(fmt, args...)
 #endif
 #if DEBUG_LEVEL >= MSG_TRANSFER
  #if defined(__ANDROID__)
    #define DEBUG_TRANS(fmt, args...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "ZT_TRANS : %s:%d:%s(): " fmt "\n", __FILENAME__, __LINE__, __FUNCTION__, ##args))
  #else
    #define DEBUG_TRANS(fmt, args...) fprintf(stderr, "ZT_TRANS: %s:%d:%s(): " fmt "\n", __FILENAME__, __LINE__, __FUNCTION__, ##args)
  #endif
 #else
  #define DEBUG_TRANS(fmt, args...)
 #endif
 #if DEBUG_LEVEL >= MSG_EXTRA
   #if defined(__ANDROID__)
    #define DEBUG_EXTRA(fmt, args...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "ZT_EXTRA : %s:%d:%s(): " fmt "\n", __FILENAME__, __LINE__, __FUNCTION__, ##args))
  #else
    #define DEBUG_EXTRA(fmt, args...) fprintf(stderr, "ZT_EXTRA: %s:%d:%s(): " fmt "\n", __FILENAME__, __LINE__, __FUNCTION__, ##args)
  #endif
 #else
  #define DEBUG_EXTRA(fmt, args...)
 #endif
//#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _SDK_DEBUG_H_


/*
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

// Outputs to Android debug console
#if defined(__ANDROID__)
  __android_log_vprint(ANDROID_LOG_VERBOSE, "ZT-JNI", fmt, ap);
#endif

  fflush(stderr);
  errno = saveerr;
  va_end(ap);
#endif // _SDK_DEBUG
}
*/