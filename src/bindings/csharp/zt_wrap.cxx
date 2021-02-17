/*
 * Copyright (c)2013-2020 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2024-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

#ifndef SWIGCSHARP
#define SWIGCSHARP
#endif



#ifdef __cplusplus
/* SwigValueWrapper is described in swig.swg */
template<typename T> class SwigValueWrapper {
  struct SwigMovePointer {
    T *ptr;
    SwigMovePointer(T *p) : ptr(p) { }
    ~SwigMovePointer() { delete ptr; }
    SwigMovePointer& operator=(SwigMovePointer& rhs) { T* oldptr = ptr; ptr = 0; delete oldptr; ptr = rhs.ptr; rhs.ptr = 0; return *this; }
  } pointer;
  SwigValueWrapper& operator=(const SwigValueWrapper<T>& rhs);
  SwigValueWrapper(const SwigValueWrapper<T>& rhs);
public:
  SwigValueWrapper() : pointer(0) { }
  SwigValueWrapper& operator=(const T& t) { SwigMovePointer tmp(new T(t)); pointer = tmp; return *this; }
  operator T&() const { return *pointer.ptr; }
  T *operator&() { return pointer.ptr; }
};

template <typename T> T SwigValueInit() {
  return T();
}
#endif

/* -----------------------------------------------------------------------------
 *  This section contains generic SWIG labels for method/variable
 *  declarations/attributes, and other compiler dependent labels.
 * ----------------------------------------------------------------------------- */

/* template workaround for compilers that cannot correctly implement the C++ standard */
#ifndef SWIGTEMPLATEDISAMBIGUATOR
# if defined(__SUNPRO_CC) && (__SUNPRO_CC <= 0x560)
#  define SWIGTEMPLATEDISAMBIGUATOR template
# elif defined(__HP_aCC)
/* Needed even with `aCC -AA' when `aCC -V' reports HP ANSI C++ B3910B A.03.55 */
/* If we find a maximum version that requires this, the test would be __HP_aCC <= 35500 for A.03.55 */
#  define SWIGTEMPLATEDISAMBIGUATOR template
# else
#  define SWIGTEMPLATEDISAMBIGUATOR
# endif
#endif

/* inline attribute */
#ifndef SWIGINLINE
# if defined(__cplusplus) || (defined(__GNUC__) && !defined(__STRICT_ANSI__))
#   define SWIGINLINE inline
# else
#   define SWIGINLINE
# endif
#endif

/* attribute recognised by some compilers to avoid 'unused' warnings */
#ifndef SWIGUNUSED
# if defined(__GNUC__)
#   if !(defined(__cplusplus)) || (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))
#     define SWIGUNUSED __attribute__ ((__unused__))
#   else
#     define SWIGUNUSED
#   endif
# elif defined(__ICC)
#   define SWIGUNUSED __attribute__ ((__unused__))
# else
#   define SWIGUNUSED
# endif
#endif

#ifndef SWIG_MSC_UNSUPPRESS_4505
# if defined(_MSC_VER)
#   pragma warning(disable : 4505) /* unreferenced local function has been removed */
# endif
#endif

#ifndef SWIGUNUSEDPARM
# ifdef __cplusplus
#   define SWIGUNUSEDPARM(p)
# else
#   define SWIGUNUSEDPARM(p) p SWIGUNUSED
# endif
#endif

/* internal SWIG method */
#ifndef SWIGINTERN
# define SWIGINTERN static SWIGUNUSED
#endif

/* internal inline SWIG method */
#ifndef SWIGINTERNINLINE
# define SWIGINTERNINLINE SWIGINTERN SWIGINLINE
#endif

/* exporting methods */
#if defined(__GNUC__)
#  if (__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#    ifndef GCC_HASCLASSVISIBILITY
#      define GCC_HASCLASSVISIBILITY
#    endif
#  endif
#endif

#ifndef SWIGEXPORT
# if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#   if defined(STATIC_LINKED)
#     define SWIGEXPORT
#   else
#     define SWIGEXPORT __declspec(dllexport)
#   endif
# else
#   if defined(__GNUC__) && defined(GCC_HASCLASSVISIBILITY)
#     define SWIGEXPORT __attribute__ ((visibility("default")))
#   else
#     define SWIGEXPORT
#   endif
# endif
#endif

/* calling conventions for Windows */
#ifndef SWIGSTDCALL
# if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#   define SWIGSTDCALL __stdcall
# else
#   define SWIGSTDCALL
# endif
#endif

