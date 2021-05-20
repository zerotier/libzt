/* libzt.i */

%begin
%{
#define SWIG_PYTHON_CAST_MODE
%}

%include <stdint.i>

#define ZTS_ENABLE_PYTHON 1

%module(directors="1") libzt
%module libzt
%{
#include "ZeroTierSockets.h"
#include "PythonSockets.h"
%}

%feature("director") PythonDirectorCallbackClass;

%ignore zts_in6_addr;
%ignore zts_sockaddr;
%ignore zts_in_addr;
%ignore zts_sockaddr_in;
%ignore zts_sockaddr_storage;
%ignore zts_sockaddr_in6;

%ignore zts_linger;
%ignore zts_ip_mreq;
%ignore zts_in_pktinfo;
%ignore zts_ipv6_mreq;

%ignore zts_fd_set;
%ignore zts_pollfd;
%ignore zts_nfds_t;
%ignore zts_msghdr;

%include "ZeroTierSockets.h"
%include "PythonSockets.h"
