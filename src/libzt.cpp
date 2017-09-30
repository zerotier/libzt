/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2017  ZeroTier, Inc.  https://www.zerotier.com/
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
 * Application-facing, partially-POSIX-compliant socket API
 */

#include <cstring>

#if defined(STACK_LWIP)
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"
#endif
#if defined(NO_STACK)
#include <sys/socket.h>
#endif

#include "libzt.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(STACK_LWIP)
void sys2lwip(int fd, const struct sockaddr *orig, struct sockaddr *modified) 
{
	/* Inelegant fix for lwIP 'sequential' API address error check (in sockets.c). For some reason
	lwIP seems to lose track of the sa_family for the socket internally, when lwip_connect()
	is called, it thus receives an AF_UNSPEC socket which fails. Here we use lwIP's own facilities
	to get the sa_family ourselves and rebuild the address structure and pass it to lwip_connect().
	I suspect this is due to a struct memory alignment issue or my own misuse of the API */
	struct sockaddr_storage ss;
	socklen_t namelen = sizeof(ss);
	int err = 0;
	if ((err = lwip_getsockname(fd, (struct sockaddr*)&ss, &namelen)) < 0) {
		DEBUG_ERROR("error while determining socket family");
		return;
	}
#if defined(LIBZT_IPV4)
	if (ss.ss_family == AF_INET) {
#if defined(__linux__)
		struct sockaddr_in *p4 = (struct sockaddr_in *)modified;
		struct sockaddr_in *addr4 = (struct sockaddr_in*)orig;
		p4->sin_len = sizeof(struct sockaddr_in);
		p4->sin_family = ss.ss_family;
		p4->sin_port = addr4->sin_port;
		p4->sin_addr.s_addr = addr4->sin_addr.s_addr;
#endif // __linux__
		memcpy(modified, orig, sizeof(struct sockaddr_in));
	}
#endif // LIBZT_IPV4

#if defined(LIBZT_IPV6)
	if (ss.ss_family == AF_INET6) {
#if defined(__linux__)
		struct sockaddr_in6 *p6 = (struct sockaddr_in6 *)modified;
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6*)orig;
		p6->sin6_len = sizeof(struct sockaddr_in6);
		p6->sin6_family = ss.ss_family;
		p6->sin6_port = addr6->sin6_port;
		//p6->sin6_addr.s6_addr = addr6->sin6_addr.s6_addr;
#endif // __linux__
		memcpy(modified, orig, sizeof(struct sockaddr_in6));
	}
#endif // LIBZT_IPV6

}
#endif // STACK_LWIP

int zts_socket(int socket_family, int socket_type, int protocol)
{
	int err = -1;
	DEBUG_EXTRA("family=%d, type=%d, proto=%d", socket_family, socket_type, protocol);
#if defined(STACK_LWIP)
	err = lwip_socket(socket_family, socket_type, protocol);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_connect(int fd, const struct sockaddr *addr, socklen_t addrlen)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d",fd);
#if defined(STACK_LWIP)
	struct sockaddr_storage ss;
	sys2lwip(fd, addr, (struct sockaddr*)&ss);
	err = lwip_connect(fd, (struct sockaddr*)&ss, addrlen);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_bind(int fd, const struct sockaddr *addr, socklen_t addrlen)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d", fd);
#if defined(STACK_LWIP)
	struct sockaddr_storage ss;
	sys2lwip(fd, addr, (struct sockaddr*)&ss);
	err = lwip_bind(fd, (struct sockaddr*)&ss, addrlen);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_listen(int fd, int backlog)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d", fd);
#if defined(STACK_LWIP)
	err = lwip_listen(fd, backlog);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_accept(int fd, struct sockaddr *addr, socklen_t *addrlen)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d", fd);
#if defined(STACK_LWIP)
	err = lwip_accept(fd, addr, addrlen);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

#if defined(__linux__)
int zts_accept4(int fd, struct sockaddr *addr, socklen_t *addrlen, int flags)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d", fd);
#if defined(STACK_LWIP)
	err = zts_accept(fd, addr, addrlen);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}
#endif

int zts_setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d, level=%d, optname=%d", fd, level, optname);
#if defined(STACK_LWIP)
	err = lwip_setsockopt(fd, level, optname, optval, optlen);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_getsockopt(int fd, int level, int optname, void *optval, socklen_t *optlen)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d, level=%d, optname=%d", fd, level, optname);
