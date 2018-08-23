/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2018  ZeroTier, Inc.  https://www.zerotier.com/
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
 * You can be released from the requirements of the license by purchasing
 * a commercial license. Buying such a license is mandatory as soon as you
 * develop commercial closed-source software that incorporates or links
 * directly against ZeroTier software without disclosing the source code
 * of your own application.
 */

/**
 * @file
 *
 * Javs JNI wrapper for POSIX-like socket API
 * JNI naming convention: Java_PACKAGENAME_CLASSNAME_METHODNAME
 */

#ifdef SDK_JNI

#if defined(_MSC_VER)
//
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "libzt.h"
#include "libztDefs.h"

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

namespace ZeroTier {

	void ss2zta(JNIEnv *env, struct sockaddr_storage *ss, jobject addr);
	void zta2ss(JNIEnv *env, struct sockaddr_storage *ss, jobject addr);

	/****************************************************************************/
	/* ZeroTier service controls                                                */
	/****************************************************************************/

	JNIEXPORT void JNICALL Java_com_zerotier_libzt_ZeroTier_start(
		JNIEnv *env, jobject thisObj, jstring path, jboolean blocking)
	{
		if (path) {
			const char* utf_string = env->GetStringUTFChars(path, NULL);
			zts_start(utf_string, blocking);
			env->ReleaseStringUTFChars(path, utf_string);
		}
	}

	JNIEXPORT void JNICALL Java_com_zerotier_libzt_ZeroTier_startjoin(
		JNIEnv *env, jobject thisObj, jstring path, jlong nwid)
	{
		if (path && nwid) {
			const char* utf_string = env->GetStringUTFChars(path, NULL);
			zts_startjoin(utf_string, (uint64_t)nwid);
			env->ReleaseStringUTFChars(path, utf_string);
		}
	}

	JNIEXPORT void JNICALL Java_com_zerotier_libzt_ZeroTier_stop(
		JNIEnv *env, jobject thisObj)
	{
		zts_stop();
	}

	JNIEXPORT jboolean JNICALL Java_com_zerotier_libzt_ZeroTier_core_1running(
		JNIEnv *env, jobject thisObj)
	{
		return zts_core_running();
	}

	JNIEXPORT jboolean JNICALL Java_com_zerotier_libzt_ZeroTier_stack_1running(
		JNIEnv *env, jobject thisObj)
	{
		return zts_stack_running();
	}

	JNIEXPORT jboolean JNICALL Java_com_zerotier_libzt_ZeroTier_ready(
		JNIEnv *env, jobject thisObj)
	{
		return zts_ready();
	}

	JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_join(
		JNIEnv *env, jobject thisObj, jlong nwid)
	{
		return zts_join((uint64_t)nwid);
	}

	JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_leave(
		JNIEnv *env, jobject thisObj, jlong nwid)
	{
		return zts_leave((uint64_t)nwid);
	}

	JNIEXPORT jstring JNICALL Java_com_zerotier_libzt_ZeroTier_get_1path(
		JNIEnv *env, jobject thisObj)
	{
		char pathBuf[ZT_HOME_PATH_MAX_LEN];
		zts_get_path(pathBuf, ZT_HOME_PATH_MAX_LEN);
		return env->NewStringUTF(pathBuf);
	}

	JNIEXPORT jlong JNICALL Java_com_zerotier_libzt_ZeroTier_get_1node_1id(
		JNIEnv *env, jobject thisObj)
	{
		return zts_get_node_id();
	}

	JNIEXPORT jboolean JNICALL Java_com_zerotier_libzt_ZeroTier_get_1num_1assigned_1addresses(
		JNIEnv *env, jobject thisObj, jlong nwid)
	{
		return zts_get_num_assigned_addresses(nwid);
	}

	JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_get_1address_1at_1index(
		JNIEnv *env, jobject thisObj, jlong nwid, jint index, jobject addr)
	{
		struct sockaddr_storage ss;
		socklen_t addrlen = sizeof(struct sockaddr_storage);
		int err = zts_get_address_at_index(nwid, index, (struct sockaddr*)&ss, &addrlen);
		ss2zta(env, &ss, addr);
		return err;
	}

