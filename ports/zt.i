/* libzt.i */

%begin
%{
#define SWIG_PYTHON_CAST_MODE
%}

%include <stdint.i>

#define PYTHON_BUILD 1

%module libzt
%{
#include "../include/ZeroTier.h"
#include "../include/ZeroTierConstants.h"
%}

%define %cs_callback(TYPE, CSTYPE)
    %typemap(ctype) TYPE, TYPE& "void *"
    %typemap(in) TYPE  %{ $1 = ($1_type)$input; %}
    %typemap(in) TYPE& %{ $1 = ($1_type)&$input; %}
    %typemap(imtype, out="IntPtr") TYPE, TYPE& "CSTYPE"
    %typemap(cstype, out="IntPtr") TYPE, TYPE& "CSTYPE"
    %typemap(csin) TYPE, TYPE& "$csinput"
%enddef

%cs_callback(userCallbackFunc, CSharpCallback)

%include "../include/ZeroTier.h"
%include "../include/ZeroTierConstants.h"
