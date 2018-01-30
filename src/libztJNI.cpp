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
 * Javs JNI wrapper for partially-POSIX-compliant socket API
 * JNI naming convention: Java_PACKAGENAME_CLASSNAME_METHODNAME
 */

#ifdef SDK_JNI

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "libzt.h"
#include "libztDefs.h"

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

namespace ZeroTier {

	// prototype
	jobject ss2inet(JNIEnv *env, struct sockaddr_storage *src_ss);
	int sockinet2ss(JNIEnv *env, jobject src_inet, struct sockaddr_storage *dest_ss);

	/****************************************************************************/
    /* ZeroTier service controls                                                */
    /****************************************************************************/

	JNIEXPORT void JNICALL Java_zerotier_ZeroTier_start(
		JNIEnv *env, jobject thisObj, jstring path, jboolean blocking)
	{
		if (path) {
			zts_start(env->GetStringUTFChars(path, NULL), blocking);
		}
	}

	JNIEXPORT void JNICALL Java_zerotier_ZeroTier_startjoin(
		JNIEnv *env, jobject thisObj, jstring path, jlong nwid)
	{
		if (path && nwid) {
			zts_startjoin(env->GetStringUTFChars(path, NULL), (uint64_t)nwid);
		}
	}

	JNIEXPORT void JNICALL Java_zerotier_ZeroTier_stop(
		JNIEnv *env, jobject thisObj)
	{
		zts_stop();
	}

	JNIEXPORT jboolean JNICALL Java_zerotier_ZeroTier_core_1running(
		JNIEnv *env, jobject thisObj)
	{
		return zts_core_running();
	}

	JNIEXPORT jboolean JNICALL Java_zerotier_ZeroTier_stack_1running(
		JNIEnv *env, jobject thisObj)
	{
		return zts_stack_running();
	}