	JNIEXPORT jboolean JNICALL Java_com_zerotier_libzt_ZeroTier_has_1address(
		JNIEnv *env, jobject thisObj, jlong nwid)
	{
		return zts_has_address(nwid);
	}

	JNIEXPORT jboolean JNICALL Java_com_zerotier_libzt_ZeroTier_get_1address(
		JNIEnv *env, jobject thisObj, jlong nwid, jint address_family, jobject addr)
	{
		struct sockaddr_storage ss;
		int err = zts_get_address((uint64_t)nwid, &ss, address_family);
		ss2zta(env, &ss, addr);
		return err;
	}

	JNIEXPORT void JNICALL Java_com_zerotier_libzt_ZeroTier_get_16plane_1addr(
		JNIEnv *env, jobject thisObj, jlong nwid, jlong nodeId, jobject addr)
	{
		struct sockaddr_storage ss;
		zts_get_6plane_addr(&ss, nwid, nodeId);
		ss2zta(env, &ss, addr);
	}

	JNIEXPORT void JNICALL Java_com_zerotier_libzt_ZeroTier_get_1rfc4193_1addr(
		JNIEnv *env, jobject thisObj, jlong nwid, jlong nodeId, jobject addr)
	{
		struct sockaddr_storage ss;
		zts_get_rfc4193_addr(&ss, nwid, nodeId);
		ss2zta(env, &ss, addr);
	}

	JNIEXPORT jlong JNICALL Java_com_zerotier_libzt_ZeroTier_get_1peer_1count(
		JNIEnv *env, jobject thisObj)
	{
		return zts_get_peer_count();
	}

	/****************************************************************************/
	/* ZeroTier Socket API                                                      */
	/****************************************************************************/

	JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_socket(
		JNIEnv *env, jobject thisObj, jint family, jint type, jint protocol)
	{
		return zts_socket(family, type, protocol);
	}

	JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_connect(
		JNIEnv *env, jobject thisObj, jint fd, jobject addr)
	{
		struct sockaddr_storage ss;
		zta2ss(env, &ss, addr);
		socklen_t addrlen = ss.ss_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
		return zts_connect(fd, (struct sockaddr *)&ss, addrlen);
	}

	JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_bind(
		JNIEnv *env, jobject thisObj, jint fd, jobject addr)
	{
		struct sockaddr_storage ss;
		int err;
		zta2ss(env, &ss, addr);
		socklen_t addrlen = ss.ss_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
		return zts_bind(fd, (struct sockaddr*)&ss, addrlen);
	}

	JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_listen(
		JNIEnv *env, jobject thisObj, jint fd, int backlog)
	{
		return zts_listen(fd, backlog);
	}

	JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_accept(
		JNIEnv *env, jobject thisObj, jint fd, jobject addr, jint port)
	{
		struct sockaddr_storage ss;
		socklen_t addrlen = sizeof(struct sockaddr_storage);
		int err = zts_accept(fd, (struct sockaddr *)&ss, &addrlen);
		ss2zta(env, &ss, addr);
		return err;
	}

#if defined(__linux__)
	 JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_accept4(
		JNIEnv *env, jobject thisObj, jint fd, jobject addr, jint port, jint flags)
	 {
		struct sockaddr_storage ss;
		socklen_t addrlen = sizeof(struct sockaddr_storage);
		int err = zts_accept4(fd, (struct sockaddr *)&ss, &addrlen, flags);
		ss2zta(env, &ss, addr);
		return err;
	}
#endif

	JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_setsockopt(
		JNIEnv *env, jobject thisObj, jint fd, jint level, jint optname, jint optval, jint optlen)
	{
		return zts_setsockopt(fd, level, optname, (void*)(uintptr_t)optval, optlen);
	}

	JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_getsockopt(
		JNIEnv *env, jobject thisObj, jint fd, jint level, jint optname, jint optval, jint optlen)
	{
		return zts_getsockopt(fd, level, optname, (void*)(uintptr_t)optval, (socklen_t *)optlen);
	}

