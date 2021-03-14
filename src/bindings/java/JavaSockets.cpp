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
 * ZeroTier Socket API
 */

#ifdef ZTS_ENABLE_JAVA

#include "lwip/sockets.h"
#include "lwip/def.h"
#include "lwip/inet.h"
#include "lwip/stats.h"

#include "ZeroTierSockets.h"

#define ZTS_STATE_NODE_RUNNING        0x01
#define ZTS_STATE_STACK_RUNNING       0x02
#define ZTS_STATE_NET_SERVICE_RUNNING 0x04
#define ZTS_STATE_CALLBACKS_RUNNING   0x08
#define ZTS_STATE_FREE_CALLED         0x10

#include <jni.h>

extern int zts_errno;

namespace ZeroTier {

extern uint8_t _serviceStateFlags;

#ifdef __cplusplus
extern "C" {
#endif

void ss2zta(JNIEnv *env, struct zts_sockaddr_storage *ss, jobject addr);
void zta2ss(JNIEnv *env, struct zts_sockaddr_storage *ss, jobject addr);
void ztfdset2fdset(JNIEnv *env, int nfds, jobject src_ztfd_set, zts_fd_set *dest_fd_set);
void fdset2ztfdset(JNIEnv *env, int nfds, zts_fd_set *src_fd_set, jobject dest_ztfd_set);

JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_socket(
	JNIEnv *env, jobject thisObj, jint family, jint type, jint protocol)
{
	int retval = zts_socket(family, type, protocol);
	return retval > -1 ? retval : -(zts_errno); // Encode lwIP errno into return value for JNI functions only
}

JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_connect(
	JNIEnv *env, jobject thisObj, jint fd, jobject addr)
{
	struct zts_sockaddr_storage ss;
	zta2ss(env, &ss, addr);
	socklen_t addrlen = ss.ss_family == ZTS_AF_INET ? sizeof(struct zts_sockaddr_in) : sizeof(struct zts_sockaddr_in6);
	int retval = zts_connect(fd, (struct zts_sockaddr *)&ss, addrlen);
	return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_bind(
	JNIEnv *env, jobject thisObj, jint fd, jobject addr)
{
	struct zts_sockaddr_storage ss;
	zta2ss(env, &ss, addr);
	zts_socklen_t addrlen = ss.ss_family == ZTS_AF_INET ? sizeof(struct zts_sockaddr_in) : sizeof(struct zts_sockaddr_in6);
	int retval = zts_bind(fd, (struct zts_sockaddr*)&ss, addrlen);
	return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_listen(
	JNIEnv *env, jobject thisObj, jint fd, int backlog)
{
	int retval = zts_listen(fd, backlog);
	return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_accept(
	JNIEnv *env, jobject thisObj, jint fd, jobject addr, jint port)
{
	struct zts_sockaddr_storage ss;
	zts_socklen_t addrlen = sizeof(struct zts_sockaddr_storage);
	int retval = zts_accept(fd, (zts_sockaddr*)&ss, &addrlen);
	ss2zta(env, &ss, addr);
	return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_setsockopt(
	JNIEnv *env, jobject thisObj, jint fd, jint level, jint optname, jobject optval)
{
	jclass c = env->GetObjectClass(optval);
	if (!c) {
		return ZTS_ERR_SERVICE;
	}
	int optval_int = -1;

	if (optname == SO_BROADCAST
		|| optname == SO_KEEPALIVE
		|| optname == SO_REUSEADDR
		|| optname == SO_REUSEPORT
		|| optname == TCP_NODELAY)
	{
		jfieldID fid = env->GetFieldID(c, "booleanValue", "Z");
		optval_int = (int)(env->GetBooleanField(optval, fid));
	}
	if (optname == IP_TTL
		|| optname == SO_RCVTIMEO
		|| optname == IP_TOS
		|| optname == SO_LINGER
		|| optname == SO_RCVBUF
		|| optname == SO_SNDBUF)
	{
		jfieldID fid = env->GetFieldID(c, "integerValue", "I");
		optval_int = env->GetIntField(optval, fid);
	}

	int retval = ZTS_ERR_OK;

	if (optname == SO_RCVTIMEO) {
		struct timeval tv;
		// Convert milliseconds from setSoTimeout() call to seconds and microseconds
		tv.tv_usec = optval_int * 1000;
		tv.tv_sec = optval_int / 1000000;
		retval = zts_setsockopt(fd, level, optname, &tv, sizeof(tv));
	}
	else {
		retval = zts_setsockopt(fd, level, optname, &optval_int, sizeof(optval_int));
	}
	return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_getsockopt(
	JNIEnv *env, jobject thisObj, jint fd, jint level, jint optname, jobject optval)
{
	jclass c = env->GetObjectClass(optval);
	if (!c) {
		return ZTS_ERR_SERVICE;
	}
	int optval_int = 0;
	zts_socklen_t optlen; // Intentionally not used

	int retval;

	if (optname == SO_RCVTIMEO) {
		struct zts_timeval tv;
		optlen = sizeof(tv);
		retval = zts_getsockopt(fd, level, optname, &tv, &optlen);
		// Convert seconds and microseconds back to milliseconds
		optval_int = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
	}
	else {
		retval = zts_getsockopt(fd, level, optname, &optval_int, &optlen);
	}

	if (optname == SO_BROADCAST
		|| optname == SO_KEEPALIVE
		|| optname == SO_REUSEADDR
		|| optname == SO_REUSEPORT
		|| optname == TCP_NODELAY)
	{
		jfieldID fid = env->GetFieldID(c, "isBoolean", "Z");
		env->SetBooleanField(optval, fid, true);
		fid = env->GetFieldID(c, "booleanValue", "Z");
		env->SetBooleanField(optval, fid, (bool)optval_int);
	}
	if (optname == IP_TTL
		|| optname == SO_RCVTIMEO
		|| optname == IP_TOS
		|| optname == SO_LINGER
		|| optname == SO_RCVBUF
		|| optname == SO_SNDBUF)
	{
		jfieldID fid = env->GetFieldID(c, "isInteger", "Z");
		env->SetBooleanField(optval, fid, true);
		fid = env->GetFieldID(c, "integerValue", "I");
		env->SetIntField(optval, fid, optval_int);
	}
	return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jboolean JNICALL Java_com_zerotier_libzt_ZeroTier_getsockname(JNIEnv *env, jobject thisObj,
	jint fd, jobject addr)
{
	struct zts_sockaddr_storage ss;
	zts_socklen_t addrlen = sizeof(struct zts_sockaddr_storage);
	int retval = zts_getsockname(fd, (struct zts_sockaddr *)&ss, &addrlen);
	ss2zta(env, &ss, addr);
	return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_getpeername(JNIEnv *env, jobject thisObj,
	jint fd, jobject addr)
{
	struct zts_sockaddr_storage ss;
	int retval = zts_getpeername(fd, (struct zts_sockaddr *)&ss, (zts_socklen_t*)sizeof(struct zts_sockaddr_storage));
	ss2zta(env, &ss, addr);
	return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_close(
	JNIEnv *env, jobject thisObj, jint fd)
{
	return zts_close(fd);
}

JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_select(JNIEnv *env, jobject thisObj,
	jint nfds, jobject readfds, jobject writefds, jobject exceptfds, jint timeout_sec, jint timeout_usec)
{
	struct zts_timeval _timeout;
	_timeout.tv_sec  = timeout_sec;
	_timeout.tv_usec = timeout_usec;
	zts_fd_set _readfds, _writefds, _exceptfds;
	zts_fd_set *r = NULL;
	zts_fd_set *w = NULL;
	zts_fd_set *e = NULL;
	if (readfds) {
		r = &_readfds;
		ztfdset2fdset(env, nfds, readfds, &_readfds);
	}
	if (writefds) {
		w = &_writefds;
		ztfdset2fdset(env, nfds, writefds, &_writefds);
	}
	if (exceptfds) {
		e = &_exceptfds;
		ztfdset2fdset(env, nfds, exceptfds, &_exceptfds);
	}
	int retval = zts_select(nfds, r, w, e, &_timeout);
	if (readfds) {
		fdset2ztfdset(env, nfds, &_readfds, readfds);
	}
	if (writefds) {
		fdset2ztfdset(env, nfds, &_writefds, writefds);
	}
	if (exceptfds) {
		fdset2ztfdset(env, nfds, &_exceptfds, exceptfds);
	}
	return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_fcntl(
	JNIEnv *env, jobject thisObj, jint fd, jint cmd, jint flags)
{
	int retval = zts_fcntl(fd, cmd, flags);
	return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT int JNICALL Java_com_zerotier_libzt_ZeroTier_ioctl(
	JNIEnv *env, jobject thisObj, jint fd, jlong request, jobject argp)
{
	int retval = ZTS_ERR_OK;
	if (request == FIONREAD) {
		int bytesRemaining = 0;
		retval = zts_ioctl(fd, request, &bytesRemaining);
		// set value in general object
		jclass c = env->GetObjectClass(argp);
		if (!c) {
			return ZTS_ERR_ARG;
		}
		jfieldID fid = env->GetFieldID(c, "integer", "I");
		env->SetIntField(argp, fid, bytesRemaining);
	}
	if (request == FIONBIO) {
		// TODO: double check
		int meaninglessVariable = 0;
		retval = zts_ioctl(fd, request, &meaninglessVariable);
	}
	return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_send(
	JNIEnv *env, jobject thisObj, jint fd, jbyteArray buf, int flags)
{
	void *data = env->GetPrimitiveArrayCritical(buf, NULL);
	int retval = zts_send(fd, data, env->GetArrayLength(buf), flags);
	env->ReleasePrimitiveArrayCritical(buf, data, 0);
	return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_sendto(
	JNIEnv *env, jobject thisObj, jint fd, jbyteArray buf, jint flags, jobject addr)
{
	void *data = env->GetPrimitiveArrayCritical(buf, NULL);
	struct zts_sockaddr_storage ss;
	zta2ss(env, &ss, addr);
	zts_socklen_t addrlen = ss.ss_family == ZTS_AF_INET ? sizeof(struct zts_sockaddr_in) : sizeof(struct zts_sockaddr_in6);
	int retval = zts_sendto(fd, data, env->GetArrayLength(buf), flags, (struct zts_sockaddr *)&ss, addrlen);
	env->ReleasePrimitiveArrayCritical(buf, data, 0);
	return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_recv(JNIEnv *env, jobject thisObj,
	jint fd, jbyteArray buf, jint flags)
{
	void *data = env->GetPrimitiveArrayCritical(buf, NULL);
	int retval = zts_recv(fd, data, env->GetArrayLength(buf), flags);
	env->ReleasePrimitiveArrayCritical(buf, data, 0);
	return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_recvfrom(
	JNIEnv *env, jobject thisObj, jint fd, jbyteArray buf, jint flags, jobject addr)
{
	zts_socklen_t addrlen = sizeof(struct zts_sockaddr_storage);
	struct zts_sockaddr_storage ss;
	void *data = env->GetPrimitiveArrayCritical(buf, NULL);
	int retval = zts_recvfrom(fd, data, env->GetArrayLength(buf), flags, (struct zts_sockaddr *)&ss, &addrlen);
	env->ReleasePrimitiveArrayCritical(buf, data, 0);
	ss2zta(env, &ss, addr);
	return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_read(JNIEnv *env, jobject thisObj,
	jint fd, jbyteArray buf)
{
	void *data = env->GetPrimitiveArrayCritical(buf, NULL);
	int retval = zts_read(fd, data, env->GetArrayLength(buf));
	env->ReleasePrimitiveArrayCritical(buf, data, 0);
	return retval > -1 ? retval : -(zts_errno);
}

ssize_t zts_read_offset(int fd, void *buf, size_t offset, size_t len)
{
	if (!(_serviceStateFlags & ZTS_STATE_NET_SERVICE_RUNNING)) {
		return ZTS_ERR_SERVICE;
	}
	if (!buf) {
		return ZTS_ERR_ARG;
	}
	char *cbuf = (char*)buf;
	return lwip_read(fd, &(cbuf[offset]), len);
}

JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_read_1offset(JNIEnv *env, jobject thisObj,
	jint fd, jbyteArray buf, jint offset, jint len)
{
	void *data = env->GetPrimitiveArrayCritical(buf, NULL);
	int retval = zts_read_offset(fd, data, offset, len);
	env->ReleasePrimitiveArrayCritical(buf, data, 0);
	return retval > -1 ? retval : -(zts_errno);
}
JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_read_1length(JNIEnv *env, jobject thisObj,
	jint fd, jbyteArray buf, jint len)
{
	void *data = env->GetPrimitiveArrayCritical(buf, NULL);
	int retval = zts_read(fd, data, len);
	env->ReleasePrimitiveArrayCritical(buf, data, 0);
	return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_write__IB(JNIEnv *env, jobject thisObj,
	jint fd, jbyteArray buf)
{
	void *data = env->GetPrimitiveArrayCritical(buf, NULL);
	int retval = zts_write(fd, data, env->GetArrayLength(buf));
	env->ReleasePrimitiveArrayCritical(buf, data, 0);
	return retval > -1 ? retval : -(zts_errno);
}
JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_write_1offset(JNIEnv *env, jobject thisObj,
	jint fd, jbyteArray buf, jint offset, jint len)
{
	void *data = env->GetPrimitiveArrayCritical(&(buf[offset]), NULL); // PENDING: check?
	int retval = zts_write(fd, data, len);
	env->ReleasePrimitiveArrayCritical(buf, data, 0);
	return retval > -1 ? retval : -(zts_errno);
}
JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_write_1byte(JNIEnv *env, jobject thisObj,
	jint fd, jbyte buf)
{
	int retval = zts_write(fd, &buf, 1);
	return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_shutdown(
	JNIEnv *env, jobject thisObj, int fd, int how)
{
	return zts_shutdown(fd, how);
}

#ifdef ZTS_ENABLE_STATS

JNIEXPORT jint JNICALL Java_com_zerotier_libzt_ZeroTier_get_1protocol_1stats(
	JNIEnv *env, jobject thisObj, jint protocolType, jobject protoStatsObj)
{
	struct stats_proto stats;
	int retval = zts_get_protocol_stats(protocolType, &stats);
	// Copy stats into Java object
	jclass c = env->GetObjectClass(protoStatsObj);
	if (!c) {
		return ZTS_ERR_ARG;
	}
	jfieldID fid;
	fid = env->GetFieldID(c, "xmit", "I");
	env->SetIntField(protoStatsObj, fid, stats.xmit);
	fid = env->GetFieldID(c, "recv", "I");
	env->SetIntField(protoStatsObj, fid, stats.recv);
	fid = env->GetFieldID(c, "fw", "I");
	env->SetIntField(protoStatsObj, fid, stats.fw);
	fid = env->GetFieldID(c, "drop", "I");
	env->SetIntField(protoStatsObj, fid, stats.drop);
	fid = env->GetFieldID(c, "chkerr", "I");
	env->SetIntField(protoStatsObj, fid, stats.chkerr);
	fid = env->GetFieldID(c, "lenerr", "I");
	env->SetIntField(protoStatsObj, fid, stats.lenerr);
	fid = env->GetFieldID(c, "memerr", "I");
	env->SetIntField(protoStatsObj, fid, stats.memerr);
	fid = env->GetFieldID(c, "rterr", "I");
	env->SetIntField(protoStatsObj, fid, stats.rterr);
	fid = env->GetFieldID(c, "proterr", "I");
	env->SetIntField(protoStatsObj, fid, stats.proterr);
	fid = env->GetFieldID(c, "opterr", "I");
	env->SetIntField(protoStatsObj, fid, stats.opterr);
	fid = env->GetFieldID(c, "err", "I");
	env->SetIntField(protoStatsObj, fid, stats.err);
	fid = env->GetFieldID(c, "cachehit", "I");
	env->SetIntField(protoStatsObj, fid, stats.cachehit);
	return retval;
}
#endif // ZTS_ENABLE_STATS

void ztfdset2fdset(JNIEnv *env, int nfds, jobject src_ztfd_set, zts_fd_set *dest_fd_set)
{
	jclass c = env->GetObjectClass(src_ztfd_set);
	if (!c) {
		return;
	}
	ZTS_FD_ZERO(dest_fd_set);
	jfieldID fid = env->GetFieldID(c, "fds_bits", "[B");
	jobject fdData = env->GetObjectField (src_ztfd_set, fid);
	jbyteArray * arr = reinterpret_cast<jbyteArray*>(&fdData);
	char *data = (char*)env->GetByteArrayElements(*arr, NULL);
	for (int i=0; i<nfds; i++) {
		if (data[i] == 0x01)  {
			ZTS_FD_SET(i, dest_fd_set);
		}
	}
	env->ReleaseByteArrayElements(*arr, (jbyte*)data, 0);
	return;
}

void fdset2ztfdset(JNIEnv *env, int nfds, zts_fd_set *src_fd_set, jobject dest_ztfd_set)
{
	jclass c = env->GetObjectClass(dest_ztfd_set);
	if (!c) {
		return;
	}
	jfieldID fid = env->GetFieldID(c, "fds_bits", "[B");
	jobject fdData = env->GetObjectField (dest_ztfd_set, fid);
	jbyteArray * arr = reinterpret_cast<jbyteArray*>(&fdData);
	char *data = (char*)env->GetByteArrayElements(*arr, NULL);
	for (int i=0; i<nfds; i++) {
		if (ZTS_FD_ISSET(i, src_fd_set)) {
			data[i] = 0x01;
		}
	}
	env->ReleaseByteArrayElements(*arr, (jbyte*)data, 0);
	return;
}

//////////////////////////////////////////////////////////////////////////////
// Helpers (for moving data across the JNI barrier)                         //
//////////////////////////////////////////////////////////////////////////////

void ss2zta(JNIEnv *env, struct zts_sockaddr_storage *ss, jobject addr)
{
	jclass c = env->GetObjectClass(addr);
	if (!c) {
		return;
	}
	if(ss->ss_family == ZTS_AF_INET)
	{
		struct zts_sockaddr_in *in4 = (struct zts_sockaddr_in*)ss;
		jfieldID fid = env->GetFieldID(c, "_port", "I");
		env->SetIntField(addr, fid, lwip_ntohs(in4->sin_port));
		fid = env->GetFieldID(c,"_family", "I");
		env->SetIntField(addr, fid, (in4->sin_family));
		fid = env->GetFieldID(c, "_ip4", "[B");
		jobject ipData = env->GetObjectField (addr, fid);
		jbyteArray * arr = reinterpret_cast<jbyteArray*>(&ipData);
		char *data = (char*)env->GetByteArrayElements(*arr, NULL);
		memcpy(data, &(in4->sin_addr.s_addr), 4);
		env->ReleaseByteArrayElements(*arr, (jbyte*)data, 0);

		return;
	}
	if(ss->ss_family == ZTS_AF_INET6)
	{
		struct zts_sockaddr_in6 *in6 = (struct zts_sockaddr_in6*)ss;
		jfieldID fid = env->GetFieldID(c, "_port", "I");
		env->SetIntField(addr, fid, lwip_ntohs(in6->sin6_port));
		fid = env->GetFieldID(c,"_family", "I");
		env->SetIntField(addr, fid, (in6->sin6_family));
		fid = env->GetFieldID(c, "_ip6", "[B");
		jobject ipData = env->GetObjectField (addr, fid);
		jbyteArray * arr = reinterpret_cast<jbyteArray*>(&ipData);
		char *data = (char*)env->GetByteArrayElements(*arr, NULL);
		memcpy(data, &(in6->sin6_addr.s6_addr), 16);
		env->ReleaseByteArrayElements(*arr, (jbyte*)data, 0);
		return;
	}
}

void zta2ss(JNIEnv *env, struct zts_sockaddr_storage *ss, jobject addr)
{
	jclass c = env->GetObjectClass(addr);
	if (!c) {
		return;
	}
	jfieldID fid = env->GetFieldID(c, "_family", "I");
	int family = env->GetIntField(addr, fid);
	if (family == ZTS_AF_INET)
	{
		struct zts_sockaddr_in *in4 = (struct zts_sockaddr_in*)ss;
		fid = env->GetFieldID(c, "_port", "I");
		in4->sin_port = lwip_htons(env->GetIntField(addr, fid));
		in4->sin_family = ZTS_AF_INET;
		fid = env->GetFieldID(c, "_ip4", "[B");
		jobject ipData = env->GetObjectField (addr, fid);
		jbyteArray * arr = reinterpret_cast<jbyteArray*>(&ipData);
		char *data = (char*)env->GetByteArrayElements(*arr, NULL);
		memcpy(&(in4->sin_addr.s_addr), data, 4);
		env->ReleaseByteArrayElements(*arr, (jbyte*)data, 0);
		return;
	}
	if (family == ZTS_AF_INET6)
	{
		struct zts_sockaddr_in6 *in6 = (struct zts_sockaddr_in6*)ss;
		jfieldID fid = env->GetFieldID(c, "_port", "I");
		in6->sin6_port = lwip_htons(env->GetIntField(addr, fid));
		fid = env->GetFieldID(c,"_family", "I");
		in6->sin6_family = ZTS_AF_INET6;
		fid = env->GetFieldID(c, "_ip6", "[B");
		jobject ipData = env->GetObjectField (addr, fid);
		jbyteArray * arr = reinterpret_cast<jbyteArray*>(&ipData);
		char *data = (char*)env->GetByteArrayElements(*arr, NULL);
		memcpy(&(in6->sin6_addr.s6_addr), data, 16);
		env->ReleaseByteArrayElements(*arr, (jbyte*)data, 0);
		return;
	}
}

#ifdef __cplusplus
}
#endif

} // namespace ZeroTier

#endif // ZTS_ENABLE_JAVA

