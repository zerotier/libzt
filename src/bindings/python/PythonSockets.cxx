/*
 * Copyright (c)2013-2021 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2026-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

/**
 * @file
 *
 * ZeroTier Socket API (Python)
 *
 * This code derives from the Python standard library:
 *
 * Lib/socket.py
 * Modules/socketmodule.c
 * Modules/fcntlmodule.c
 * Modules/clinic/fcntlmodule.c.h
 * Modules/clinic/selectmodule.c.h
 *
 * Copyright and license text can be found in pypi packaging directory.
 *
 */

#include "ZeroTierSockets.h"

#ifdef ZTS_ENABLE_PYTHON

#include "Python.h"
#include "PythonSockets.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "structmember.h"   // PyMemberDef

#include <string.h>
#include <sys/time.h>

PyObject* set_error(void)
{
    return NULL;   // PyErr_SetFromErrno(zts_errno);
}

static int zts_py_tuple_to_sockaddr(int family, PyObject* addr_obj, struct zts_sockaddr* dst_addr, int* addrlen)
{
    if (family == AF_INET) {
        struct zts_sockaddr_in* addr;
        char* host_str;
        int result, port;
        if (! PyTuple_Check(addr_obj)) {
            return ZTS_ERR_ARG;
        }
        if (! PyArg_ParseTuple(addr_obj, "eti:zts_py_tuple_to_sockaddr", "idna", &host_str, &port)) {
            return ZTS_ERR_ARG;
        }
        addr = (struct zts_sockaddr_in*)dst_addr;
        result = zts_inet_pton(ZTS_AF_INET, host_str, &(addr->sin_addr.s_addr));
        PyMem_Free(host_str);
        if (port < 0 || port > 0xFFFF) {
            return ZTS_ERR_ARG;
        }
        if (result < 0) {
            return ZTS_ERR_ARG;
        }
        addr->sin_family = AF_INET;
        addr->sin_port = lwip_htons((short)port);
        *addrlen = sizeof *addr;
        return ZTS_ERR_OK;
    }
    if (family == AF_INET6) {
        // TODO
    }
    return ZTS_ERR_ARG;
}

PyObject* zts_py_accept(int fd)
{
    struct zts_sockaddr_in addrbuf = { 0 };
    socklen_t addrlen = sizeof(addrbuf);
    int err = ZTS_ERR_OK;
    Py_BEGIN_ALLOW_THREADS;
    err = zts_bsd_accept(fd, (struct zts_sockaddr*)&addrbuf, &addrlen);
    Py_END_ALLOW_THREADS;
    char ipstr[ZTS_INET_ADDRSTRLEN] = { 0 };
    zts_inet_ntop(ZTS_AF_INET, &(addrbuf.sin_addr), ipstr, ZTS_INET_ADDRSTRLEN);
    PyObject* t;
    t = PyTuple_New(3);
    PyTuple_SetItem(t, 0, PyLong_FromLong(err));   // New file descriptor
    PyTuple_SetItem(t, 1, PyUnicode_FromString(ipstr));
    PyTuple_SetItem(t, 2, PyLong_FromLong(lwip_ntohs(addrbuf.sin_port)));
    Py_INCREF(t);
    return t;
}

int zts_py_bind(int fd, int family, int type, PyObject* addr_obj)
{
    struct zts_sockaddr_storage addrbuf;
    int addrlen;
    int err;
    if (zts_py_tuple_to_sockaddr(family, addr_obj, (struct zts_sockaddr*)&addrbuf, &addrlen) != ZTS_ERR_OK) {
        return ZTS_ERR_ARG;
    }
    Py_BEGIN_ALLOW_THREADS;
    err = zts_bsd_bind(fd, (struct zts_sockaddr*)&addrbuf, addrlen);
    Py_END_ALLOW_THREADS;
    return err;
}

int zts_py_connect(int fd, int family, int type, PyObject* addr_obj)
{
    struct zts_sockaddr_storage addrbuf;
    int addrlen;
    int err;
    if (zts_py_tuple_to_sockaddr(family, addr_obj, (struct zts_sockaddr*)&addrbuf, &addrlen) != ZTS_ERR_OK) {
        return ZTS_ERR_ARG;
    }
    Py_BEGIN_ALLOW_THREADS;
    err = zts_bsd_connect(fd, (struct zts_sockaddr*)&addrbuf, addrlen);
    Py_END_ALLOW_THREADS;
    return err;
}

