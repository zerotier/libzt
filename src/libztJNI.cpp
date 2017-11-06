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
 * Javs JNI wrapper for partially-POSIX-compliant socket API
 * JNI naming convention: Java_PACKAGENAME_CLASSNAME_METHODNAME
 */

#include <sys/socket.h>

#include "libzt.h"
#include "ZT1Service.h"

#ifdef __cplusplus
extern "C" {
#endif

namespace ZeroTier {

	#include <jni.h>

	/****************************************************************************/
    /* ZeroTier Socket API (for JNI wrapper)                                    */
    /****************************************************************************/

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_socket(JNIEnv *env, jobject thisObj,
		jint family, jint type, jint protocol)
	{
		return zts_socket(family, type, protocol);
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_connect(JNIEnv *env, jobject thisObj,
		jint fd, jstring addrstr, jint port)
	{
		struct sockaddr_storage ss;
		socklen_t namelen = sizeof(ss);
		int err = 0;
		if ((err = zts_getsockname(fd, (struct sockaddr*)&ss, &namelen)) < 0) {
			DEBUG_ERROR("error while determining socket family");
			return -1;
		}
		const char *str;
#if defined(LIBZT_IPV4)
		if (ss.ss_family == AF_INET) {
			struct sockaddr_in in_addr;
			str = (*env).GetStringUTFChars(addrstr, 0);
			in_addr.sin_addr.s_addr = inet_addr(str);
			in_addr.sin_family = AF_INET;
			in_addr.sin_port = htons(port);
			(*env).ReleaseStringUTFChars(addrstr, str);
		}
#endif // LIBZT_IPV4
#if defined(LIBZT_IPV6)
		if (ss.ss_family == AF_INET6) {
			struct sockaddr_in6 in_addr;
			str = (*env).GetStringUTFChars(addrstr, 0);
			//in_addr.sin_addr.s_addr = inet_addr(str);
			in_addr.sin6_family = AF_INET6;
			in_addr.sin6_port = htons(port);
			(*env).ReleaseStringUTFChars(addrstr, str);
		}
#endif // LIBZT_IPV6
		DEBUG_INFO("fd=%d, addr=%s, port=%d", fd, str, port);
		return zts_connect(fd, (struct sockaddr *)&ss, sizeof(in_addr));
	}

/*

	JNIEXPORT jobject JNICALL Java_zerotier_ZeroTier_get_address(
		JNIEnv *env, jobject thisObj, jstring nwid)
	{
		// get address
		const char *nwid_str = env->GetStringUTFChars(nwid, NULL);
		struct sockaddr_storage ss;
		zts_get_address(nwid_str, &ss, INET_ADDRSTRLEN);
		// map sockaddr_storage contents to java InetAddress
		jclass c = (*env).FindClass("java/net/InetSocketAddress");
		if (c) {

			jobject addresses = (*env).NewObject(c, (*env).GetMethodID(c, "<init>", "()V"));
			env->CallBooleanMethod(addresses, env->GetMethodID(c, "getPort", "(Ljava/lang/Object;)Z"), _str);	

			DEBUG_INFO("port from JNI layer = %d", port);
			
			fid = (*env).GetFieldID(c, "port", "I");
			(*env).SetIntField(ztaddr, fid, addr.sin_port);
			fid = (*env).GetFieldID(c,"_rawAddr", "J");
			(*env).SetLongField(ztaddr, fid,addr.sin_addr.s_addr);
			//jobject addresses = (*env).NewObject(c, (*env).GetMethodID(c, "<init>", "()V"));
			//env->CallBooleanMethod(addresses, env->GetMethodID(c, "add", "(Ljava/lang/Object;)Z"), _str);	
			//return addresses;
		}
		return NULL;
	}

*/


/*
	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_bind(JNIEnv *env, jobject thisObj,
		jint fd, jstring addrstr, jint port)
	{
		struct sockaddr_storage ss;
		socklen_t namelen = sizeof(ss);
		int err = 0;
		if ((err = zts_getsockname(fd, (struct sockaddr*)&ss, &namelen)) < 0) {
			DEBUG_ERROR("error while determining socket family");
			return -1;
		}
		const char *str;
#if defined(LIBZT_IPV4)
		if (ss.ss_family == AF_INET) {
			struct sockaddr_in in_addr;
			str = (*env).GetStringUTFChars(addrstr, 0);
			in_addr.sin_addr.s_addr = inet_addr(str);
			in_addr.sin_family = AF_INET;
			in_addr.sin_port = htons(port);
			(*env).ReleaseStringUTFChars(addrstr, str);
		}
#endif // LIBZT_IPV4
#if defined(LIBZT_IPV6)
		if (ss.ss_family == AF_INET6) {
			struct sockaddr_in6 in_addr;
			str = (*env).GetStringUTFChars(addrstr, 0);
			//in_addr.sin_addr.s_addr = inet_addr(str);
			in_addr.sin6_family = AF_INET6;
			in_addr.sin6_port = htons(port);
			(*env).ReleaseStringUTFChars(addrstr, str);
		}
#endif // LIBZT_IPV6
		DEBUG_INFO("fd=%d, addr=%s, port=%d", fd, str, port);
		return zts_bind(fd, (struct sockaddr *)&ss, sizeof(in_addr));
	}
*/




/*
	jfieldID IPv4 = (*env).GetFieldID(inetClass, "IPv4", "I");
						if (IPv4) {
							ipv4 = (*env).GetIntField(inetClass, IPv4);
						}
						else {
							DEBUG_ERROR("No field IPv4");
						}
						jfieldID IPv6 = (*env).GetFieldID(inetClass, "IPv6", "I");
						if (IPv6) {
							ipv6 = (*env).GetIntField(inetClass, IPv6);
						}
						else {
							DEBUG_ERROR("No field IPv6");
						}
						ipv6 = (*env).GetIntField(inetClass, IPv6);
*/	

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_bind(JNIEnv *env, jobject thisObj, jint fd, jobject addr)
	{
		struct sockaddr_storage ss;
		memset(&ss, 0, sizeof(ss));
		struct sockaddr_in *in4 = (struct sockaddr_in*)&ss;
		struct sockaddr_in6 *in6 = (struct sockaddr_in6*)&ss;
		int err = -1;
		int port = 0;
		int socket_family = 0;
		socklen_t addrlen;

		jclass c = (*env).GetObjectClass(addr);
		if (!c) {
			return -1;
		}
		// get port
		jmethodID getPort = (*env).GetMethodID(c, "getPort", "()I");
		if (!getPort) {
			return -1;
		}
		port = (*env).CallIntMethod(addr, getPort);	
		// get internal InetAddress
		jobject inetaddr;
		int family = 0;
		jmethodID getAddress = (*env).GetMethodID(c, "getAddress", "()Ljava/net/InetAddress;");
		if (!getAddress) {
			return -1;
		}
		inetaddr = (*env).CallObjectMethod(addr, getAddress);	
		if(!inetaddr) {
			return -1;
		}
		jclass inetClass = (*env).GetObjectClass(inetaddr);
		if(!inetClass) {
			return -1;
		}
		// string representation of IP address
		jmethodID getHostAddress = (*env).GetMethodID(inetClass, "getHostAddress", "()Ljava/lang/String;");
		jstring addrstr = (jstring)(*env).CallObjectMethod(inetaddr, getHostAddress);	
		const char *addr_str = (*env).GetStringUTFChars(addrstr, NULL);
		DEBUG_INFO("addr_str=%s", addr_str);
		for (int i=0; i<strlen(addr_str); i++) {
			if(addr_str[i]=='.') {
				DEBUG_INFO("ipv4, inet_addr");
				socket_family = AF_INET;
				in4->sin_family = AF_INET;
				in4->sin_port = htons(port);
				in4->sin_addr.s_addr = inet_addr(addr_str);
				/*
				if(!inet_pton(AF_INET, addr_str, &(in4->sin_addr))) {
					DEBUG_ERROR("error converting address %s", addr_str);
				}
				*/
				addrlen = sizeof(struct sockaddr_in);
				break;
			}
			if(addr_str[i]==':') {
				DEBUG_INFO("ipv6");
				socket_family = AF_INET6;
				if(!inet_pton(AF_INET6, addr_str, &(in6->sin6_addr))) {									
					DEBUG_ERROR("error converting address %s", addr_str);
				}
				addrlen = sizeof(struct sockaddr_in6);
				break;
			}
		}
		(*env).ReleaseStringUTFChars(addrstr, addr_str);

		DEBUG_TEST("RESULT => %s : %d", inet_ntoa(in4->sin_addr), ntohs(in4->sin_port));

		// 
		/*
		jmethodID getRawAddress = (*env).GetMethodID(inetClass, "getAddress", "()[B"); // get raw IP bytes
		if (getRawAddress) 
		{
			jbyte rawIPbuf = (*env).CallByteMethod(inetaddr, getRawAddress);
			if (socket_family == AF_INET) 
			{
				DEBUG_INFO("copying buffer as AF_INET");
				in4->sin_family = socket_family;
				in4->sin_port = port;
				memcpy(in4->sin_addr, &rawIPbuf, sizeof(in4->sin_addr));
			}
			if (socket_family == AF_INET6)
			{
				DEBUG_INFO("copying buffer as AF_INET");
				in6->sin6_family = socket_family;
			}
		}
		else {
			DEBUG_ERROR("!getRawAddress");
		}
		*/

		//DEBUG_INFO("port     = %d", port);
		//DEBUG_INFO("inetaddr = %p", inetaddr);
		err = zts_bind(fd, (struct sockaddr*)&ss, addrlen);
		return err;
	}

#if defined(__linux__)
	 JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_accept4(JNIEnv *env, jobject thisObj,
		jint fd, jstring addrstr, jint port, jint flags)
	 {
		struct sockaddr_in addr;
		char *str;
		// = env->GetStringUTFChars(addrstr, NULL);
		(*env).ReleaseStringUTFChars(addrstr, str);
		addr.sin_addr.s_addr = inet_addr(str);
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		return zts_accept4(fd, (struct sockaddr *)&addr, sizeof(addr), flags);
	}
#endif

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_accept(JNIEnv *env, jobject thisObj,
		jint fd, jstring addrstr, jint port)
	{
		struct sockaddr_in addr;
		// TODO: Send addr info back to Javaland
		addr.sin_addr.s_addr = inet_addr("");
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		return zts_accept(fd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr));
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_listen(JNIEnv *env, jobject thisObj,
		jint fd, int backlog)
	{
		return zts_listen(fd, backlog);
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_close(JNIEnv *env, jobject thisObj,
		jint fd)
	{
		return zts_close(fd);
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

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_recvfrom(
		JNIEnv *env, jobject thisObj, jint fd, jbyteArray buf, jint len, jint flags, jobject ztaddr)
	{
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
		return rxbytes;
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_send(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len, int flags)
	{
		jbyte *body = (*env).GetByteArrayElements((_jbyteArray *)buf, 0);
		char * bufp = (char *)malloc(sizeof(char)*len);
		memcpy(bufp, body, len);
		(*env).ReleaseByteArrayElements((_jbyteArray *)buf, body, 0);
		int written_bytes = zts_write(fd, body, len);
		return written_bytes;
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

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_read(JNIEnv *env, jobject thisObj,
		jint fd, jarray buf, jint len)
	{
		jbyte *body = (*env).GetByteArrayElements((_jbyteArray *)buf, 0);
		int read_bytes = read(fd, body, len);
		(*env).ReleaseByteArrayElements((_jbyteArray *)buf, body, 0);
		return read_bytes;
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_setsockopt(
		JNIEnv *env, jobject thisObj,
		jint fd, jint level, jint optname, jint optval, jint optlen)
	{
		return zts_setsockopt(fd, level, optname, (const void*)optval, optlen);
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_getsockopt(JNIEnv *env, jobject thisObj,
		jint fd, jint level, jint optname, jint optval, jint optlen)
	{
		return zts_getsockopt(fd, level, optname, (void*)optval, (socklen_t *)optlen);
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

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_fcntl(JNIEnv *env, jobject thisObj,
		jint fd, jint cmd, jint flags)
	{
		return zts_fcntl(fd,cmd,flags);
	}

	/****************************************************************************/
    /* ZeroTier service controls (for JNI wrapper)                              */
    /****************************************************************************/

	JNIEXPORT void JNICALL Java_zerotier_ZeroTier_start(JNIEnv *env, jobject thisObj, jstring path, jboolean blocking)
	{
		if (path) {
			zts_start(env->GetStringUTFChars(path, NULL), blocking);
		}
	}

	JNIEXPORT void JNICALL Java_zerotier_ZeroTier_startjoin(JNIEnv *env, jobject thisObj, jstring path, jstring nwid)
	{
		if (path && nwid) {
			zts_startjoin(env->GetStringUTFChars(path, NULL), env->GetStringUTFChars(nwid, NULL));
		}
	}

	JNIEXPORT void JNICALL Java_zerotier_ZeroTier_stop(JNIEnv *env, jobject thisObj)
	{
		zts_stop();
	}

	JNIEXPORT jboolean JNICALL Java_zerotier_ZeroTier_running(
		JNIEnv *env, jobject thisObj)
	{
		return zts_running();
	}

	JNIEXPORT void JNICALL Java_zerotier_ZeroTier_join(JNIEnv *env, jobject thisObj, jstring nwid)
	{
		if (nwid) {
			zts_join(env->GetStringUTFChars(nwid, NULL));
		}
	}

	JNIEXPORT void JNICALL Java_zerotier_ZeroTier_leave(JNIEnv *env, jobject thisObj, jstring nwid)
	{
		if (nwid) {
			zts_leave(env->GetStringUTFChars(nwid, NULL));
		}
	}

	JNIEXPORT jstring JNICALL Java_zerotier_ZeroTier_homepath(
		JNIEnv *env, jobject thisObj)
	{
		// TODO: fix, should copy into given arg
		// return (*env).NewStringUTF(zts_get_homepath());
		return (*env).NewStringUTF("");
	}

	JNIEXPORT jobject JNICALL Java_zerotier_ZeroTier_get_address(
		JNIEnv *env, jobject thisObj, jstring nwid)
	{
		// get address
		const char *nwid_str = env->GetStringUTFChars(nwid, NULL);
		struct sockaddr_storage ss;
		zts_get_address(nwid_str, &ss, INET_ADDRSTRLEN);
		// map sockaddr_storage contents to java InetAddress
		/*
		jclass c = (*env).FindClass("java/net/InetAddress");
		if (c) {
			jobject addresses = (*env).NewObject(c, (*env).GetMethodID(c, "<init>", "()V"));
			env->CallBooleanMethod(addresses, env->GetMethodID(c, "add", "(Ljava/lang/Object;)Z"), _str);	
			return addresses;
		}*/
		return NULL;
	}

	JNIEXPORT jint Java_zerotier_ZeroTier_get_id()
	{
		return zts_get_id(NULL); // TODO
	}
}

#ifdef __cplusplus
}
#endif