	JNIEXPORT jboolean JNICALL Java_com_zerotier_libzt_ZeroTier_getsockname(JNIEnv *env, jobject thisObj,
		jint fd, jobject addr)
	{
		struct sockaddr_storage ss;
		socklen_t addrlen = sizeof(struct sockaddr_storage);
		int err = zts_getsockname(fd, (struct sockaddr *)&ss, &addrlen);
		ss2zta(env, &ss, addr);
		return err;
	}

	JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_getpeername(JNIEnv *env, jobject thisObj,
		jint fd, jobject addr)
	{
		struct sockaddr_storage ss;
		int err = zts_getpeername(fd, (struct sockaddr *)&ss, (socklen_t *)sizeof(struct sockaddr_storage));
		ss2zta(env, &ss, addr);
		return err;
	}

	JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_close(
		JNIEnv *env, jobject thisObj, jint fd)
	{
		return zts_close(fd);
	}

	JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_fcntl(
		JNIEnv *env, jobject thisObj, jint fd, jint cmd, jint flags)
	{
		return zts_fcntl(fd, cmd, flags);
	}

	JNIEXPORT int JNICALL Java_com_zerotier_libzt_ZeroTier_ioctl(jint fd, jlong request, void *argp)
	{
		return zts_ioctl(fd, request, argp);
	}

	JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_send(
		JNIEnv *env, jobject thisObj, jint fd, jbyteArray buf, int flags)
	{
		void *data = env->GetPrimitiveArrayCritical(buf, NULL);
		int w = zts_send(fd, data, env->GetArrayLength(buf), flags);
		env->ReleasePrimitiveArrayCritical(buf, data, 0);
		return w;
	}

	JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_sendto(
		JNIEnv *env, jobject thisObj, jint fd, jbyteArray buf, jint flags, jobject addr)
	{
		void *data = env->GetPrimitiveArrayCritical(buf, NULL);
		struct sockaddr_storage ss;
		zta2ss(env, &ss, addr);
		socklen_t addrlen = ss.ss_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
		int w = zts_sendto(fd, data, env->GetArrayLength(buf), flags, (struct sockaddr *)&ss, addrlen);
		env->ReleasePrimitiveArrayCritical(buf, data, 0);
		return w;
	}

	JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_recv(JNIEnv *env, jobject thisObj,
		jint fd, jbyteArray buf, jint flags)
	{
		void *data = env->GetPrimitiveArrayCritical(buf, NULL);
		int r = zts_recv(fd, data, env->GetArrayLength(buf), flags);
		env->ReleasePrimitiveArrayCritical(buf, data, 0);
		return r;
	}

	JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_recvfrom(
		JNIEnv *env, jobject thisObj, jint fd, jbyteArray buf, jint flags, jobject addr)
	{
		socklen_t addrlen = sizeof(struct sockaddr_storage);
		struct sockaddr_storage ss;
		void *data = env->GetPrimitiveArrayCritical(buf, NULL);
		int r = zts_recvfrom(fd, data, env->GetArrayLength(buf), flags, (struct sockaddr *)&ss, &addrlen);
		env->ReleasePrimitiveArrayCritical(buf, data, 0);
		ss2zta(env, &ss, addr);
		return r;
	}

	JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_read(JNIEnv *env, jobject thisObj,
		jint fd, jbyteArray buf)
	{
		void *data = env->GetPrimitiveArrayCritical(buf, NULL);
		int r = zts_read(fd, data, env->GetArrayLength(buf));
		env->ReleasePrimitiveArrayCritical(buf, data, 0);
		return r;
	}

	JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_write(JNIEnv *env, jobject thisObj,
		jint fd, jbyteArray buf)
	{
		void *data = env->GetPrimitiveArrayCritical(buf, NULL);
		int w = zts_write(fd, data, env->GetArrayLength(buf));
		env->ReleasePrimitiveArrayCritical(buf, data, 0);
		return w;
	}

	JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_shutdown(
		JNIEnv *env, jobject thisObj, int fd, int how)
	{
		return zts_shutdown(fd, how);
	}
}

	/****************************************************************************/
	/* Helpers (for moving data across the JNI barrier)                         */
	/****************************************************************************/