/* Deal with Microsoft's attempt at deprecating C standard runtime functions */
#if !defined(SWIG_NO_CRT_SECURE_NO_DEPRECATE) && defined(_MSC_VER) && !defined(_CRT_SECURE_NO_DEPRECATE)
# define _CRT_SECURE_NO_DEPRECATE
#endif

/* Deal with Microsoft's attempt at deprecating methods in the standard C++ library */
#if !defined(SWIG_NO_SCL_SECURE_NO_DEPRECATE) && defined(_MSC_VER) && !defined(_SCL_SECURE_NO_DEPRECATE)
# define _SCL_SECURE_NO_DEPRECATE
#endif

/* Deal with Apple's deprecated 'AssertMacros.h' from Carbon-framework */
#if defined(__APPLE__) && !defined(__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES)
# define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0
#endif

/* Intel's compiler complains if a variable which was never initialised is
 * cast to void, which is a common idiom which we use to indicate that we
 * are aware a variable isn't used.  So we just silence that warning.
 * See: https://github.com/swig/swig/issues/192 for more discussion.
 */
#ifdef __INTEL_COMPILER
# pragma warning disable 592
#endif


#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/* Support for throwing C# exceptions from C/C++. There are two types: 
 * Exceptions that take a message and ArgumentExceptions that take a message and a parameter name. */
typedef enum {
  SWIG_CSharpApplicationException,
  SWIG_CSharpArithmeticException,
  SWIG_CSharpDivideByZeroException,
  SWIG_CSharpIndexOutOfRangeException,
  SWIG_CSharpInvalidCastException,
  SWIG_CSharpInvalidOperationException,
  SWIG_CSharpIOException,
  SWIG_CSharpNullReferenceException,
  SWIG_CSharpOutOfMemoryException,
  SWIG_CSharpOverflowException,
  SWIG_CSharpSystemException
} SWIG_CSharpExceptionCodes;

typedef enum {
  SWIG_CSharpArgumentException,
  SWIG_CSharpArgumentNullException,
  SWIG_CSharpArgumentOutOfRangeException
} SWIG_CSharpExceptionArgumentCodes;

typedef void (SWIGSTDCALL* SWIG_CSharpExceptionCallback_t)(const char *);
typedef void (SWIGSTDCALL* SWIG_CSharpExceptionArgumentCallback_t)(const char *, const char *);

typedef struct {
  SWIG_CSharpExceptionCodes code;
  SWIG_CSharpExceptionCallback_t callback;
} SWIG_CSharpException_t;

typedef struct {
  SWIG_CSharpExceptionArgumentCodes code;
  SWIG_CSharpExceptionArgumentCallback_t callback;
} SWIG_CSharpExceptionArgument_t;

static SWIG_CSharpException_t SWIG_csharp_exceptions[] = {
  { SWIG_CSharpApplicationException, NULL },
  { SWIG_CSharpArithmeticException, NULL },
  { SWIG_CSharpDivideByZeroException, NULL },
  { SWIG_CSharpIndexOutOfRangeException, NULL },
  { SWIG_CSharpInvalidCastException, NULL },
  { SWIG_CSharpInvalidOperationException, NULL },
  { SWIG_CSharpIOException, NULL },
  { SWIG_CSharpNullReferenceException, NULL },
  { SWIG_CSharpOutOfMemoryException, NULL },
  { SWIG_CSharpOverflowException, NULL },
  { SWIG_CSharpSystemException, NULL }
};

static SWIG_CSharpExceptionArgument_t SWIG_csharp_exceptions_argument[] = {
  { SWIG_CSharpArgumentException, NULL },
  { SWIG_CSharpArgumentNullException, NULL },
  { SWIG_CSharpArgumentOutOfRangeException, NULL }
};

static void SWIGUNUSED SWIG_CSharpSetPendingException(SWIG_CSharpExceptionCodes code, const char *msg) {
  SWIG_CSharpExceptionCallback_t callback = SWIG_csharp_exceptions[SWIG_CSharpApplicationException].callback;
  if ((size_t)code < sizeof(SWIG_csharp_exceptions)/sizeof(SWIG_CSharpException_t)) {
    callback = SWIG_csharp_exceptions[code].callback;
  }
  callback(msg);
}

static void SWIGUNUSED SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpExceptionArgumentCodes code, const char *msg, const char *param_name) {
  SWIG_CSharpExceptionArgumentCallback_t callback = SWIG_csharp_exceptions_argument[SWIG_CSharpArgumentException].callback;
  if ((size_t)code < sizeof(SWIG_csharp_exceptions_argument)/sizeof(SWIG_CSharpExceptionArgument_t)) {
    callback = SWIG_csharp_exceptions_argument[code].callback;
  }
  callback(msg, param_name);
}


