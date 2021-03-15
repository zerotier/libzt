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
 * ZeroTier Socket API (Python)
 */

#include "lwip/sockets.h"
#include "lwip/inet.h"

#include "ZeroTierSockets.h"

#ifdef ZTS_ENABLE_PYTHON

int zts_py_setblocking(int fd, int flag)
{
	int flags = ZTS_ERR_OK;
	if ((flags = zts_fcntl(fd, F_GETFL, 0)) < 0) {
		return ZTS_ERR_SOCKET;
	}
	return zts_fcntl(fd, F_SETFL, flags | ZTS_O_NONBLOCK);
}

int zts_py_getblocking(int fd)
{
	int flags = ZTS_ERR_OK;
	if ((flags = zts_fcntl(fd, F_GETFL, 0)) < 0) {
		return ZTS_ERR_SOCKET;
	}
	return flags & ZTS_O_NONBLOCK;
}

static int zts_py_tuple_to_sockaddr(int family,
	PyObject *addr_obj, struct zts_sockaddr *dst_addr, int *addrlen)
{
	if (family == AF_INET) {
		struct zts_sockaddr_in* addr;
		char *host_str;
		int result, port;
		if (!PyTuple_Check(addr_obj)) {
			return ZTS_ERR_ARG;
		}
		if (!PyArg_ParseTuple(addr_obj,
			"eti:zts_py_tuple_to_sockaddr", "idna", &host_str, &port)) {
			return ZTS_ERR_ARG;
		}
		addr = (struct zts_sockaddr_in*)dst_addr;
		zts_inet_pton(ZTS_AF_INET, host_str, &(addr->sin_addr.s_addr));
		PyMem_Free(host_str);
		if (port < 0 || port > 0xFFFF) {
			return ZTS_ERR_ARG;
		}
		if (result < 0) {
			return ZTS_ERR_ARG;
		}
		addr->sin_family = AF_INET;
		addr->sin_port   = lwip_htons((short)port);
		*addrlen         = sizeof *addr;
		return ZTS_ERR_OK;
	}
	if (family == AF_INET6) {
		// TODO
	}
	return ZTS_ERR_ARG;
}

PyObject * zts_py_accept(int fd)
{
	struct zts_sockaddr_in addrbuf;
	socklen_t addrlen = sizeof(addrbuf);
	memset(&addrbuf, 0, addrlen);
	int err = zts_accept(fd, (struct zts_sockaddr*)&addrbuf, &addrlen);
	char ipstr[ZTS_INET_ADDRSTRLEN];
	memset(ipstr, 0, sizeof(ipstr));
	zts_inet_ntop(ZTS_AF_INET, &(addrbuf.sin_addr), ipstr, ZTS_INET_ADDRSTRLEN);
	PyObject *t;
	t = PyTuple_New(3);
	PyTuple_SetItem(t, 0, PyLong_FromLong(err)); // New file descriptor
	PyTuple_SetItem(t, 1, PyUnicode_FromString(ipstr));
	PyTuple_SetItem(t, 2, PyLong_FromLong(lwip_ntohs(addrbuf.sin_port)));
	Py_INCREF(t);
	return t;
}

int zts_py_listen(int fd, int backlog)
{
	return zts_listen(fd, backlog);
}

int zts_py_bind(int fd, int family, int type, PyObject *addr_obj)
{
	struct zts_sockaddr_storage addrbuf;
	int addrlen;
	int err;
	if (zts_py_tuple_to_sockaddr(family, addr_obj,
		(struct zts_sockaddr *)&addrbuf, &addrlen) != ZTS_ERR_OK)
	{
		return ZTS_ERR_ARG;
	}
	Py_BEGIN_ALLOW_THREADS
	err = zts_bind(fd, (struct zts_sockaddr *)&addrbuf, addrlen);
	Py_END_ALLOW_THREADS
	Py_INCREF(Py_None);
	return err;
}

int zts_py_connect(int fd, int family, int type, PyObject *addr_obj)
{
	struct zts_sockaddr_storage addrbuf;
	int addrlen;
	int err;
	if (zts_py_tuple_to_sockaddr(family, addr_obj,
		(struct zts_sockaddr *)&addrbuf, &addrlen) != ZTS_ERR_OK)
	{
		return ZTS_ERR_ARG;
	}
	Py_BEGIN_ALLOW_THREADS
	err = zts_connect(fd, (struct zts_sockaddr *)&addrbuf, addrlen);
	Py_END_ALLOW_THREADS
	Py_INCREF(Py_None);
	return err;
}

PyObject * zts_py_recv(int fd, int len, int flags)
{
	PyObject *t;
	char buf[4096];
	int err = zts_recv(fd, buf, len, flags);
	if (err < 0) {
		return NULL;
	}
	t = PyTuple_New(2);
	PyTuple_SetItem(t, 0, PyLong_FromLong(err));
	PyTuple_SetItem(t, 1, PyUnicode_FromString(buf));
	Py_INCREF(t);
	return t;
}

int zts_py_send(int fd, PyObject *buf, int len, int flags)
{
	int err = ZTS_ERR_OK;
	PyObject *encodedStr = PyUnicode_AsEncodedString(buf, "UTF-8", "strict");
	if (encodedStr) {
	    char *bytes = PyBytes_AsString(encodedStr);
	    err = zts_send(fd, bytes, len, flags);
	    Py_DECREF(encodedStr);
	}
	return err;
}

int zts_py_close(int fd)
{
	return zts_close(fd);
}

#endif // ZTS_ENABLE_PYTHON