void ss2zta(JNIEnv *env, struct sockaddr_storage *ss, jobject addr)
{
	jclass c = (*env).GetObjectClass(addr);
	if (!c) {
		return;
	}
	if(ss->ss_family == AF_INET)
	{
		struct sockaddr_in *in4 = (struct sockaddr_in*)ss;
		jfieldID fid = (*env).GetFieldID(c, "_port", "I");
		(*env).SetIntField(addr, fid, ntohs(in4->sin_port));
		fid = (*env).GetFieldID(c,"_family", "I");
		(*env).SetIntField(addr, fid, (in4->sin_family));
		fid = env->GetFieldID(c, "_ip4", "[B");
		jobject ipData = (*env).GetObjectField (addr, fid);
		jbyteArray * arr = reinterpret_cast<jbyteArray*>(&ipData);
		char *data = (char*)(*env).GetByteArrayElements(*arr, NULL);
		memcpy(data, &(in4->sin_addr.s_addr), 4);
		(*env).ReleaseByteArrayElements(*arr, (jbyte*)data, 0);

		return;
	}
	if(ss->ss_family == AF_INET6)
	{
		struct sockaddr_in6 *in6 = (struct sockaddr_in6*)ss;
		jfieldID fid = (*env).GetFieldID(c, "_port", "I");
		(*env).SetIntField(addr, fid, ntohs(in6->sin6_port));
		fid = (*env).GetFieldID(c,"_family", "I");
		(*env).SetIntField(addr, fid, (in6->sin6_family));
		fid = env->GetFieldID(c, "_ip6", "[B");
		jobject ipData = (*env).GetObjectField (addr, fid);
		jbyteArray * arr = reinterpret_cast<jbyteArray*>(&ipData);
		char *data = (char*)(*env).GetByteArrayElements(*arr, NULL);
		memcpy(data, &(in6->sin6_addr.s6_addr), 16);
		(*env).ReleaseByteArrayElements(*arr, (jbyte*)data, 0);
		return;
	}
}

void zta2ss(JNIEnv *env, struct sockaddr_storage *ss, jobject addr)
{
	jclass c = (*env).GetObjectClass(addr);
	if (!c) {
		return;
	}
	jfieldID fid = (*env).GetFieldID(c, "_family", "I");
	int family = (*env).GetIntField(addr, fid);
	if (family == AF_INET)
	{
		struct sockaddr_in *in4 = (struct sockaddr_in*)ss;
		fid = (*env).GetFieldID(c, "_port", "I");
		in4->sin_port = htons((*env).GetIntField(addr, fid));
		in4->sin_family = AF_INET;
		fid = env->GetFieldID(c, "_ip4", "[B");
		jobject ipData = (*env).GetObjectField (addr, fid);
		jbyteArray * arr = reinterpret_cast<jbyteArray*>(&ipData);
		char *data = (char*)(*env).GetByteArrayElements(*arr, NULL);
		memcpy(&(in4->sin_addr.s_addr), data, 4);
		(*env).ReleaseByteArrayElements(*arr, (jbyte*)data, 0);
		return;
	}
	if (family == AF_INET6)
	{
		struct sockaddr_in6 *in6 = (struct sockaddr_in6*)ss;
		jfieldID fid = (*env).GetFieldID(c, "_port", "I");
		in6->sin6_port = htons((*env).GetIntField(addr, fid));
		fid = (*env).GetFieldID(c,"_family", "I");
		in6->sin6_family = AF_INET6;
		fid = env->GetFieldID(c, "_ip6", "[B");
		jobject ipData = (*env).GetObjectField (addr, fid);
		jbyteArray * arr = reinterpret_cast<jbyteArray*>(&ipData);
		char *data = (char*)(*env).GetByteArrayElements(*arr, NULL);
		memcpy(&(in6->sin6_addr.s6_addr), data, 16);
		(*env).ReleaseByteArrayElements(*arr, (jbyte*)data, 0);
		return;
	}
}

#ifdef __cplusplus
}
#endif

#endif // SDK_JNI