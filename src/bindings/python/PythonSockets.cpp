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

#include <string.h>

#include "lwip/sockets.h"
#include "lwip/inet.h"

#include "ZeroTierSockets.h"

#ifdef ZTS_ENABLE_PYTHON

int zts_py_setblocking(int fd, int block)
{
	int new_flags, flags, err = 0;

	Py_BEGIN_ALLOW_THREADS
  flags = zts_fcntl(fd, F_GETFL, 0);

	if (flags < 0) {
		err = ZTS_ERR_SOCKET;
    goto done;
	}

  if (block) {
    new_flags |= ZTS_O_NONBLOCK;
  } else {
    new_flags &= ~ZTS_O_NONBLOCK;
  }

  if (new_flags != flags) {
	  err = zts_fcntl(fd, F_SETFL, flags);
  }

done:
	Py_END_ALLOW_THREADS

  return err;
}

int zts_py_getblocking(int fd)
{
	int flags;

	Py_BEGIN_ALLOW_THREADS
	flags = zts_fcntl(fd, F_GETFL, 0);
	Py_END_ALLOW_THREADS

	if (flags < 0) {
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
  if (backlog < 0) {
    backlog = 128;
  }
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
	return err;
}

PyObject * zts_py_recv(int fd, int len, int flags)
{
	PyObject *t, *buf;
	int bytes_read;

	buf = PyBytes_FromStringAndSize((char *) 0, len);
	if (buf == NULL) {
		return NULL;
	}

	bytes_read = zts_recv(fd, PyBytes_AS_STRING(buf), len, flags);
	t = PyTuple_New(2);
	PyTuple_SetItem(t, 0, PyLong_FromLong(bytes_read));

	if (bytes_read < 0) {
		Py_DECREF(buf);
		Py_INCREF(Py_None);
		PyTuple_SetItem(t, 1, Py_None);
		Py_INCREF(t);
		return t;
	}

	if (bytes_read != len) {
		 _PyBytes_Resize(&buf, bytes_read);
	}

	PyTuple_SetItem(t, 1, buf);
	Py_INCREF(t);
	return t;
}

int zts_py_send(int fd, PyObject *buf, int flags)
{
	int bytes_sent = ZTS_ERR_OK;
	char *bytes = NULL;
	PyObject *encodedStr = NULL;

	// Check for various encodings, or lack thereof

	if (PyByteArray_Check(buf)) {
		bytes = PyByteArray_AsString(buf);
	}

	if (PyUnicode_Check(buf)) {
		encodedStr = PyUnicode_AsEncodedString(buf, "UTF-8", "strict");
		if (!encodedStr) {
			return ZTS_ERR_ARG;
		}
		bytes = PyBytes_AsString(encodedStr);
	}

	if (!bytes) {
		// No encoding detected
		bytes = PyBytes_AsString(buf);
	}

	// If we still don't have a valid pointer to a C-string, fail
	if (!bytes) {
		bytes_sent = ZTS_ERR_ARG;
	}
	else {
		bytes_sent = zts_send(fd, bytes, strlen(bytes), flags);
	}

	if (encodedStr) {
		Py_DECREF(encodedStr);
	}
	return bytes_sent;
}

int zts_py_close(int fd)
{
  int err;
	Py_BEGIN_ALLOW_THREADS
	err = zts_close(fd);
	Py_END_ALLOW_THREADS
	return err;
}

#endif // ZTS_ENABLE_PYTHON