#ifdef __cplusplus
extern "C" 
#endif
SWIGEXPORT void SWIGSTDCALL SWIGRegisterExceptionCallbacks_zt(
                                                SWIG_CSharpExceptionCallback_t applicationCallback,
                                                SWIG_CSharpExceptionCallback_t arithmeticCallback,
                                                SWIG_CSharpExceptionCallback_t divideByZeroCallback, 
                                                SWIG_CSharpExceptionCallback_t indexOutOfRangeCallback, 
                                                SWIG_CSharpExceptionCallback_t invalidCastCallback,
                                                SWIG_CSharpExceptionCallback_t invalidOperationCallback,
                                                SWIG_CSharpExceptionCallback_t ioCallback,
                                                SWIG_CSharpExceptionCallback_t nullReferenceCallback,
                                                SWIG_CSharpExceptionCallback_t outOfMemoryCallback, 
                                                SWIG_CSharpExceptionCallback_t overflowCallback, 
                                                SWIG_CSharpExceptionCallback_t systemCallback) {
  SWIG_csharp_exceptions[SWIG_CSharpApplicationException].callback = applicationCallback;
  SWIG_csharp_exceptions[SWIG_CSharpArithmeticException].callback = arithmeticCallback;
  SWIG_csharp_exceptions[SWIG_CSharpDivideByZeroException].callback = divideByZeroCallback;
  SWIG_csharp_exceptions[SWIG_CSharpIndexOutOfRangeException].callback = indexOutOfRangeCallback;
  SWIG_csharp_exceptions[SWIG_CSharpInvalidCastException].callback = invalidCastCallback;
  SWIG_csharp_exceptions[SWIG_CSharpInvalidOperationException].callback = invalidOperationCallback;
  SWIG_csharp_exceptions[SWIG_CSharpIOException].callback = ioCallback;
  SWIG_csharp_exceptions[SWIG_CSharpNullReferenceException].callback = nullReferenceCallback;
  SWIG_csharp_exceptions[SWIG_CSharpOutOfMemoryException].callback = outOfMemoryCallback;
  SWIG_csharp_exceptions[SWIG_CSharpOverflowException].callback = overflowCallback;
  SWIG_csharp_exceptions[SWIG_CSharpSystemException].callback = systemCallback;
}

#ifdef __cplusplus
extern "C" 
#endif
SWIGEXPORT void SWIGSTDCALL SWIGRegisterExceptionArgumentCallbacks_zt(
                                                SWIG_CSharpExceptionArgumentCallback_t argumentCallback,
                                                SWIG_CSharpExceptionArgumentCallback_t argumentNullCallback,
                                                SWIG_CSharpExceptionArgumentCallback_t argumentOutOfRangeCallback) {
  SWIG_csharp_exceptions_argument[SWIG_CSharpArgumentException].callback = argumentCallback;
  SWIG_csharp_exceptions_argument[SWIG_CSharpArgumentNullException].callback = argumentNullCallback;
  SWIG_csharp_exceptions_argument[SWIG_CSharpArgumentOutOfRangeException].callback = argumentOutOfRangeCallback;
}


/* Callback for returning strings to C# without leaking memory */
typedef char * (SWIGSTDCALL* SWIG_CSharpStringHelperCallback)(const char *);
static SWIG_CSharpStringHelperCallback SWIG_csharp_string_callback = NULL;


#ifdef __cplusplus
extern "C" 
#endif
SWIGEXPORT void SWIGSTDCALL SWIGRegisterStringCallback_zt(SWIG_CSharpStringHelperCallback callback) {
  SWIG_csharp_string_callback = callback;
}


/* Contract support */

#define SWIG_contract_assert(nullreturn, expr, msg) if (!(expr)) {SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentOutOfRangeException, msg, ""); return nullreturn; } else

/*  Errors in SWIG */
#define  SWIG_UnknownError    	   -1
#define  SWIG_IOError        	   -2
#define  SWIG_RuntimeError   	   -3
#define  SWIG_IndexError     	   -4
#define  SWIG_TypeError      	   -5
#define  SWIG_DivisionByZero 	   -6
#define  SWIG_OverflowError  	   -7
#define  SWIG_SyntaxError    	   -8
#define  SWIG_ValueError     	   -9
#define  SWIG_SystemError    	   -10
#define  SWIG_AttributeError 	   -11
#define  SWIG_MemoryError    	   -12
#define  SWIG_NullReferenceError   -13




#include <typeinfo>
#include <stdexcept>


#include <string>


#include <vector>
#include <algorithm>
#include <stdexcept>


#include <map>
#include <algorithm>
#include <stdexcept>


