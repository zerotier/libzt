%module zt

%include <windows.i>
%include <stl.i>
%include <typemaps.i>

// Prevent SWIG from doing anything funny with our types
%apply unsigned char      {  uint8_t };
%apply char               {   int8_t };
%apply unsigned short     { uint16_t };
%apply int                {  int16_t };
%apply unsigned short     { uint32_t };
%apply int                {  int32_t };
%apply unsigned long long { uint64_t };
%apply long long          {  int64_t };

%typemap(ctype)   zts_sockaddr* "zts_sockaddr*"

// Ignore all classes/structs (We'll define these manually in C#)
// %rename($ignore, %$isclass) "";

%ignore zts_in6_addr;

%ignore zts_sockaddr;
%ignore zts_in_addr;
%ignore zts_sockaddr_in;
%ignore zts_sockaddr_storage;
%ignore zts_sockaddr_in6;

%ignore zts_event_msg_t;
%ignore zts_node_info_t;
%ignore zts_net_info_t;
%ignore zts_netif_info_t;
%ignore zts_route_info_t;
%ignore zts_peer_info_t;
%ignore zts_addr_info_t;

#define ZTS_ENABLE_PINVOKE 1

%{
#include "../../../include/ZeroTierSockets.h"
%}

// Typemap for our event callback
%define %cs_callback(TYPE, CSTYPE)
    %typemap(ctype) TYPE, TYPE& "void *"
    %typemap(in) TYPE  %{ $1 = ($1_type)$input; %}
    %typemap(in) TYPE& %{ $1 = ($1_type)&$input; %}
    %typemap(imtype, out="IntPtr") TYPE, TYPE& "CSTYPE"
    %typemap(cstype, out="IntPtr") TYPE, TYPE& "CSTYPE"
    %typemap(csin) TYPE, TYPE& "$csinput"
%enddef
%cs_callback(CppCallback, CSharpCallbackWithStruct)

%include "../../../include/ZeroTierSockets.h"