	JNIEXPORT jboolean JNICALL Java_zerotier_ZeroTier_ready(
		JNIEnv *env, jobject thisObj)
	{
		return zts_ready();
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_join(
		JNIEnv *env, jobject thisObj, jlong nwid)
	{
		return zts_join((uint64_t)nwid);
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_leave(
		JNIEnv *env, jobject thisObj, jlong nwid)
	{
		return zts_leave((uint64_t)nwid);
	}

	JNIEXPORT jstring JNICALL Java_zerotier_ZeroTier_get_1path(
		JNIEnv *env, jobject thisObj)
	{
		char pathBuf[ZT_HOME_PATH_MAX_LEN];
		zts_get_path(pathBuf, ZT_HOME_PATH_MAX_LEN);
		return (*env).NewStringUTF(pathBuf);
	}

	JNIEXPORT jlong JNICALL Java_zerotier_ZeroTier_get_1node_1id(
		JNIEnv *env, jobject thisObj)
	{
		return zts_get_node_id();
	}

	// TODO: ZT_SOCKET_API uint64_t ZTCALL zts_get_node_id_from_file(const char *filepath);

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_get_1num_1assigned_1addresses(
		JNIEnv *env, jobject thisObj, jlong nwid)
	{
		return zts_get_num_assigned_addresses(nwid);
	}

	JNIEXPORT jobject JNICALL Java_zerotier_ZeroTier_get_1address_1at_1index(
		JNIEnv *env, jobject thisObj, jlong nwid, jint index)
	{
		struct sockaddr_storage ss;
		int err;
		if((err = zts_get_address_at_index(nwid, index, &ss)) < 0) {
			return NULL;
		}
		return ss2inet(env, &ss);
	}

	JNIEXPORT jboolean JNICALL Java_zerotier_ZeroTier_has_1address(
		JNIEnv *env, jobject thisObj, jlong nwid)
	{
		return zts_has_address(nwid);
	}

	JNIEXPORT jobject JNICALL Java_zerotier_ZeroTier_get_1address(
		JNIEnv *env, jobject thisObj, jlong nwid, jint address_family)
	{
		struct sockaddr_storage ss;
		int err;
		if ((err = zts_get_address((uint64_t)nwid, &ss, address_family)) < 0) {
			return NULL;
		}
		return ss2inet(env, &ss);
	}

	JNIEXPORT jobject JNICALL Java_zerotier_ZeroTier_get_6plane_addr(
		JNIEnv *env, jobject thisObj, jlong nwid, jlong nodeId)
	{
		struct sockaddr_storage ss;
		zts_get_6plane_addr(&ss, nwid, nodeId);
		return ss2inet(env, &ss);
	}

	JNIEXPORT jobject JNICALL Java_zerotier_ZeroTier_get_rfc4193_addr(
		JNIEnv *env, jobject thisObj, jlong nwid, jlong nodeId)
	{
		struct sockaddr_storage ss;
		zts_get_rfc4193_addr(&ss, nwid, nodeId);
		return ss2inet(env, &ss);
	}

	JNIEXPORT jlong JNICALL Java_zerotier_ZeroTier_get_peer_count(
		JNIEnv *env, jobject thisObj)
	{
		return zts_get_peer_count();
	}

	// TODO: ZT_SOCKET_API int ZTCALL zts_get_peer_address(char *peer, const uint64_t nodeId);
	// TODO: ZT_SOCKET_API void ZTCALL zts_allow_http_control(bool allowed);

	/****************************************************************************/
    /* ZeroTier Socket API                                                      */
    /****************************************************************************/

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_socket(
		JNIEnv *env, jobject thisObj, jint family, jint type, jint protocol)
	{
		return zts_socket(family, type, protocol);
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_connect(
		JNIEnv *env, jobject thisObj, jint fd, jobject addr)
	{
		struct sockaddr_storage ss;
		if(sockinet2ss(env, addr, &ss) < 0) {
			return -1; // possibly invalid address format
			// TODO: set errno
		}
		return zts_connect(fd, (struct sockaddr *)&ss, sizeof(struct sockaddr_storage));
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_bind(
		JNIEnv *env, jobject thisObj, jint fd, jobject addr)
	{
		struct sockaddr_storage ss;
		int err;
		if(sockinet2ss(env, addr, &ss) < 0) {
			return -1; // possibly invalid address format
			// TODO: set errno
		}
		//DEBUG_TEST("RESULT => %s : %d", inet_ntoa(in4->sin_addr), ntohs(in4->sin_port));
		socklen_t addrlen = ss.ss_family == AF_INET ? 4 : 16;
		err = zts_bind(fd, (struct sockaddr*)&ss, addrlen);
		return err;
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_listen(
		JNIEnv *env, jobject thisObj, jint fd, int backlog)
	{
		return zts_listen(fd, backlog);
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_accept(
		JNIEnv *env, jobject thisObj, jint fd, jobject addr, jint port)
	{
		struct sockaddr_storage ss;
		int err;
		socklen_t addrlen = sizeof(struct sockaddr_storage);
		if ((err = zts_accept(fd, (struct sockaddr *)&ss, &addrlen)) < 0) {
			return err;
		}
		addr = ss2inet(env, &ss);
		return err;
	}

#if defined(__linux__)
	 JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_accept4(
	 	JNIEnv *env, jobject thisObj, jint fd, jobject addr, jint port, jint flags)
	 {
		struct sockaddr_storage ss;
		int err;
		socklen_t addrlen = sizeof(struct sockaddr_storage);
		if ((err = zts_accept4(fd, (struct sockaddr *)&ss, &addrlen, flags)) < 0) {
			return err;
		}
		addr = ss2inet(env, &ss);
		return err;
	}
#endif

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_setsockopt(
		JNIEnv *env, jobject thisObj, jint fd, jint level, jint optname, jint optval, jint optlen)
	{
		return zts_setsockopt(fd, level, optname, (void*)(uintptr_t)optval, optlen);
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_getsockopt(
		JNIEnv *env, jobject thisObj, jint fd, jint level, jint optname, jint optval, jint optlen)
	{
		return zts_getsockopt(fd, level, optname, (void*)(uintptr_t)optval, (socklen_t *)optlen);
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_getsockname(JNIEnv *env, jobject thisObj,
		jint fd, jobject ztaddr)
	{
		struct sockaddr_in addr;
		int err = zts_getsockname(fd, (struct sockaddr *)&addr, (socklen_t *)sizeof(struct sockaddr));
		jfieldID fid;
		jclass c = (*env).GetObjectClass(ztaddr);
		if (c) {
			fid = (*env).GetFieldID(c, "port", "I");
			(*env).SetIntField(ztaddr, fid, addr.sin_port);
			fid = (*env).GetFieldID(c,"_rawAddr", "J");
			(*env).SetLongField(ztaddr, fid,addr.sin_addr.s_addr);
		}
		return err;
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_getpeername(JNIEnv *env, jobject thisObj,
		jint fd, jobject ztaddr)
	{
		struct sockaddr_in addr;
		int err = zts_getpeername(fd, (struct sockaddr *)&addr, (socklen_t *)sizeof(struct sockaddr));
		jfieldID fid;
		jclass c = (*env).GetObjectClass( ztaddr);
		if (c) {
			fid = (*env).GetFieldID(c, "port", "I");
			(*env).SetIntField(ztaddr, fid, addr.sin_port);
			fid = (*env).GetFieldID(c,"_rawAddr", "J");
			(*env).SetLongField(ztaddr, fid,addr.sin_addr.s_addr);
		}
		return err;
	}

	// TODO: ZT_SOCKET_API struct hostent *zts_gethostbyname(const char *name);

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_close(
		JNIEnv *env, jobject thisObj, jint fd)
	{
		return zts_close(fd);
	}

	// TODO: ZT_SOCKET_API int ZTCALL zts_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_fcntl(
		JNIEnv *env, jobject thisObj, jint fd, jint cmd, jint flags)
	{
		return zts_fcntl(fd, cmd, flags);
	}

	// TODO: ZT_SOCKET_API int ZTCALL zts_ioctl(int fd, unsigned long request, void *argp);

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_send(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len, int flags)
	{
		jbyte *body = (*env).GetByteArrayElements((_jbyteArray *)buf, 0);
		char * bufp = (char *)malloc(sizeof(char)*len);
		memcpy(bufp, body, len);
		(*env).ReleaseByteArrayElements((_jbyteArray *)buf, body, 0);
		int written_bytes = zts_write(fd, body, len);
		return written_bytes;
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_sendto(
		JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len, jint flags, jobject ztaddr)
	{
		struct sockaddr_in addr;
		int sent_bytes = 0;
		jclass c = (*env).GetObjectClass( ztaddr);
		if (c) {
			jfieldID f = (*env).GetFieldID(c, "port", "I");
			addr.sin_port = htons((*env).GetIntField( ztaddr, f));
			f = (*env).GetFieldID(c, "_rawAddr", "J");
			addr.sin_addr.s_addr = (*env).GetLongField( ztaddr, f);
			addr.sin_family = AF_INET;
			//LOGV("zt_sendto(): fd = %d\naddr = %s\nport=%d", fd, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
			// TODO: Optimize this
			jbyte *body = (*env).GetByteArrayElements((_jbyteArray *)buf, 0);
			char * bufp = (char *)malloc(sizeof(char)*len);
			memcpy(bufp, body, len);
			(*env).ReleaseByteArrayElements((_jbyteArray *)buf, body, 0);
			// "connect" and send buffer contents
			sent_bytes = zts_sendto(fd, body, len, flags, (struct sockaddr *)&addr, sizeof(addr));
		}
		return sent_bytes;
	}

	// TODO: ZT_SOCKET_API ssize_t ZTCALL zts_sendmsg(int fd, const struct msghdr *msg, int flags);
	// TODO: ZT_SOCKET_API ssize_t ZTCALL zts_recv(int fd, void *buf, size_t len, int flags);

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_recvfrom(
		JNIEnv *env, jobject thisObj, jint fd, jbyteArray buf, jint len, jint flags, jobject ztaddr)
	{
	/*
		struct sockaddr_in addr;
		jbyte *body = (*env).GetByteArrayElements( buf, 0);
		unsigned char buffer[ZT_SDK_MTU];
		int payload_offset = sizeof(int32_t) + sizeof(struct sockaddr_storage);
		int rxbytes = zts_recvfrom(fd, &buffer, len, flags, (struct sockaddr *)&addr, (socklen_t *)sizeof(struct sockaddr_storage));
		if (rxbytes > 0) {
			memcpy(body, (jbyte*)buffer + payload_offset, rxbytes);
		}
		(*env).ReleaseByteArrayElements( buf, body, 0);
		// Update fields of Java ZTAddress object
		jfieldID fid;
		jclass c = (*env).GetObjectClass( ztaddr);
		if (c) {
			fid = (*env).GetFieldID(c, "port", "I");
			(*env).SetIntField(ztaddr, fid, addr.sin_port);
			fid = (*env).GetFieldID(c,"_rawAddr", "J");
			(*env).SetLongField(ztaddr, fid,addr.sin_addr.s_addr);
		}
		*/
		return 1;
	}

	// TODO: ZT_SOCKET_API ssize_t ZTCALL zts_recvmsg(int fd, struct msghdr *msg,int flags);

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_read(JNIEnv *env, jobject thisObj,
		jint fd, jarray buf, jint len)
	{
		jbyte *body = (*env).GetByteArrayElements((_jbyteArray *)buf, 0);
		int read_bytes = read(fd, body, len);
		(*env).ReleaseByteArrayElements((_jbyteArray *)buf, body, 0);
		return read_bytes;
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_write(JNIEnv *env, jobject thisObj,
		jint fd, jarray buf, jint len)
	{
		jbyte *body = (*env).GetByteArrayElements((_jbyteArray *)buf, 0);
		char * bufp = (char *)malloc(sizeof(char)*len);
		memcpy(bufp, body, len);
		(*env).ReleaseByteArrayElements((_jbyteArray *)buf, body, 0);
		int written_bytes = zts_write(fd, body, len);
		return written_bytes;
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_shutdown(
		JNIEnv *env, jobject thisObj, int fd, int how)
	{
		return zts_shutdown(fd, how);
	}

	// TODO: ZT_SOCKET_API int ZTCALL zts_add_dns_nameserver(struct sockaddr *addr);
	// TODO: ZT_SOCKET_API int ZTCALL zts_del_dns_nameserver(struct sockaddr *addr);	
}


// convenience function
jobject ss2inet(JNIEnv *env, struct sockaddr_storage *src_ss)
{
	jobject dest_inet;
	if(src_ss->ss_family == AF_INET)
	{
		DEBUG_ERROR("converting from INET");
		struct sockaddr_in *in4 = (struct sockaddr_in*)src_ss;
		int arrlen = 4;
		jbyteArray bytes = (*env).NewByteArray(arrlen);
		jbyte *java_address_bytes;
		java_address_bytes = (*env).GetByteArrayElements(bytes, NULL);
		memcpy(java_address_bytes, &(in4->sin_addr.s_addr), arrlen);
		(*env).ReleaseByteArrayElements(bytes, java_address_bytes, 0);
		jclass cls = (*env).FindClass("java/net/InetAddress");
		jmethodID mid = (*env).GetStaticMethodID(cls, "getByAddress", "([B)Ljava/net/InetAddress;");
		dest_inet = (*env).CallStaticObjectMethod(cls, mid, bytes);
		(*env).DeleteLocalRef(bytes);
	}
	if(src_ss->ss_family == AF_INET6)
	{
		DEBUG_ERROR("converting from INET6");
		struct sockaddr_in6 *in6 = (struct sockaddr_in6*)src_ss;
		int arrlen = 16;
		jbyteArray bytes = (*env).NewByteArray(arrlen);
		(*env).SetByteArrayRegion(bytes, 0, 16, (const jbyte *)&(in6->sin6_addr));
		jclass cls = (*env).FindClass("java/net/InetAddress");
		jmethodID mid = (*env).GetStaticMethodID(cls, "getByAddress", "([B)Ljava/net/InetAddress;");
		dest_inet = (*env).CallStaticObjectMethod(cls, mid, bytes);
		(*env).DeleteLocalRef(bytes);
	}
	return dest_inet;
}


int sockinet2ss(JNIEnv *env, jobject src_inet, struct sockaddr_storage *dest_ss)
{
	struct sockaddr_in *in4 = (struct sockaddr_in*)dest_ss;
	struct sockaddr_in6 *in6 = (struct sockaddr_in6*)dest_ss;
	int port = 0;
	int socket_family = 0;
	socklen_t addrlen;

	// ---

	jclass c = (*env).GetObjectClass(src_inet);
	if (!c) {
		return -1;
	}
	// get port
	jmethodID getPort = (*env).GetMethodID(c, "getPort", "()I");
	if (!getPort) {
		return -1;
	}
	port = (*env).CallIntMethod(src_inet, getPort);	
	// get internal InetAddress
	jobject inetaddr;
	jmethodID getAddress = (*env).GetMethodID(c, "getAddress", "()Ljava/net/InetAddress;");
	if (!getAddress) {
		return -1;
	}
	inetaddr = (*env).CallObjectMethod(src_inet, getAddress);	
	if (!inetaddr) {
		return -1;
	}
	jclass inetClass = (*env).GetObjectClass(inetaddr);
	if (!inetClass) {
		return -1;
	}
	// string representation of IP address
	jmethodID getHostAddress = (*env).GetMethodID(inetClass, "getHostAddress", "()Ljava/lang/String;");
	jstring addrstr = (jstring)(*env).CallObjectMethod(inetaddr, getHostAddress);	
	const char *addr_str = (*env).GetStringUTFChars(addrstr, NULL);
	for (int i=0; i<strlen(addr_str); i++) {
		if (addr_str[i]=='.') {
			DEBUG_INFO("ipv4, inet_addr");
			socket_family = AF_INET;
			in4->sin_family = AF_INET;
			in4->sin_port = htons(port);
			in4->sin_addr.s_addr = inet_addr(addr_str);
			/*
			if (!inet_pton(AF_INET, addr_str, &(in4->sin_addr))) {
				DEBUG_ERROR("error converting address %s", addr_str);
			}
			*/
			addrlen = sizeof(struct sockaddr_in);
			break;
		}
		if (addr_str[i]==':') {
			DEBUG_INFO("ipv6");
			socket_family = AF_INET6;
			if (!inet_pton(AF_INET6, addr_str, &(in6->sin6_addr))) {									
				DEBUG_ERROR("error converting address %s", addr_str);
			}
			addrlen = sizeof(struct sockaddr_in6);
			break;
		}
	}
	(*env).ReleaseStringUTFChars(addrstr, addr_str);
	DEBUG_TEST("RESULT => %s : %d", inet_ntoa(in4->sin_addr), ntohs(in4->sin_port));
	return 0;
}

#ifdef __cplusplus
}
#endif

#endif // SDK_JNI