SWIGINTERN void SWIG_CSharpException(int code, const char *msg) {
  if (code == SWIG_ValueError) {
    SWIG_CSharpExceptionArgumentCodes exception_code = SWIG_CSharpArgumentOutOfRangeException;
    SWIG_CSharpSetPendingExceptionArgument(exception_code, msg, 0);
  } else {
    SWIG_CSharpExceptionCodes exception_code = SWIG_CSharpApplicationException;
    switch(code) {
    case SWIG_MemoryError:
      exception_code = SWIG_CSharpOutOfMemoryException;
      break;
    case SWIG_IndexError:
      exception_code = SWIG_CSharpIndexOutOfRangeException;
      break;
    case SWIG_DivisionByZero:
      exception_code = SWIG_CSharpDivideByZeroException;
      break;
    case SWIG_IOError:
      exception_code = SWIG_CSharpIOException;
      break;
    case SWIG_OverflowError:
      exception_code = SWIG_CSharpOverflowException;
      break;
    case SWIG_RuntimeError:
    case SWIG_TypeError:
    case SWIG_SyntaxError:
    case SWIG_SystemError:
    case SWIG_UnknownError:
    default:
      exception_code = SWIG_CSharpApplicationException;
      break;
    }
    SWIG_CSharpSetPendingException(exception_code, msg);
  }
}


#include <typeinfo>
#include <stdexcept>


#include <utility>


#include "../../include/ZeroTierSockets.h"