#if defined(STACK_LWIP)
	err = lwip_getsockopt(fd, level, optname, optval, optlen);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_getsockname(int fd, struct sockaddr *addr, socklen_t *addrlen)
{
	int err = -1;
	DEBUG_EXTRA("fd=%p", fd);
#if defined(STACK_LWIP)
	err = lwip_getsockname(fd, addr, addrlen);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_getpeername(int fd, struct sockaddr *addr, socklen_t *addrlen)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d", fd);
#if defined(STACK_LWIP)
	err = lwip_getpeername(fd, addr, addrlen);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_gethostname(char *name, size_t len)
{
	DEBUG_EXTRA();
	int err = -1;
#if defined(STACK_LWIP)
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_sethostname(const char *name, size_t len)
{
	DEBUG_EXTRA();
	int err = -1;
#if defined(STACK_LWIP)
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_close(int fd)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d", fd);
#if defined(STACK_LWIP)
	err = lwip_close(fd);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	int err = -1;
#if defined(STACK_LWIP)
	DEBUG_ERROR("warning, this is not implemented");
	return poll(fds, nfds, timeout);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, 
	struct timeval *timeout)
{
	int err = -1;
	//DEBUG_EXTRA();
#if defined(STACK_LWIP)
	err = lwip_select(nfds, readfds, writefds, exceptfds, timeout);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_fcntl(int fd, int cmd, int flags)
{
	int err = -1;
	DEBUG_EXTRA("fd=%p, cmd=%d, flags=%d", cmd, flags);
#if defined(STACK_LWIP)
	// translation required since lwIP uses different flag values
	int translated_flags = 0;
	if (flags == 2048) {
		translated_flags = 1;
	}
	err = lwip_fcntl(fd, cmd, translated_flags);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_ioctl(int fd, unsigned long request, void *argp)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d, req=%d", fd, request);
#if defined(STACK_LWIP)
	err = lwip_ioctl(fd, request, argp);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

ssize_t zts_sendto(int fd, const void *buf, size_t len, int flags, 
	const struct sockaddr *addr, socklen_t addrlen)
{
	int err = -1;
	DEBUG_TRANS("fd=%d, len=%d", fd, len);
#if defined(STACK_LWIP)
	struct sockaddr_storage ss;
	sys2lwip(fd, addr, (struct sockaddr*)&ss);
	err = lwip_sendto(fd, buf, len, flags, (struct sockaddr*)&ss, addrlen);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

ssize_t zts_send(int fd, const void *buf, size_t len, int flags)
{
	int err = -1;
	DEBUG_TRANS("fd=%d, len=%d", fd, len);
#if defined(STACK_LWIP)
	err = lwip_send(fd, buf, len, flags);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

ssize_t zts_sendmsg(int fd, const struct msghdr *msg, int flags)
{
	int err = -1;
	DEBUG_TRANS("fd=%d", fd);
#if defined(STACK_LWIP)
	err = lwip_sendmsg(fd, msg, flags);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

ssize_t zts_recv(int fd, void *buf, size_t len, int flags)
{
	int err = -1;
	DEBUG_TRANS("fd=%d", fd);
#if defined(STACK_LWIP)
	err = lwip_recv(fd, buf, len, flags);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

ssize_t zts_recvfrom(int fd, void *buf, size_t len, int flags, 
	struct sockaddr *addr, socklen_t *addrlen)
{
	int err = -1;
	DEBUG_TRANS("fd=%d", fd);
#if defined(STACK_LWIP)
	err = lwip_recvfrom(fd, buf, len, flags, addr, addrlen);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

ssize_t zts_recvmsg(int fd, struct msghdr *msg,int flags)
{
	DEBUG_TRANS("fd=%d", fd);
	int err = -1;
#if defined(STACK_LWIP)
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_read(int fd, void *buf, size_t len)
{
	int err = -1;
	//DEBUG_TRANS("fd=%d, len=%d", fd, len);
#if defined(STACK_LWIP)
	err = lwip_read(fd, buf, len);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_write(int fd, const void *buf, size_t len) 
{
	//DEBUG_TRANS("fd=%d, len=%d", fd, len);
	int err = -1;
#if defined(STACK_LWIP)
	err = lwip_write(fd, buf, len);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_shutdown(int fd, int how)
{
	int err = -1;
	DEBUG_EXTRA("fd=%d, how=%d", fd, how);
#if defined(STACK_LWIP)
	err = lwip_shutdown(fd, how);
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_add_dns_nameserver(struct sockaddr *addr)
{
	DEBUG_EXTRA();
	int err = -1;
#if defined(STACK_LWIP)
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}

int zts_del_dns_nameserver(struct sockaddr *addr)
{
	DEBUG_EXTRA();
	int err = -1;
#if defined(STACK_LWIP)
#endif
#if defined(STCK_PICO)
#endif
#if defined(NO_STACK)
#endif
	return err;
}


/****************************************************************************/
/* SDK Socket API (Java Native Interface JNI)                               */
/* JNI naming convention: Java_PACKAGENAME_CLASSNAME_METHODNAME             */
/****************************************************************************/

#if defined(SDK_JNI)

namespace ZeroTier {

	#include <jni.h>

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1sendto(
		JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len, jint flags, jobject ztaddr)
	{
		struct sockaddr_in addr;
		jclass cls = (*env).GetObjectClass( ztaddr);
		jfieldID f = (*env).GetFieldID( cls, "port", "I");
		addr.sin_port = htons((*env).GetIntField( ztaddr, f));
		f = (*env).GetFieldID( cls, "_rawAddr", "J");
		addr.sin_addr.s_addr = (*env).GetLongField( ztaddr, f);
		addr.sin_family = AF_INET;
		//LOGV("zt_sendto(): fd = %d\naddr = %s\nport=%d", fd, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
		// TODO: Optimize this
		jbyte *body = (*env).GetByteArrayElements((_jbyteArray *)buf, 0);
		char * bufp = (char *)malloc(sizeof(char)*len);
		memcpy(bufp, body, len);
		(*env).ReleaseByteArrayElements((_jbyteArray *)buf, body, 0);
		// "connect" and send buffer contents
		int sent_bytes = zts_sendto(fd, body, len, flags, (struct sockaddr *)&addr, sizeof(addr));
		return sent_bytes;
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1recvfrom(
		JNIEnv *env, jobject thisObj, jint fd, jbyteArray buf, jint len, jint flags, jobject ztaddr)
	{
		struct sockaddr_in addr;
		jbyte *body = (*env).GetByteArrayElements( buf, 0);
		unsigned char buffer[ZT_SDK_MTU];
		int payload_offset = sizeof(int32_t) + sizeof(struct sockaddr_storage);
		int rxbytes = zts_recvfrom(fd, &buffer, len, flags, (struct sockaddr *)&addr, (socklen_t *)sizeof(struct sockaddr_storage));
		if (rxbytes > 0)
			memcpy(body, (jbyte*)buffer + payload_offset, rxbytes);
		(*env).ReleaseByteArrayElements( buf, body, 0);
		// Update fields of Java ZTAddress object
		jfieldID fid;
		jclass cls = (*env).GetObjectClass( ztaddr);
		fid = (*env).GetFieldID( cls, "port", "I");
		(*env).SetIntField( ztaddr, fid, addr.sin_port);
		fid = (*env).GetFieldID( cls,"_rawAddr", "J");
		(*env).SetLongField( ztaddr, fid,addr.sin_addr.s_addr);
		return rxbytes;
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1send(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len, int flags)
	{
		jbyte *body = (*env).GetByteArrayElements((_jbyteArray *)buf, 0);
		char * bufp = (char *)malloc(sizeof(char)*len);
		memcpy(bufp, body, len);
		(*env).ReleaseByteArrayElements((_jbyteArray *)buf, body, 0);
		int written_bytes = zts_write(fd, body, len);
		return written_bytes;
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1write(JNIEnv *env, jobject thisObj, 
		jint fd, jarray buf, jint len)
	{
		jbyte *body = (*env).GetByteArrayElements((_jbyteArray *)buf, 0);
		char * bufp = (char *)malloc(sizeof(char)*len);
		memcpy(bufp, body, len);
		(*env).ReleaseByteArrayElements((_jbyteArray *)buf, body, 0);
		int written_bytes = zts_write(fd, body, len);
		return written_bytes;
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1read(JNIEnv *env, jobject thisObj, 
		jint fd, jarray buf, jint len)
	{
		jbyte *body = (*env).GetByteArrayElements((_jbyteArray *)buf, 0);
		int read_bytes = read(fd, body, len);
		(*env).ReleaseByteArrayElements((_jbyteArray *)buf, body, 0);
		return read_bytes;
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1setsockopt(
		JNIEnv *env, jobject thisObj, 
		jint fd, jint level, jint optname, jint optval, jint optlen) {
		return zts_setsockopt(fd, level, optname, (const void*)optval, optlen);
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1getsockopt(JNIEnv *env, jobject thisObj, 
		jint fd, jint level, jint optname, jint optval, jint optlen) {
		return zts_getsockopt(fd, level, optname, (void*)optval, (socklen_t *)optlen);
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1socket(JNIEnv *env, jobject thisObj, 
		jint family, jint type, jint protocol) {
		return zts_socket(family, type, protocol);
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1connect(JNIEnv *env, jobject thisObj, 
		jint fd, jstring addrstr, jint port) {
		struct sockaddr_in addr;
		const char *str = (*env).GetStringUTFChars( addrstr, 0);
		addr.sin_addr.s_addr = inet_addr(str);
		addr.sin_family = AF_INET;
		addr.sin_port = htons( port );
		(*env).ReleaseStringUTFChars( addrstr, str);
		return zts_connect(fd, (struct sockaddr *)&addr, sizeof(addr));
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1bind(JNIEnv *env, jobject thisObj, 
		jint fd, jstring addrstr, jint port) {
		struct sockaddr_in addr;
		const char *str = (*env).GetStringUTFChars( addrstr, 0);
		DEBUG_INFO("fd=%d, addr=%s, port=%d", fd, str, port);
		addr.sin_addr.s_addr = inet_addr(str);
		addr.sin_family = AF_INET;
		addr.sin_port = htons( port );
		(*env).ReleaseStringUTFChars( addrstr, str);
		return zts_bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	}

#if defined(__linux__)
	 JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1accept4(JNIEnv *env, jobject thisObj, 
		jint fd, jstring addrstr, jint port, jint flags) {
		struct sockaddr_in addr;
		char *str;
		// = env->GetStringUTFChars(addrstr, NULL);
		(*env).ReleaseStringUTFChars( addrstr, str);
		addr.sin_addr.s_addr = inet_addr(str);
		addr.sin_family = AF_INET;
		addr.sin_port = htons( port );
		return zts_accept4(fd, (struct sockaddr *)&addr, sizeof(addr), flags);
	}
#endif

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1accept(JNIEnv *env, jobject thisObj, 
		jint fd, jstring addrstr, jint port) {
		struct sockaddr_in addr;
		// TODO: Send addr info back to Javaland
		addr.sin_addr.s_addr = inet_addr("");
		addr.sin_family = AF_INET;
		addr.sin_port = htons( port );
		return zts_accept(fd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr));
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1listen(JNIEnv *env, jobject thisObj, 
		jint fd, int backlog) {
		return zts_listen(fd, backlog);
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1close(JNIEnv *env, jobject thisObj, 
		jint fd) {
		return zts_close(fd);
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1getsockname(JNIEnv *env, jobject thisObj, 
		jint fd, jobject ztaddr) {
		struct sockaddr_in addr;
		int err = zts_getsockname(fd, (struct sockaddr *)&addr, (socklen_t *)sizeof(struct sockaddr));
		jfieldID fid;
		jclass cls = (*env).GetObjectClass(ztaddr);
		fid = (*env).GetFieldID( cls, "port", "I");
		(*env).SetIntField( ztaddr, fid, addr.sin_port);
		fid = (*env).GetFieldID( cls,"_rawAddr", "J");
		(*env).SetLongField( ztaddr, fid,addr.sin_addr.s_addr);
		return err;
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1getpeername(JNIEnv *env, jobject thisObj, 
		jint fd, jobject ztaddr) {
		struct sockaddr_in addr;
		int err = zts_getpeername(fd, (struct sockaddr *)&addr, (socklen_t *)sizeof(struct sockaddr));
		jfieldID fid;
		jclass cls = (*env).GetObjectClass( ztaddr);
		fid = (*env).GetFieldID( cls, "port", "I");
		(*env).SetIntField( ztaddr, fid, addr.sin_port);
		fid = (*env).GetFieldID( cls,"_rawAddr", "J");
		(*env).SetLongField( ztaddr, fid,addr.sin_addr.s_addr);
		return err;
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1fcntl(JNIEnv *env, jobject thisObj, 
		jint fd, jint cmd, jint flags) {
		return zts_fcntl(fd,cmd,flags);
	}
}
#endif // SDK_JNI

#ifdef __cplusplus
}
#endif
