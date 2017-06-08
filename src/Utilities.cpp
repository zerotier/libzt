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

#include <stdio.h>
#include <execinfo.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

/*
void zt_dump_stacktrace(int sig) {
  void *array[16];
  size_t size;
  size = backtrace(array, 16);
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}
*/

/*
char *beautify_pico_error(int err)
{
	switch(err){
		PICO_ERR_NOERR = 0,
    PICO_ERR_EPERM = 1,
    PICO_ERR_ENOENT = 2,
    
    PICO_ERR_EINTR = 4,
    PICO_ERR_EIO = 5,
    PICO_ERR_ENXIO = 6,
    
    PICO_ERR_EAGAIN = 11,
    PICO_ERR_ENOMEM = 12,
    PICO_ERR_EACCESS = 13,
    PICO_ERR_EFAULT = 14,
   
    PICO_ERR_EBUSY = 16,
    PICO_ERR_EEXIST = 17,
   
    PICO_ERR_EINVAL = 22,

    PICO_ERR_ENONET = 64,

    PICO_ERR_EPROTO = 71,

    PICO_ERR_ENOPROTOOPT = 92,
    PICO_ERR_EPROTONOSUPPORT = 93,

    PICO_ERR_EOPNOTSUPP = 95,
    PICO_ERR_EADDRINUSE = 98,
    PICO_ERR_EADDRNOTAVAIL = 99,
    PICO_ERR_ENETDOWN = 100,
    PICO_ERR_ENETUNREACH = 101,

    PICO_ERR_ECONNRESET = 104,

    PICO_ERR_EISCONN = 106,
    PICO_ERR_ENOTCONN = 107,
    PICO_ERR_ESHUTDOWN = 108,

    PICO_ERR_ETIMEDOUT = 110,
    PICO_ERR_ECONNREFUSED = 111,
    PICO_ERR_EHOSTDOWN = 112,
    PICO_ERR_EHOSTUNREACH = 113,
	}
	return err_text;
}
*/