PyObject* zts_py_recv(int fd, int len, int flags)
{
    PyObject *t, *buf;
    int bytes_read;

    buf = PyBytes_FromStringAndSize((char*)0, len);
    if (buf == NULL) {
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS;
    bytes_read = zts_bsd_recv(fd, PyBytes_AS_STRING(buf), len, flags);
    Py_END_ALLOW_THREADS;
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

int zts_py_send(int fd, PyObject* buf, int flags)
{
    Py_buffer output;
    int bytes_sent;

    if (PyObject_GetBuffer(buf, &output, PyBUF_SIMPLE) != 0) {
        return 0;
    }
    Py_BEGIN_ALLOW_THREADS;
    bytes_sent = zts_bsd_send(fd, output.buf, output.len, flags);
    Py_END_ALLOW_THREADS;
    PyBuffer_Release(&output);

    return bytes_sent;
}

int zts_py_close(int fd)
{
    int err;
    Py_BEGIN_ALLOW_THREADS;
    err = zts_bsd_close(fd);
    Py_END_ALLOW_THREADS;
    return err;
}

PyObject* zts_py_addr_get_str(uint64_t net_id, int family)
{
    char addr_str[ZTS_IP_MAX_STR_LEN] = { 0 };
    if (zts_addr_get_str(net_id, family, addr_str, ZTS_IP_MAX_STR_LEN) < 0) {
        PyErr_SetString(PyExc_Warning, "No address of the given type has been assigned by the network");
        return NULL;
    }
    PyObject* t = PyUnicode_FromString(addr_str);
    return t;
}

/* list of Python objects and their file descriptor */
typedef struct {
    PyObject* obj; /* owned reference */
    int fd;
    int sentinel; /* -1 == sentinel */
} pylist;

void reap_obj(pylist fd2obj[ZTS_FD_SETSIZE + 1])
{
    unsigned int i;
    for (i = 0; i < (unsigned int)ZTS_FD_SETSIZE + 1 && fd2obj[i].sentinel >= 0; i++) {
        Py_CLEAR(fd2obj[i].obj);
    }
    fd2obj[0].sentinel = -1;
}

/* returns NULL and sets the Python exception if an error occurred */
PyObject* set2list(zts_fd_set* set, pylist fd2obj[ZTS_FD_SETSIZE + 1])
{
    int i, j, count = 0;
    PyObject *list, *o;
    int fd;

    for (j = 0; fd2obj[j].sentinel >= 0; j++) {
        if (ZTS_FD_ISSET(fd2obj[j].fd, set)) {
            count++;
        }
    }
    list = PyList_New(count);
    if (! list) {
        return NULL;
    }

    i = 0;
    for (j = 0; fd2obj[j].sentinel >= 0; j++) {
        fd = fd2obj[j].fd;
        if (ZTS_FD_ISSET(fd, set)) {
            o = fd2obj[j].obj;
            fd2obj[j].obj = NULL;
            /* transfer ownership */
            if (PyList_SetItem(list, i, o) < 0) {
                goto finally;
            }
            i++;
        }
    }
    return list;
finally:
    Py_DECREF(list);
    return NULL;
}

/* returns -1 and sets the Python exception if an error occurred, otherwise
   returns a number >= 0
*/
int seq2set(PyObject* seq, zts_fd_set* set, pylist fd2obj[FD_SETSIZE + 1])
{
    int max = -1;
    unsigned int index = 0;
    Py_ssize_t i;
    PyObject* fast_seq = NULL;
    PyObject* o = NULL;

    fd2obj[0].obj = (PyObject*)0; /* set list to zero size */
    ZTS_FD_ZERO(set);

    fast_seq = PySequence_Fast(seq, "arguments 1-3 must be sequences");
    if (! fast_seq) {
        return -1;
    }

    for (i = 0; i < PySequence_Fast_GET_SIZE(fast_seq); i++) {
        int v;

        /* any intervening fileno() calls could decr this refcnt */
        if (! (o = PySequence_Fast_GET_ITEM(fast_seq, i))) {
            goto finally;
        }

        Py_INCREF(o);
        v = PyObject_AsFileDescriptor(o);
        if (v == -1) {
            goto finally;
        }

#if defined(_MSC_VER)
        max = 0; /* not used for Win32 */
#else            /* !_MSC_VER */
        if (! _PyIsSelectable_fd(v)) {
            PyErr_SetString(PyExc_ValueError, "filedescriptor out of range in select()");
            goto finally;
        }
        if (v > max) {
            max = v;
        }
#endif           /* _MSC_VER */
        ZTS_FD_SET(v, set);

        /* add object and its file descriptor to the list */
        if (index >= (unsigned int)FD_SETSIZE) {
            PyErr_SetString(PyExc_ValueError, "too many file descriptors in select()");
            goto finally;
        }
        fd2obj[index].obj = o;
        fd2obj[index].fd = v;
        fd2obj[index].sentinel = 0;
        fd2obj[++index].sentinel = -1;
    }
    Py_DECREF(fast_seq);
    return max + 1;

finally:
    Py_XDECREF(o);
    Py_DECREF(fast_seq);
    return -1;
}

PyObject* zts_py_select(PyObject* module, PyObject* rlist, PyObject* wlist, PyObject* xlist, PyObject* timeout_obj)
{
    pylist rfd2obj[FD_SETSIZE + 1];
    pylist wfd2obj[FD_SETSIZE + 1];
    pylist efd2obj[FD_SETSIZE + 1];
    PyObject* ret = NULL;
    zts_fd_set ifdset, ofdset, efdset;
    struct timeval tv, *tvp;
    int imax, omax, emax, max;
    int n;
    _PyTime_t timeout, deadline = 0;

    if (timeout_obj == Py_None) {
        tvp = (struct timeval*)NULL;
    }
    else {
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION <= 5
        _PyTime_round_t roundingMode = _PyTime_ROUND_CEILING;
#else
        _PyTime_round_t roundingMode = _PyTime_ROUND_UP;
#endif
        if (_PyTime_FromSecondsObject(&timeout, timeout_obj, roundingMode) < 0) {
            if (PyErr_ExceptionMatches(PyExc_TypeError)) {
                PyErr_SetString(PyExc_TypeError, "timeout must be a float or None");
            }
            return NULL;
        }
        if (_PyTime_AsTimeval(timeout, &tv, roundingMode) == -1) {
            return NULL;
        }
        if (tv.tv_sec < 0) {
            PyErr_SetString(PyExc_ValueError, "timeout must be non-negative");
            return NULL;
        }
        tvp = &tv;
    }
    /* Convert iterables to zts_fd_sets, and get maximum fd number
     * propagates the Python exception set in seq2set()
     */
    rfd2obj[0].sentinel = -1;
    wfd2obj[0].sentinel = -1;
    efd2obj[0].sentinel = -1;
    if ((imax = seq2set(rlist, &ifdset, rfd2obj)) < 0) {
        goto finally;
    }
    if ((omax = seq2set(wlist, &ofdset, wfd2obj)) < 0) {
        goto finally;
    }
    if ((emax = seq2set(xlist, &efdset, efd2obj)) < 0) {
        goto finally;
    }

    max = imax;
    if (omax > max) {
        max = omax;
    }
    if (emax > max) {
        max = emax;
    }
    if (tvp) {
        deadline = _PyTime_GetMonotonicClock() + timeout;
    }

    do {
        Py_BEGIN_ALLOW_THREADS;
        errno = 0;
        // struct zts_timeval zts_tvp;
        // zts_tvp.tv_sec = tvp.tv_sec;
        // zts_tvp.tv_sec = tvp.tv_sec;

        n = zts_bsd_select(max, &ifdset, &ofdset, &efdset, (struct zts_timeval*)tvp);
        Py_END_ALLOW_THREADS;

        if (errno != EINTR) {
            break;
        }

        /* select() was interrupted by a signal */
        if (PyErr_CheckSignals()) {
            goto finally;
        }

        if (tvp) {
            timeout = deadline - _PyTime_GetMonotonicClock();
            if (timeout < 0) {
                /* bpo-35310: lists were unmodified -- clear them explicitly */
                ZTS_FD_ZERO(&ifdset);
                ZTS_FD_ZERO(&ofdset);
                ZTS_FD_ZERO(&efdset);
                n = 0;
                break;
            }
            _PyTime_AsTimeval_noraise(timeout, &tv, _PyTime_ROUND_CEILING);
            /* retry select() with the recomputed timeout */
        }
    } while (1);

#ifdef MS_WINDOWS
    if (n == SOCKET_ERROR) {
        PyErr_SetExcFromWindowsErr(PyExc_OSError, WSAGetLastError());
    }
#else
    if (n < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
    }
#endif
    else {
        /* any of these three calls can raise an exception.  it's more
           convenient to test for this after all three calls... but
           is that acceptable?
        */
        rlist = set2list(&ifdset, rfd2obj);
        wlist = set2list(&ofdset, wfd2obj);
        xlist = set2list(&efdset, efd2obj);
        if (PyErr_Occurred()) {
            ret = NULL;
        }
        else {
            ret = PyTuple_Pack(3, rlist, wlist, xlist);
        }
        Py_XDECREF(rlist);
        Py_XDECREF(wlist);
        Py_XDECREF(xlist);
    }

finally:
    reap_obj(rfd2obj);
    reap_obj(wfd2obj);
    reap_obj(efd2obj);
    return ret;
}

int zts_py_setsockopt(int fd, PyObject* args)
{
    int level;
    int optname;
    int res;
    Py_buffer optval;
    int flag;
    unsigned int optlen;
    PyObject* none;

    // setsockopt(level, opt, flag)
    if (PyArg_ParseTuple(args, "iii:setsockopt", &level, &optname, &flag)) {
        res = zts_bsd_setsockopt(fd, level, optname, (char*)&flag, sizeof flag);
        goto done;
    }

    PyErr_Clear();
    // setsockopt(level, opt, None, flag)
    if (PyArg_ParseTuple(args, "iiO!I:setsockopt", &level, &optname, Py_TYPE(Py_None), &none, &optlen)) {
        assert(sizeof(socklen_t) >= sizeof(unsigned int));
        res = zts_bsd_setsockopt(fd, level, optname, NULL, (socklen_t)optlen);
        goto done;
    }

    PyErr_Clear();
    // setsockopt(level, opt, buffer)
    if (! PyArg_ParseTuple(args, "iiy*:setsockopt", &level, &optname, &optval)) {
        return (int)NULL;
    }

#ifdef MS_WINDOWS
    if (optval.len > INT_MAX) {
        PyBuffer_Release(&optval);
        PyErr_Format(PyExc_OverflowError, "socket option is larger than %i bytes", INT_MAX);
        return (int)NULL;
    }
    res = zts_bsd_setsockopt(fd, level, optname, optval.buf, (int)optval.len);
#else
    res = zts_bsd_setsockopt(fd, level, optname, optval.buf, optval.len);
#endif
    PyBuffer_Release(&optval);

done:
    return res;
}

PyObject* zts_py_getsockopt(int fd, PyObject* args)
{
    int level;
    int optname;
    int res;
    PyObject* buf;
    socklen_t buflen = 0;
    int flag = 0;
    socklen_t flagsize;

    if (! PyArg_ParseTuple(args, "ii|i:getsockopt", &level, &optname, &buflen)) {
        return NULL;
    }
    if (buflen == 0) {
        flagsize = sizeof flag;
        res = zts_bsd_getsockopt(fd, level, optname, (void*)&flag, &flagsize);
        if (res < 0) {
            return set_error();
        }
        return PyLong_FromLong(flag);
    }
    if (buflen <= 0 || buflen > 1024) {
        PyErr_SetString(PyExc_OSError, "getsockopt buflen out of range");
        return NULL;
    }
    buf = PyBytes_FromStringAndSize((char*)NULL, buflen);
    if (buf == NULL) {
        return NULL;
    }
    res = zts_bsd_getsockopt(fd, level, optname, (void*)PyBytes_AS_STRING(buf), &buflen);
    if (res < 0) {
        Py_DECREF(buf);
        return set_error();
    }
    _PyBytes_Resize(&buf, buflen);
    return buf;
}

PyObject* zts_py_fcntl(int fd, int code, PyObject* arg)
{
    unsigned int int_arg = 0;
    int ret;

    if (arg != NULL) {
        int parse_result;
        PyErr_Clear();
        parse_result = PyArg_Parse(
            arg,
            "I;fcntl requires a file or file descriptor,"
            " an integer and optionally a third integer",
            &int_arg);
        if (! parse_result) {
            return NULL;
        }
    }
    do {
        Py_BEGIN_ALLOW_THREADS;
        ret = zts_bsd_fcntl(fd, code, (int)int_arg);
        Py_END_ALLOW_THREADS;
    } while (ret == -1 && zts_errno == ZTS_EINTR);
    if (ret < 0) {
        return set_error();
    }
    return PyLong_FromLong((long)ret);
}

PyObject* zts_py_ioctl(int fd, unsigned int code, PyObject* ob_arg, int mutate_arg)
{
#define IOCTL_BUFSZ 1024
    int arg = 0;
    int ret;
    Py_buffer pstr;
    char* str;
    Py_ssize_t len;
    char buf[IOCTL_BUFSZ + 1]; /* argument plus NUL byte */

    if (ob_arg != NULL) {
        if (PyArg_Parse(ob_arg, "w*:ioctl", &pstr)) {
            char* arg;
            str = (char*)pstr.buf;
            len = pstr.len;

            if (mutate_arg) {
                if (len <= IOCTL_BUFSZ) {
                    memcpy(buf, str, len);
                    buf[len] = '\0';
                    arg = buf;
                }
                else {
                    arg = str;
                }
            }
            else {
                if (len > IOCTL_BUFSZ) {
                    PyBuffer_Release(&pstr);
                    PyErr_SetString(PyExc_ValueError, "ioctl string arg too long");
                    return NULL;
                }
                else {
                    memcpy(buf, str, len);
                    buf[len] = '\0';
                    arg = buf;
                }
            }
            if (buf == arg) {
                Py_BEGIN_ALLOW_THREADS;
                /* think array.resize() */
                ret = zts_bsd_ioctl(fd, code, arg);
                Py_END_ALLOW_THREADS;
            }
            else {
                ret = zts_bsd_ioctl(fd, code, arg);
            }
            if (mutate_arg && (len <= IOCTL_BUFSZ)) {
                memcpy(str, buf, len);
            }
            PyBuffer_Release(&pstr); /* No further access to str below this point */
            if (ret < 0) {
                PyErr_SetFromErrno(PyExc_OSError);
                return NULL;
            }
            if (mutate_arg) {
                return PyLong_FromLong(ret);
            }
            else {
                return PyBytes_FromStringAndSize(buf, len);
            }
        }

        PyErr_Clear();
        if (PyArg_Parse(ob_arg, "s*:ioctl", &pstr)) {
            str = (char*)pstr.buf;
            len = pstr.len;
            if (len > IOCTL_BUFSZ) {
                PyBuffer_Release(&pstr);
                PyErr_SetString(PyExc_ValueError, "ioctl string arg too long");
                return NULL;
            }
            memcpy(buf, str, len);
            buf[len] = '\0';
            Py_BEGIN_ALLOW_THREADS;
            ret = zts_bsd_ioctl(fd, code, buf);
            Py_END_ALLOW_THREADS;
            if (ret < 0) {
                PyBuffer_Release(&pstr);
                PyErr_SetFromErrno(PyExc_OSError);
                return NULL;
            }
            PyBuffer_Release(&pstr);
            return PyBytes_FromStringAndSize(buf, len);
        }

        PyErr_Clear();
        if (! PyArg_Parse(
                ob_arg,
                "i;ioctl requires a file or file descriptor,"
                " an integer and optionally an integer or buffer argument",
                &arg)) {
            return NULL;
        }
        // Fall-through to outside the 'if' statement.
    }
    // TODO: Double check that &arg is correct
    Py_BEGIN_ALLOW_THREADS;
    ret = zts_bsd_ioctl(fd, code, &arg);
    Py_END_ALLOW_THREADS;
    if (ret < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    return PyLong_FromLong((long)ret);
#undef IOCTL_BUFSZ
}

#endif   // ZTS_ENABLE_PYTHON