#ifdef __cplusplus
extern "C" {
#endif

SWIGEXPORT void SWIGSTDCALL CSharp_zts_errno_set(int jarg1) {
  int arg1 ;
  
  arg1 = (int)jarg1; 
  zts_errno = arg1;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_errno_get() {
  int jresult ;
  int result;
  
  result = (int)zts_errno;
  jresult = result; 
  return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_allow_network_caching(unsigned char jarg1) {
  int jresult ;
  uint8_t arg1 ;
  int result;
  
  arg1 = (uint8_t)jarg1; 
  result = (int)zts_allow_network_caching(arg1);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_allow_peer_caching(unsigned char jarg1) {
  int jresult ;
  uint8_t arg1 ;
  int result;
  
  arg1 = (uint8_t)jarg1; 
  result = (int)zts_allow_peer_caching(arg1);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_allow_local_conf(unsigned char jarg1) {
  int jresult ;
  uint8_t arg1 ;
  int result;
  
  arg1 = (uint8_t)jarg1; 
  result = (int)zts_allow_local_conf(arg1);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_start(char * jarg1, void * jarg2, unsigned short jarg3) {
  int jresult ;
  char *arg1 = (char *) 0 ;
  CppCallback arg2 = (CppCallback) 0 ;
  uint16_t arg3 ;
  int result;
  
  arg1 = (char *)jarg1; 
  arg2 = (CppCallback)jarg2; 
  arg3 = (uint16_t)jarg3; 
  result = (int)zts_start((char const *)arg1,arg2,arg3);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_stop() {
  int jresult ;
  int result;
  
  result = (int)zts_stop();
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_restart() {
  int jresult ;
  int result;
  
  result = (int)zts_restart();
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_free() {
  int jresult ;
  int result;
  
  result = (int)zts_free();
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_join(unsigned long long jarg1) {
  int jresult ;
  uint64_t arg1 ;
  int result;
  
  arg1 = (uint64_t)jarg1; 
  result = (int)zts_join(arg1);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_leave(unsigned long long jarg1) {
  int jresult ;
  uint64_t arg1 ;
  int result;
  
  arg1 = (uint64_t)jarg1; 
  result = (int)zts_leave(arg1);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_orbit(unsigned long long jarg1, unsigned long long jarg2) {
  int jresult ;
  uint64_t arg1 ;
  uint64_t arg2 ;
  int result;
  
  arg1 = (uint64_t)jarg1; 
  arg2 = (uint64_t)jarg2; 
  result = (int)zts_orbit(arg1,arg2);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_deorbit(unsigned long long jarg1) {
  int jresult ;
  uint64_t arg1 ;
  int result;
  
  arg1 = (uint64_t)jarg1; 
  result = (int)zts_deorbit(arg1);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_get_6plane_addr(void * jarg1, unsigned long long jarg2, unsigned long long jarg3) {
  int jresult ;
  zts_sockaddr_storage *arg1 = (zts_sockaddr_storage *) 0 ;
  uint64_t arg2 ;
  uint64_t arg3 ;
  int result;
  
  arg1 = (zts_sockaddr_storage *)jarg1; 
  arg2 = (uint64_t)jarg2; 
  arg3 = (uint64_t)jarg3; 
  result = (int)zts_get_6plane_addr(arg1,arg2,arg3);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_get_rfc4193_addr(void * jarg1, unsigned long long jarg2, unsigned long long jarg3) {
  int jresult ;
  zts_sockaddr_storage *arg1 = (zts_sockaddr_storage *) 0 ;
  uint64_t arg2 ;
  uint64_t arg3 ;
  int result;
  
  arg1 = (zts_sockaddr_storage *)jarg1; 
  arg2 = (uint64_t)jarg2; 
  arg3 = (uint64_t)jarg3; 
  result = (int)zts_get_rfc4193_addr(arg1,arg2,arg3);
  jresult = result; 
  return jresult;
}


SWIGEXPORT unsigned long long SWIGSTDCALL CSharp_zts_generate_adhoc_nwid_from_range(unsigned short jarg1, unsigned short jarg2) {
  unsigned long long jresult ;
  uint16_t arg1 ;
  uint16_t arg2 ;
  uint64_t result;
  
  arg1 = (uint16_t)jarg1; 
  arg2 = (uint16_t)jarg2; 
  result = zts_generate_adhoc_nwid_from_range(arg1,arg2);
  jresult = result; 
  return jresult;
}


SWIGEXPORT void SWIGSTDCALL CSharp_zts_delay_ms(long jarg1) {
  long arg1 ;
  
  arg1 = (long)jarg1; 
  zts_delay_ms(arg1);
}

/*
SWIGEXPORT int SWIGSTDCALL CSharp_zts_get_all_stats(void * jarg1) {
  int jresult ;
  zts_stats *arg1 = (zts_stats *) 0 ;
  int result;
  
  arg1 = (zts_stats *)jarg1; 
  result = (int)zts_get_all_stats(arg1);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_get_protocol_stats(int jarg1, void * jarg2) {
  int jresult ;
  int arg1 ;
  void *arg2 = (void *) 0 ;
  int result;
  
  arg1 = (int)jarg1; 
  arg2 = (void *)jarg2; 
  result = (int)zts_get_protocol_stats(arg1,arg2);
  jresult = result; 
  return jresult;
}
*/

SWIGEXPORT int SWIGSTDCALL CSharp_zts_socket(int jarg1, int jarg2, int jarg3) {
  int jresult ;
  int arg1 ;
  int arg2 ;
  int arg3 ;
  int result;
  
  arg1 = (int)jarg1; 
  arg2 = (int)jarg2; 
  arg3 = (int)jarg3; 
  result = (int)zts_socket(arg1,arg2,arg3);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_connect(int jarg1, zts_sockaddr* jarg2, unsigned short jarg3) {
  int jresult ;
  int arg1 ;
  zts_sockaddr *arg2 = (zts_sockaddr *) 0 ;
  zts_socklen_t arg3 ;
  int result;
  
  arg1 = (int)jarg1; 
  arg2 = (zts_sockaddr *)jarg2; 
  arg3 = (zts_socklen_t)jarg3; 
  result = (int)zts_connect(arg1,(zts_sockaddr const *)arg2,arg3);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_bind(int jarg1, zts_sockaddr* jarg2, unsigned short jarg3) {
  int jresult ;
  int arg1 ;
  zts_sockaddr *arg2 = (zts_sockaddr *) 0 ;
  zts_socklen_t arg3 ;
  int result;
  
  arg1 = (int)jarg1; 
  arg2 = (zts_sockaddr *)jarg2; 
  arg3 = (zts_socklen_t)jarg3; 
  result = (int)zts_bind(arg1,(zts_sockaddr const *)arg2,arg3);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_listen(int jarg1, int jarg2) {
  int jresult ;
  int arg1 ;
  int arg2 ;
  int result;
  
  arg1 = (int)jarg1; 
  arg2 = (int)jarg2; 
  result = (int)zts_listen(arg1,arg2);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_accept(int jarg1, zts_sockaddr* jarg2, int * jarg3) {
  int jresult ;
  int arg1 ;
  zts_sockaddr *arg2 = (zts_sockaddr *) 0 ;
  zts_socklen_t *arg3 = (zts_socklen_t *) 0 ;
  int result;
  
  arg1 = (int)jarg1; 
  arg2 = (zts_sockaddr *)jarg2; 
  arg3 = (zts_socklen_t *)jarg3; 
  result = (int)zts_accept(arg1,arg2,arg3);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_setsockopt(int jarg1, int jarg2, int jarg3, void * jarg4, unsigned short jarg5) {
  int jresult ;
  int arg1 ;
  int arg2 ;
  int arg3 ;
  void *arg4 = (void *) 0 ;
  zts_socklen_t arg5 ;
  int result;
  
  arg1 = (int)jarg1; 
  arg2 = (int)jarg2; 
  arg3 = (int)jarg3; 
  arg4 = (void *)jarg4; 
  arg5 = (zts_socklen_t)jarg5; 
  result = (int)zts_setsockopt(arg1,arg2,arg3,(void const *)arg4,arg5);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_getsockopt(int jarg1, int jarg2, int jarg3, void * jarg4, void * jarg5) {
  int jresult ;
  int arg1 ;
  int arg2 ;
  int arg3 ;
  void *arg4 = (void *) 0 ;
  zts_socklen_t *arg5 = (zts_socklen_t *) 0 ;
  int result;
  
  arg1 = (int)jarg1; 
  arg2 = (int)jarg2; 
  arg3 = (int)jarg3; 
  arg4 = (void *)jarg4; 
  arg5 = (zts_socklen_t *)jarg5; 
  result = (int)zts_getsockopt(arg1,arg2,arg3,arg4,arg5);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_getsockname(int jarg1, zts_sockaddr* jarg2, void * jarg3) {
  int jresult ;
  int arg1 ;
  zts_sockaddr *arg2 = (zts_sockaddr *) 0 ;
  zts_socklen_t *arg3 = (zts_socklen_t *) 0 ;
  int result;
  
  arg1 = (int)jarg1; 
  arg2 = (zts_sockaddr *)jarg2; 
  arg3 = (zts_socklen_t *)jarg3; 
  result = (int)zts_getsockname(arg1,arg2,arg3);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_getpeername(int jarg1, zts_sockaddr* jarg2, void * jarg3) {
  int jresult ;
  int arg1 ;
  zts_sockaddr *arg2 = (zts_sockaddr *) 0 ;
  zts_socklen_t *arg3 = (zts_socklen_t *) 0 ;
  int result;
  
  arg1 = (int)jarg1; 
  arg2 = (zts_sockaddr *)jarg2; 
  arg3 = (zts_socklen_t *)jarg3; 
  result = (int)zts_getpeername(arg1,arg2,arg3);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_close(int jarg1) {
  int jresult ;
  int arg1 ;
  int result;
  
  arg1 = (int)jarg1; 
  result = (int)zts_close(arg1);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_select(int jarg1, void * jarg2, void * jarg3, void * jarg4, void * jarg5) {
  int jresult ;
  int arg1 ;
  zts_fd_set *arg2 = (zts_fd_set *) 0 ;
  zts_fd_set *arg3 = (zts_fd_set *) 0 ;
  zts_fd_set *arg4 = (zts_fd_set *) 0 ;
  zts_timeval *arg5 = (zts_timeval *) 0 ;
  int result;
  
  arg1 = (int)jarg1; 
  arg2 = (zts_fd_set *)jarg2; 
  arg3 = (zts_fd_set *)jarg3; 
  arg4 = (zts_fd_set *)jarg4; 
  arg5 = (zts_timeval *)jarg5; 
  result = (int)zts_select(arg1,arg2,arg3,arg4,arg5);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_fcntl(int jarg1, int jarg2, int jarg3) {
  int jresult ;
  int arg1 ;
  int arg2 ;
  int arg3 ;
  int result;
  
  arg1 = (int)jarg1; 
  arg2 = (int)jarg2; 
  arg3 = (int)jarg3; 
  result = (int)zts_fcntl(arg1,arg2,arg3);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_poll(void * jarg1, unsigned int jarg2, int jarg3) {
  int jresult ;
  zts_pollfd *arg1 = (zts_pollfd *) 0 ;
  zts_nfds_t arg2 ;
  int arg3 ;
  int result;
  
  arg1 = (zts_pollfd *)jarg1; 
  arg2 = (zts_nfds_t)jarg2; 
  arg3 = (int)jarg3; 
  result = (int)zts_poll(arg1,arg2,arg3);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_ioctl(int jarg1, unsigned long jarg2, void * jarg3) {
  int jresult ;
  int arg1 ;
  unsigned long arg2 ;
  void *arg3 = (void *) 0 ;
  int result;
  
  arg1 = (int)jarg1; 
  arg2 = (unsigned long)jarg2; 
  arg3 = (void *)jarg3; 
  result = (int)zts_ioctl(arg1,arg2,arg3);
  jresult = result; 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_zts_send(int jarg1, void * jarg2, unsigned long jarg3, int jarg4) {
  void * jresult ;
  int arg1 ;
  void *arg2 = (void *) 0 ;
  size_t arg3 ;
  int arg4 ;
  ssize_t result;
  
  arg1 = (int)jarg1; 
  arg2 = (void *)jarg2; 
  arg3 = (size_t)jarg3; 
  arg4 = (int)jarg4; 
  result = zts_send(arg1,(void const *)arg2,arg3,arg4);
  jresult = new ssize_t((const ssize_t &)result); 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_zts_sendto(int jarg1, void * jarg2, unsigned long jarg3, int jarg4, zts_sockaddr* jarg5, unsigned short jarg6) {
  void * jresult ;
  int arg1 ;
  void *arg2 = (void *) 0 ;
  size_t arg3 ;
  int arg4 ;
  zts_sockaddr *arg5 = (zts_sockaddr *) 0 ;
  zts_socklen_t arg6 ;
  ssize_t result;
  
  arg1 = (int)jarg1; 
  arg2 = (void *)jarg2; 
  arg3 = (size_t)jarg3; 
  arg4 = (int)jarg4; 
  arg5 = (zts_sockaddr *)jarg5; 
  arg6 = (zts_socklen_t)jarg6; 
  result = zts_sendto(arg1,(void const *)arg2,arg3,arg4,(zts_sockaddr const *)arg5,arg6);
  jresult = new ssize_t((const ssize_t &)result); 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_zts_sendmsg(int jarg1, void * jarg2, int jarg3) {
  void * jresult ;
  int arg1 ;
  msghdr *arg2 = (msghdr *) 0 ;
  int arg3 ;
  ssize_t result;
  
  arg1 = (int)jarg1; 
  arg2 = (msghdr *)jarg2; 
  arg3 = (int)jarg3; 
  result = zts_sendmsg(arg1,(msghdr const *)arg2,arg3);
  jresult = new ssize_t((const ssize_t &)result); 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_recv(int jarg1, void * jarg2, unsigned long jarg3, int jarg4) {
  void * jresult ;
  int arg1 ;
  void *arg2 = (void *) 0 ;
  size_t arg3 ;
  int arg4 ;
  ssize_t result;
  
  arg1 = (int)jarg1; 
  arg2 = (void *)jarg2; 
  arg3 = (size_t)jarg3; 
  arg4 = (int)jarg4; 
  result = zts_recv(arg1,arg2,arg3,arg4);
  return result;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_zts_recvfrom(int jarg1, void * jarg2, unsigned long jarg3, int jarg4, zts_sockaddr* jarg5, void * jarg6) {
  void * jresult ;
  int arg1 ;
  void *arg2 = (void *) 0 ;
  size_t arg3 ;
  int arg4 ;
  zts_sockaddr *arg5 = (zts_sockaddr *) 0 ;
  zts_socklen_t *arg6 = (zts_socklen_t *) 0 ;
  ssize_t result;
  
  arg1 = (int)jarg1; 
  arg2 = (void *)jarg2; 
  arg3 = (size_t)jarg3; 
  arg4 = (int)jarg4; 
  arg5 = (zts_sockaddr *)jarg5; 
  arg6 = (zts_socklen_t *)jarg6; 
  result = zts_recvfrom(arg1,arg2,arg3,arg4,arg5,arg6);
  jresult = new ssize_t((const ssize_t &)result); 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_zts_recvmsg(int jarg1, void * jarg2, int jarg3) {
  void * jresult ;
  int arg1 ;
  msghdr *arg2 = (msghdr *) 0 ;
  int arg3 ;
  ssize_t result;
  
  arg1 = (int)jarg1; 
  arg2 = (msghdr *)jarg2; 
  arg3 = (int)jarg3; 
  result = zts_recvmsg(arg1,arg2,arg3);
  jresult = new ssize_t((const ssize_t &)result); 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_zts_read(int jarg1, void * jarg2, unsigned long jarg3) {
  void * jresult ;
  int arg1 ;
  void *arg2 = (void *) 0 ;
  size_t arg3 ;
  ssize_t result;
  
  arg1 = (int)jarg1; 
  arg2 = (void *)jarg2; 
  arg3 = (size_t)jarg3; 
  result = zts_read(arg1,arg2,arg3);
  jresult = new ssize_t((const ssize_t &)result); 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_zts_readv(int jarg1, void * jarg2, int jarg3) {
  void * jresult ;
  int arg1 ;
  zts_iovec *arg2 = (zts_iovec *) 0 ;
  int arg3 ;
  ssize_t result;
  
  arg1 = (int)jarg1; 
  arg2 = (zts_iovec *)jarg2; 
  arg3 = (int)jarg3; 
  result = zts_readv(arg1,(zts_iovec const *)arg2,arg3);
  jresult = new ssize_t((const ssize_t &)result); 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_zts_write(int jarg1, void * jarg2, unsigned long jarg3) {
  void * jresult ;
  int arg1 ;
  void *arg2 = (void *) 0 ;
  size_t arg3 ;
  ssize_t result;
  
  arg1 = (int)jarg1; 
  arg2 = (void *)jarg2; 
  arg3 = (size_t)jarg3; 
  result = zts_write(arg1,(void const *)arg2,arg3);
  jresult = new ssize_t((const ssize_t &)result); 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_zts_writev(int jarg1, void * jarg2, int jarg3) {
  void * jresult ;
  int arg1 ;
  zts_iovec *arg2 = (zts_iovec *) 0 ;
  int arg3 ;
  ssize_t result;
  
  arg1 = (int)jarg1; 
  arg2 = (zts_iovec *)jarg2; 
  arg3 = (int)jarg3; 
  result = zts_writev(arg1,(zts_iovec const *)arg2,arg3);
  jresult = new ssize_t((const ssize_t &)result); 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_shutdown(int jarg1, int jarg2) {
  int jresult ;
  int arg1 ;
  int arg2 ;
  int result;
  
  arg1 = (int)jarg1; 
  arg2 = (int)jarg2; 
  result = (int)zts_shutdown(arg1,arg2);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_add_dns_nameserver(zts_sockaddr* jarg1) {
  int jresult ;
  zts_sockaddr *arg1 = (zts_sockaddr *) 0 ;
  int result;
  
  arg1 = (zts_sockaddr *)jarg1; 
  result = (int)zts_add_dns_nameserver(arg1);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_del_dns_nameserver(zts_sockaddr* jarg1) {
  int jresult ;
  zts_sockaddr *arg1 = (zts_sockaddr *) 0 ;
  int result;
  
  arg1 = (zts_sockaddr *)jarg1; 
  result = (int)zts_del_dns_nameserver(arg1);
  jresult = result; 
  return jresult;
}


SWIGEXPORT unsigned short SWIGSTDCALL CSharp_zts_htons(unsigned short jarg1) {
  unsigned short jresult ;
  uint16_t arg1 ;
  uint16_t result;
  
  arg1 = (uint16_t)jarg1; 
  result = zts_htons(arg1);
  jresult = result; 
  return jresult;
}


SWIGEXPORT unsigned short SWIGSTDCALL CSharp_zts_htonl(unsigned short jarg1) {
  unsigned short jresult ;
  uint32_t arg1 ;
  uint32_t result;
  
  arg1 = (uint32_t)jarg1; 
  result = zts_htonl(arg1);
  jresult = result; 
  return jresult;
}

SWIGEXPORT unsigned short SWIGSTDCALL CSharp_zts_ntohs(unsigned short jarg1) {
  unsigned short jresult ;
  uint16_t arg1 ;
  uint16_t result;
  
  arg1 = (uint16_t)jarg1; 
  result = zts_ntohs(arg1);
  jresult = result; 
  return jresult;
}


SWIGEXPORT unsigned short SWIGSTDCALL CSharp_zts_ntohl(unsigned short jarg1) {
  unsigned short jresult ;
  uint32_t arg1 ;
  uint32_t result;
  
  arg1 = (uint32_t)jarg1; 
  result = zts_ntohl(arg1);
  jresult = result; 
  return jresult;
}


SWIGEXPORT char * SWIGSTDCALL CSharp_zts_inet_ntop(int jarg1, void * jarg2, char * jarg3, unsigned short jarg4) {
  char * jresult ;
  int arg1 ;
  void *arg2 = (void *) 0 ;
  char *arg3 = (char *) 0 ;
  zts_socklen_t arg4 ;
  char *result = 0 ;
  
  arg1 = (int)jarg1; 
  arg2 = (void *)jarg2; 
  arg3 = (char *)jarg3; 
  arg4 = (zts_socklen_t)jarg4; 
  result = (char *)zts_inet_ntop(arg1,(void const *)arg2,arg3,arg4);
  jresult = SWIG_csharp_string_callback((const char *)result); 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_zts_inet_pton(int jarg1, char * jarg2, void * jarg3) {
  int jresult ;
  int arg1 ;
  char *arg2 = (char *) 0 ;
  void *arg3 = (void *) 0 ;
  int result;
  
  arg1 = (int)jarg1; 
  arg2 = (char *)jarg2; 
  arg3 = (void *)jarg3; 
  result = (int)zts_inet_pton(arg1,(char const *)arg2,arg3);
  jresult = result; 
  return jresult;
}

#ifdef __cplusplus
}
#endif

