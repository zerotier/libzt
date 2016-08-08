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

#ifdef __cplusplus
extern "C" {
#endif
	
#if defined(__ANDROID__)
	#include <jni.h>
#endif

#include <vector>

#ifndef ONE_SERVICE_SETUP_HPP
#define ONE_SERVICE_SETUP_HPP
    
#define INTERCEPT_ENABLED   111
#define INTERCEPT_DISABLED  222

#if defined(__ANDROID__)	
    // JNI naming convention: Java_PACKAGENAME_CLASSNAME_METHODNAME
    /* If you define anything else in this file it that you wish to expose to your Android 
     Java application you *must* follow that convention and any corresponding Java package/classes 
     in your Android project must match this as well */
	JNIEXPORT void JNICALL Java_ZeroTier_SDK_zt_1join_1network(JNIEnv *env, jobject thisObj, jstring nwid);
    JNIEXPORT void JNICALL Java_ZeroTier_SDK_zt_1leave_1network(JNIEnv *env, jobject thisObj, jstring nwid);
    JNIEXPORT jboolean JNICALL Java_ZeroTier_SDK_zt_1running(JNIEnv *env, jobject thisObj);
    JNIEXPORT jobject JNICALL Java_ZeroTier_SDK_zt_1get_1addresses(JNIEnv *env, jobject thisObj, jstring nwid);
#else
	void init_service(int key, const char * path);
    void init_service_and_rpc(int key, const char * path, const char * nwid);
	void init_intercept(int key);
#endif

#if defined (__ANDROID__)
	JNIEXPORT int JNICALL Java_ZeroTier_SDK_zt_1start_1service(JNIEnv *env, jobject thisObj, jstring path);
#else
	void * zt_start_service(void *thread_id);
#endif

void set_intercept_status(int mode);
void zts_join_network(const char * nwid);
void zts_leave_network(const char * nwid);
bool zts_is_running();
void zts_terminate();
std::vector<std::string> zt_get_addresses(std::string nwid);

#endif

#ifdef __cplusplus
}
#endif