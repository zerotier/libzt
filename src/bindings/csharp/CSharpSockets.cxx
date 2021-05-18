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

#ifndef SWIGCSHARP
#define SWIGCSHARP
#endif

#ifdef __cplusplus
/* SwigValueWrapper is described in swig.swg */
template <typename T> class SwigValueWrapper {
    struct SwigMovePointer {
        T* ptr;
        SwigMovePointer(T* p) : ptr(p)
        {
        }
        ~SwigMovePointer()
        {
            delete ptr;
        }
        SwigMovePointer& operator=(SwigMovePointer& rhs)
        {
            T* oldptr = ptr;
            ptr = 0;
            delete oldptr;
            ptr = rhs.ptr;
            rhs.ptr = 0;
            return *this;
        }
    } pointer;
    SwigValueWrapper& operator=(const SwigValueWrapper<T>& rhs);
    SwigValueWrapper(const SwigValueWrapper<T>& rhs);

  public:
    SwigValueWrapper() : pointer(0)
    {
    }
    SwigValueWrapper& operator=(const T& t)
    {
        SwigMovePointer tmp(new T(t));
        pointer = tmp;
        return *this;
    }
    operator T&() const
    {
        return *pointer.ptr;
    }
    T* operator&()
    {
        return pointer.ptr;
    }
};

template <typename T> T SwigValueInit()
{
    return T();
}
#endif

/* -----------------------------------------------------------------------------
 *  This section contains generic SWIG labels for method/variable
 *  declarations/attributes, and other compiler dependent labels.
 * ----------------------------------------------------------------------------- */

/* template workaround for compilers that cannot correctly implement the C++ standard */
#ifndef SWIGTEMPLATEDISAMBIGUATOR
#if defined(__SUNPRO_CC) && (__SUNPRO_CC <= 0x560)
#define SWIGTEMPLATEDISAMBIGUATOR template
#elif defined(__HP_aCC)
/* Needed even with `aCC -AA' when `aCC -V' reports HP ANSI C++ B3910B A.03.55 */
/* If we find a maximum version that requires this, the test would be __HP_aCC <= 35500 for A.03.55 */
#define SWIGTEMPLATEDISAMBIGUATOR template
#else
#define SWIGTEMPLATEDISAMBIGUATOR
#endif
#endif

/* inline attribute */
#ifndef SWIGINLINE
#if defined(__cplusplus) || (defined(__GNUC__) && ! defined(__STRICT_ANSI__))
#define SWIGINLINE inline
#else
#define SWIGINLINE
#endif
#endif

/* attribute recognised by some compilers to avoid 'unused' warnings */
#ifndef SWIGUNUSED
#if defined(__GNUC__)
#if ! (defined(__cplusplus)) || (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))
#define SWIGUNUSED __attribute__((__unused__))
#else
#define SWIGUNUSED
#endif
#elif defined(__ICC)
#define SWIGUNUSED __attribute__((__unused__))
#else
#define SWIGUNUSED
#endif
#endif

#ifndef SWIG_MSC_UNSUPPRESS_4505
#if defined(_MSC_VER)
#pragma warning(disable : 4505) /* unreferenced local function has been removed */
#endif
#endif

#ifndef SWIGUNUSEDPARM
#ifdef __cplusplus
#define SWIGUNUSEDPARM(p)
#else
#define SWIGUNUSEDPARM(p) p SWIGUNUSED
#endif
#endif

/* internal SWIG method */
#ifndef SWIGINTERN
#define SWIGINTERN static SWIGUNUSED
#endif

/* internal inline SWIG method */
#ifndef SWIGINTERNINLINE
#define SWIGINTERNINLINE SWIGINTERN SWIGINLINE
#endif

/* exporting methods */
#if defined(__GNUC__)
#if (__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#ifndef GCC_HASCLASSVISIBILITY
#define GCC_HASCLASSVISIBILITY
#endif
#endif
#endif

#ifndef SWIGEXPORT
#if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#if defined(STATIC_LINKED)
#define SWIGEXPORT
#else
#define SWIGEXPORT __declspec(dllexport)
#endif
#else
#if defined(__GNUC__) && defined(GCC_HASCLASSVISIBILITY)
#define SWIGEXPORT __attribute__((visibility("default")))
#else
#define SWIGEXPORT
#endif
#endif
#endif

/* calling conventions for Windows */
#ifndef SWIGSTDCALL
#if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#define SWIGSTDCALL __stdcall
#else
#define SWIGSTDCALL
#endif
#endif

/* Deal with Microsoft's attempt at deprecating C standard runtime functions */
#if ! defined(SWIG_NO_CRT_SECURE_NO_DEPRECATE) && defined(_MSC_VER) && ! defined(_CRT_SECURE_NO_DEPRECATE)
#define _CRT_SECURE_NO_DEPRECATE
#endif

/* Deal with Microsoft's attempt at deprecating methods in the standard C++ library */
#if ! defined(SWIG_NO_SCL_SECURE_NO_DEPRECATE) && defined(_MSC_VER) && ! defined(_SCL_SECURE_NO_DEPRECATE)
#define _SCL_SECURE_NO_DEPRECATE
#endif

/* Deal with Apple's deprecated 'AssertMacros.h' from Carbon-framework */
#if defined(__APPLE__) && ! defined(__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES)
#define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0
#endif

/* Intel's compiler complains if a variable which was never initialised is
 * cast to void, which is a common idiom which we use to indicate that we
 * are aware a variable isn't used.  So we just silence that warning.
 * See: https://github.com/swig/swig/issues/192 for more discussion.
 */
#ifdef __INTEL_COMPILER
#pragma warning disable 592
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

typedef void(SWIGSTDCALL* SWIG_CSharpExceptionCallback_t)(const char*);
typedef void(SWIGSTDCALL* SWIG_CSharpExceptionArgumentCallback_t)(const char*, const char*);

typedef struct {
    SWIG_CSharpExceptionCodes code;
    SWIG_CSharpExceptionCallback_t callback;
} SWIG_CSharpException_t;

typedef struct {
    SWIG_CSharpExceptionArgumentCodes code;
    SWIG_CSharpExceptionArgumentCallback_t callback;
} SWIG_CSharpExceptionArgument_t;

static SWIG_CSharpException_t SWIG_csharp_exceptions[] = {
    { SWIG_CSharpApplicationException, NULL },  { SWIG_CSharpArithmeticException, NULL },
    { SWIG_CSharpDivideByZeroException, NULL }, { SWIG_CSharpIndexOutOfRangeException, NULL },
    { SWIG_CSharpInvalidCastException, NULL },  { SWIG_CSharpInvalidOperationException, NULL },
    { SWIG_CSharpIOException, NULL },           { SWIG_CSharpNullReferenceException, NULL },
    { SWIG_CSharpOutOfMemoryException, NULL },  { SWIG_CSharpOverflowException, NULL },
    { SWIG_CSharpSystemException, NULL }
};

static SWIG_CSharpExceptionArgument_t SWIG_csharp_exceptions_argument[] = { { SWIG_CSharpArgumentException, NULL },
                                                                            { SWIG_CSharpArgumentNullException, NULL },
                                                                            { SWIG_CSharpArgumentOutOfRangeException,
                                                                              NULL } };

static void SWIGUNUSED SWIG_CSharpSetPendingException(SWIG_CSharpExceptionCodes code, const char* msg)
{
    SWIG_CSharpExceptionCallback_t callback = SWIG_csharp_exceptions[SWIG_CSharpApplicationException].callback;
    if ((size_t)code < sizeof(SWIG_csharp_exceptions) / sizeof(SWIG_CSharpException_t)) {
        callback = SWIG_csharp_exceptions[code].callback;
    }
    callback(msg);
}

static void SWIGUNUSED
SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpExceptionArgumentCodes code, const char* msg, const char* param_name)
{
    SWIG_CSharpExceptionArgumentCallback_t callback =
        SWIG_csharp_exceptions_argument[SWIG_CSharpArgumentException].callback;
    if ((size_t)code < sizeof(SWIG_csharp_exceptions_argument) / sizeof(SWIG_CSharpExceptionArgument_t)) {
        callback = SWIG_csharp_exceptions_argument[code].callback;
    }
    callback(msg, param_name);
}

#ifdef __cplusplus
extern "C"
#endif
    SWIGEXPORT void SWIGSTDCALL
    SWIGRegisterExceptionCallbacks_zt(
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
        SWIG_CSharpExceptionCallback_t systemCallback)
{
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
    SWIGEXPORT void SWIGSTDCALL
    SWIGRegisterExceptionArgumentCallbacks_zt(
        SWIG_CSharpExceptionArgumentCallback_t argumentCallback,
        SWIG_CSharpExceptionArgumentCallback_t argumentNullCallback,
        SWIG_CSharpExceptionArgumentCallback_t argumentOutOfRangeCallback)
{
    SWIG_csharp_exceptions_argument[SWIG_CSharpArgumentException].callback = argumentCallback;
    SWIG_csharp_exceptions_argument[SWIG_CSharpArgumentNullException].callback = argumentNullCallback;
    SWIG_csharp_exceptions_argument[SWIG_CSharpArgumentOutOfRangeException].callback = argumentOutOfRangeCallback;
}

/* Callback for returning strings to C# without leaking memory */
typedef char*(SWIGSTDCALL* SWIG_CSharpStringHelperCallback)(const char*);
static SWIG_CSharpStringHelperCallback SWIG_csharp_string_callback = NULL;

#ifdef __cplusplus
extern "C"
#endif
    SWIGEXPORT void SWIGSTDCALL
    SWIGRegisterStringCallback_zt(SWIG_CSharpStringHelperCallback callback)
{
    SWIG_csharp_string_callback = callback;
}

/* Contract support */

#define SWIG_contract_assert(nullreturn, expr, msg)                                                                    \
    if (! (expr)) {                                                                                                    \
        SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentOutOfRangeException, msg, "");                       \
        return nullreturn;                                                                                             \
    }                                                                                                                  \
    else

/*  Errors in SWIG */
#define SWIG_UnknownError       -1
#define SWIG_IOError            -2
#define SWIG_RuntimeError       -3
#define SWIG_IndexError         -4
#define SWIG_TypeError          -5
#define SWIG_DivisionByZero     -6
#define SWIG_OverflowError      -7
#define SWIG_SyntaxError        -8
#define SWIG_ValueError         -9
#define SWIG_SystemError        -10
#define SWIG_AttributeError     -11
#define SWIG_MemoryError        -12
#define SWIG_NullReferenceError -13

#include <algorithm>
#include <map>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

SWIGINTERN void SWIG_CSharpException(int code, const char* msg)
{
    if (code == SWIG_ValueError) {
        SWIG_CSharpExceptionArgumentCodes exception_code = SWIG_CSharpArgumentOutOfRangeException;
        SWIG_CSharpSetPendingExceptionArgument(exception_code, msg, 0);
    }
    else {
        SWIG_CSharpExceptionCodes exception_code = SWIG_CSharpApplicationException;
        switch (code) {
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

#include "../../../include/ZeroTierSockets.h"

#include <stdexcept>
#include <typeinfo>
#include <utility>

#ifdef __cplusplus
extern "C" {
#endif

SWIGEXPORT int SWIGSTDCALL CSharp_zts_errno_get()
{
    return zts_errno;
}

#ifndef ZTS_DISABLE_CENTRAL_API

SWIGEXPORT int SWIGSTDCALL CSharp_zts_central_set_access_mode(char jarg1)
{
    int jresult;
    int8_t arg1;
    int result;
    arg1 = (int8_t)jarg1;
    result = (int)zts_central_set_access_mode(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_central_set_verbose(char jarg1)
{
    int jresult;
    int8_t arg1;
    int result;
    arg1 = (int8_t)jarg1;
    result = (int)zts_central_set_verbose(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT void SWIGSTDCALL CSharp_zts_central_clear_resp_buf()
{
    zts_central_clear_resp_buf();
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_central_init(char* jarg1, char* jarg2, char* jarg3, unsigned short jarg4)
{
    int jresult;
    char* arg1 = (char*)0;
    char* arg2 = (char*)0;
    char* arg3 = (char*)0;
    uint32_t arg4;
    int result;
    arg1 = (char*)jarg1;
    arg2 = (char*)jarg2;
    arg3 = (char*)jarg3;
    arg4 = (uint32_t)jarg4;
    result = (int)zts_central_init((char const*)arg1, (char const*)arg2, arg3, arg4);
    jresult = result;
    return jresult;
}

SWIGEXPORT void SWIGSTDCALL CSharp_zts_central_cleanup()
{
    zts_central_cleanup();
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_central_get_last_resp_buf(char* jarg1, int jarg2)
{
    int jresult;
    char* arg1 = (char*)0;
    int arg2;
    int result;
    arg1 = (char*)jarg1;
    arg2 = (int)jarg2;
    result = (int)zts_central_get_last_resp_buf(arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_central_status_get(void* jarg1)
{
    int jresult;
    int* arg1 = (int*)0;
    int result;
    arg1 = (int*)jarg1;
    result = (int)zts_central_status_get(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_central_self_get(void* jarg1)
{
    int jresult;
    int* arg1 = (int*)0;
    int result;
    arg1 = (int*)jarg1;
    result = (int)zts_central_self_get(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_central_net_get(void* jarg1, unsigned long long jarg2)
{
    int jresult;
    int* arg1 = (int*)0;
    uint64_t arg2;
    int result;
    arg1 = (int*)jarg1;
    arg2 = (uint64_t)jarg2;
    result = (int)zts_central_net_get(arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_central_net_update(void* jarg1, unsigned long long jarg2)
{
    int jresult;
    int* arg1 = (int*)0;
    uint64_t arg2;
    int result;
    arg1 = (int*)jarg1;
    arg2 = (uint64_t)jarg2;
    result = (int)zts_central_net_update(arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_central_net_delete(void* jarg1, unsigned long long jarg2)
{
    int jresult;
    int* arg1 = (int*)0;
    uint64_t arg2;
    int result;
    arg1 = (int*)jarg1;
    arg2 = (uint64_t)jarg2;
    result = (int)zts_central_net_delete(arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_central_net_get_all(void* jarg1)
{
    int jresult;
    int* arg1 = (int*)0;
    int result;
    arg1 = (int*)jarg1;
    result = (int)zts_central_net_get_all(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL
CSharp_zts_central_member_get(void* jarg1, unsigned long long jarg2, unsigned long long jarg3)
{
    int jresult;
    int* arg1 = (int*)0;
    uint64_t arg2;
    uint64_t arg3;
    int result;
    arg1 = (int*)jarg1;
    arg2 = (uint64_t)jarg2;
    arg3 = (uint64_t)jarg3;
    result = (int)zts_central_member_get(arg1, arg2, arg3);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL
CSharp_zts_central_member_update(void* jarg1, unsigned long long jarg2, unsigned long long jarg3, char* jarg4)
{
    int jresult;
    int* arg1 = (int*)0;
    uint64_t arg2;
    uint64_t arg3;
    char* arg4 = (char*)0;
    int result;
    arg1 = (int*)jarg1;
    arg2 = (uint64_t)jarg2;
    arg3 = (uint64_t)jarg3;
    arg4 = (char*)jarg4;
    result = (int)zts_central_member_update(arg1, arg2, arg3, arg4);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL
CSharp_zts_central_node_auth(void* jarg1, unsigned long long jarg2, unsigned long long jarg3, unsigned char jarg4)
{
    int jresult;
    int* arg1 = (int*)0;
    uint64_t arg2;
    uint64_t arg3;
    uint8_t arg4;
    int result;
    arg1 = (int*)jarg1;
    arg2 = (uint64_t)jarg2;
    arg3 = (uint64_t)jarg3;
    arg4 = (uint8_t)jarg4;
    result = (int)zts_central_node_auth(arg1, arg2, arg3, arg4);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_central_net_get_members(void* jarg1, unsigned long long jarg2)
{
    int jresult;
    int* arg1 = (int*)0;
    uint64_t arg2;
    int result;
    arg1 = (int*)jarg1;
    arg2 = (uint64_t)jarg2;
    result = (int)zts_central_net_get_members(arg1, arg2);
    jresult = result;
    return jresult;
}

#endif   // ZTS_DISABLE_CENTRAL_API

SWIGEXPORT int SWIGSTDCALL CSharp_zts_id_new(char* jarg1, void* jarg2)
{
    int jresult;
    char* arg1 = (char*)0;
    unsigned int* arg2 = (unsigned int*)0;
    int result;
    arg1 = (char*)jarg1;
    arg2 = (unsigned int*)jarg2;
    result = (int)zts_id_new(arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_id_pair_is_valid(char* jarg1, int jarg2)
{
    int jresult;
    char* arg1 = (char*)0;
    int arg2;
    int result;
    arg1 = (char*)jarg1;
    arg2 = (int)jarg2;
    result = (int)zts_id_pair_is_valid((char const*)arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_init_from_storage(char* jarg1)
{
    int jresult;
    char* arg1 = (char*)0;
    int result;
    arg1 = (char*)jarg1;
    result = (int)zts_init_from_storage((char const*)arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_init_from_memory(char* jarg1, unsigned short jarg2)
{
    int jresult;
    char* arg1 = (char*)0;
    uint16_t arg2;
    int result;
    arg1 = (char*)jarg1;
    arg2 = (uint16_t)jarg2;
    result = (int)zts_init_from_memory((char const*)arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_init_set_event_handler(void* jarg1)
{
    int jresult;
    CppCallback arg1 = (CppCallback)0;
    int result;
    arg1 = (CppCallback)jarg1;
    result = (int)zts_init_set_event_handler(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_init_blacklist_if(char* jarg1, int jarg2)
{
    int jresult;
    char* arg1 = (char*)0;
    int arg2;
    int result;
    arg1 = (char*)jarg1;
    arg2 = (int)jarg2;
    result = (int)zts_init_blacklist_if((char const*)arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_init_set_roots(void* jarg1, int jarg2)
{
    int jresult;
    char* arg1 = (char*)0;
    int arg2;
    int result;
    arg1 = (char*)jarg1;
    arg2 = (int)jarg2;
    result = (int)zts_init_set_roots((char const*)arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_init_set_port(unsigned short port)
{
    return zts_init_set_port(port);
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_init_set_random_port_range(unsigned short start_port, unsigned short end_port)
{
    return zts_init_set_random_port_range(start_port, end_port);
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_init_allow_secondary_port(int allowed)
{
    return zts_init_allow_secondary_port(allowed);
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_init_allow_port_mapping(int allowed)
{
    return zts_init_allow_port_mapping(allowed);
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_init_allow_net_cache(int jarg1)
{
    int jresult;
    int arg1;
    int result;
    arg1 = (int)jarg1;
    result = (int)zts_init_allow_net_cache(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_init_allow_peer_cache(int jarg1)
{
    int jresult;
    int arg1;
    int result;
    arg1 = (int)jarg1;
    result = (int)zts_init_allow_peer_cache(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_addr_is_assigned(unsigned long long jarg1, int jarg2)
{
    int jresult;
    uint64_t arg1;
    int arg2;
    int result;
    arg1 = (uint64_t)jarg1;
    arg2 = (int)jarg2;
    result = (int)zts_addr_is_assigned(arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_addr_get(unsigned long long jarg1, int jarg2, void* jarg3)
{
    int jresult;
    uint64_t arg1;
    int arg2;
    zts_sockaddr_storage* arg3 = (zts_sockaddr_storage*)0;
    int result;
    arg1 = (uint64_t)jarg1;
    arg2 = (int)jarg2;
    arg3 = (zts_sockaddr_storage*)jarg3;
    result = (int)zts_addr_get(arg1, arg2, arg3);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_addr_get_str(unsigned long long jarg1, int jarg2, char* jarg3, int jarg4)
{
    int jresult;
    uint64_t arg1;
    int arg2;
    char* arg3 = (char*)0;
    int arg4;
    int result;
    arg1 = (uint64_t)jarg1;
    arg2 = (int)jarg2;
    arg3 = (char*)jarg3;
    arg4 = (int)jarg4;
    result = (int)zts_addr_get_str(arg1, arg2, arg3, arg4);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_addr_get_all(unsigned long long jarg1, void* jarg2, void* jarg3)
{
    int jresult;
    uint64_t arg1;
    zts_sockaddr_storage* arg2 = (zts_sockaddr_storage*)0;
    unsigned int* arg3 = (unsigned int*)0;
    int result;
    arg1 = (uint64_t)jarg1;
    arg2 = (zts_sockaddr_storage*)jarg2;
    arg3 = (unsigned int*)jarg3;
    result = (int)zts_addr_get_all(arg1, arg2, arg3);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL
CSharp_zts_addr_compute_6plane(unsigned long long jarg1, unsigned long long jarg2, void* jarg3)
{
    int jresult;
    uint64_t arg1;
    uint64_t arg2;
    zts_sockaddr_storage* arg3 = (zts_sockaddr_storage*)0;
    int result;
    arg1 = (uint64_t)jarg1;
    arg2 = (uint64_t)jarg2;
    arg3 = (zts_sockaddr_storage*)jarg3;
    result = (int)zts_addr_compute_6plane(arg1, arg2, arg3);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL
CSharp_zts_addr_compute_rfc4193(unsigned long long jarg1, unsigned long long jarg2, void* jarg3)
{
    int jresult;
    uint64_t arg1;
    uint64_t arg2;
    zts_sockaddr_storage* arg3 = (zts_sockaddr_storage*)0;
    int result;
    arg1 = (uint64_t)jarg1;
    arg2 = (uint64_t)jarg2;
    arg3 = (zts_sockaddr_storage*)jarg3;
    result = (int)zts_addr_compute_rfc4193(arg1, arg2, arg3);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL
CSharp_zts_addr_compute_rfc4193_str(unsigned long long jarg1, unsigned long long jarg2, char* jarg3, int jarg4)
{
    int jresult;
    uint64_t arg1;
    uint64_t arg2;
    char* arg3 = (char*)0;
    int arg4;
    int result;
    arg1 = (uint64_t)jarg1;
    arg2 = (uint64_t)jarg2;
    arg3 = (char*)jarg3;
    arg4 = (int)jarg4;
    result = (int)zts_addr_compute_rfc4193_str(arg1, arg2, arg3, arg4);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL
CSharp_zts_addr_compute_6plane_str(unsigned long long jarg1, unsigned long long jarg2, char* jarg3, int jarg4)
{
    int jresult;
    uint64_t arg1;
    uint64_t arg2;
    char* arg3 = (char*)0;
    int arg4;
    int result;
    arg1 = (uint64_t)jarg1;
    arg2 = (uint64_t)jarg2;
    arg3 = (char*)jarg3;
    arg4 = (int)jarg4;
    result = (int)zts_addr_compute_6plane_str(arg1, arg2, arg3, arg4);
    jresult = result;
    return jresult;
}

SWIGEXPORT unsigned long long SWIGSTDCALL CSharp_zts_net_compute_adhoc_id(unsigned short jarg1, unsigned short jarg2)
{
    unsigned long long jresult;
    uint16_t arg1;
    uint16_t arg2;
    uint64_t result;
    arg1 = (uint16_t)jarg1;
    arg2 = (uint16_t)jarg2;
    result = zts_net_compute_adhoc_id(arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_net_join(unsigned long long jarg1)
{
    int jresult;
    uint64_t arg1;
    int result;
    arg1 = (uint64_t)jarg1;
    result = (int)zts_net_join(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_net_leave(unsigned long long jarg1)
{
    int jresult;
    uint64_t arg1;
    int result;
    arg1 = (uint64_t)jarg1;
    result = (int)zts_net_leave(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_net_transport_is_ready(unsigned long long jarg1)
{
    int jresult;
    uint64_t arg1;
    int result;
    arg1 = (uint64_t)jarg1;
    result = (int)zts_net_transport_is_ready(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT unsigned long long SWIGSTDCALL CSharp_zts_net_get_mac(unsigned long long jarg1)
{
    unsigned long long jresult;
    uint64_t arg1;
    uint64_t result;
    arg1 = (uint64_t)jarg1;
    result = zts_net_get_mac(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_net_get_mac_str(unsigned long long jarg1, char* jarg2, int jarg3)
{
    int jresult;
    uint64_t arg1;
    char* arg2 = (char*)0;
    int arg3;
    int result;
    arg1 = (uint64_t)jarg1;
    arg2 = (char*)jarg2;
    arg3 = (int)jarg3;
    result = (int)zts_net_get_mac_str(arg1, arg2, arg3);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_net_get_broadcast(unsigned long long jarg1)
{
    int jresult;
    uint64_t arg1;
    int result;
    arg1 = (uint64_t)jarg1;
    result = (int)zts_net_get_broadcast(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_net_get_mtu(unsigned long long jarg1)
{
    int jresult;
    uint64_t arg1;
    int result;
    arg1 = (uint64_t)jarg1;
    result = (int)zts_net_get_mtu(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_net_get_name(unsigned long long jarg1, char* jarg2, int jarg3)
{
    int jresult;
    uint64_t arg1;
    char* arg2 = (char*)0;
    int arg3;
    int result;
    arg1 = (uint64_t)jarg1;
    arg2 = (char*)jarg2;
    arg3 = (int)jarg3;
    result = (int)zts_net_get_name(arg1, arg2, arg3);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_net_get_status(unsigned long long jarg1)
{
    int jresult;
    uint64_t arg1;
    int result;
    arg1 = (uint64_t)jarg1;
    result = (int)zts_net_get_status(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_net_get_type(unsigned long long jarg1)
{
    int jresult;
    uint64_t arg1;
    int result;
    arg1 = (uint64_t)jarg1;
    result = (int)zts_net_get_type(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_route_is_assigned(unsigned long long jarg1, int jarg2)
{
    int jresult;
    uint64_t arg1;
    int arg2;
    int result;
    arg1 = (uint64_t)jarg1;
    arg2 = (int)jarg2;
    result = (int)zts_route_is_assigned(arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_node_start()
{
    int jresult;
    int result;

    result = (int)zts_node_start();
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_node_is_online()
{
    int jresult;
    int result;

    result = (int)zts_node_is_online();
    jresult = result;
    return jresult;
}

SWIGEXPORT unsigned long long SWIGSTDCALL CSharp_zts_node_get_id()
{
    unsigned long long jresult;
    uint64_t result;

    result = zts_node_get_id();
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_node_get_id_pair(char* jarg1, void* jarg2)
{
    int jresult;
    char* arg1 = (char*)0;
    unsigned int* arg2 = (unsigned int*)0;
    int result;
    arg1 = (char*)jarg1;
    arg2 = (unsigned int*)jarg2;
    result = (int)zts_node_get_id_pair(arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_node_get_port()
{
    int jresult;
    int result;

    result = (int)zts_node_get_port();
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_node_stop()
{
    int jresult;
    int result;

    result = (int)zts_node_stop();
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_node_free()
{
    int jresult;
    int result;

    result = (int)zts_node_free();
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_moon_orbit(unsigned long long jarg1, unsigned long long jarg2)
{
    int jresult;
    uint64_t arg1;
    uint64_t arg2;
    int result;
    arg1 = (uint64_t)jarg1;
    arg2 = (uint64_t)jarg2;
    result = (int)zts_moon_orbit(arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_moon_deorbit(unsigned long long jarg1)
{
    int jresult;
    uint64_t arg1;
    int result;
    arg1 = (uint64_t)jarg1;
    result = (int)zts_moon_deorbit(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_stats_get_all(void* jarg1)
{
    int jresult;
    zts_stats_counter_t* arg1 = (zts_stats_counter_t*)0;
    int result;
    arg1 = (zts_stats_counter_t*)jarg1;
    result = (int)zts_stats_get_all(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_socket(int jarg1, int jarg2, int jarg3)
{
    int jresult;
    int arg1;
    int arg2;
    int arg3;
    int result;
    arg1 = (int)jarg1;
    arg2 = (int)jarg2;
    arg3 = (int)jarg3;
    result = (int)zts_bsd_socket(arg1, arg2, arg3);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_connect(int jarg1, zts_sockaddr* jarg2, unsigned short jarg3)
{
    int jresult;
    int arg1;
    zts_sockaddr* arg2 = (zts_sockaddr*)0;
    zts_socklen_t arg3;
    int result;
    arg1 = (int)jarg1;
    arg2 = (zts_sockaddr*)jarg2;
    arg3 = (zts_socklen_t)jarg3;
    result = (int)zts_bsd_connect(arg1, (zts_sockaddr const*)arg2, arg3);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_connect(int jarg1, char* jarg2, int jarg3, int jarg4)
{
    int jresult;
    int arg1;
    char* arg2 = (char*)0;
    int arg3;
    int arg4;
    int result;
    arg1 = (int)jarg1;
    arg2 = (char*)jarg2;
    arg3 = (int)jarg3;
    arg4 = (int)jarg4;
    result = (int)zts_connect(arg1, (char const*)arg2, arg3, arg4);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_bind(int jarg1, zts_sockaddr* jarg2, unsigned short jarg3)
{
    int jresult;
    int arg1;
    zts_sockaddr* arg2 = (zts_sockaddr*)0;
    zts_socklen_t arg3;
    int result;
    arg1 = (int)jarg1;
    arg2 = (zts_sockaddr*)jarg2;
    arg3 = (zts_socklen_t)jarg3;
    result = (int)zts_bsd_bind(arg1, (zts_sockaddr const*)arg2, arg3);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bind(int jarg1, char* jarg2, int jarg3)
{
    int jresult;
    int arg1;
    char* arg2 = (char*)0;
    int arg3;
    int result;
    arg1 = (int)jarg1;
    arg2 = (char*)jarg2;
    arg3 = (int)jarg3;
    result = (int)zts_bind(arg1, (char const*)arg2, arg3);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_listen(int jarg1, int jarg2)
{
    int jresult;
    int arg1;
    int arg2;
    int result;
    arg1 = (int)jarg1;
    arg2 = (int)jarg2;
    result = (int)zts_bsd_listen(arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_accept(int jarg1, zts_sockaddr* jarg2, void* jarg3)
{
    int jresult;
    int arg1;
    zts_sockaddr* arg2 = (zts_sockaddr*)0;
    zts_socklen_t* arg3 = (zts_socklen_t*)0;
    int result;
    arg1 = (int)jarg1;
    arg2 = (zts_sockaddr*)jarg2;
    arg3 = (zts_socklen_t*)jarg3;
    result = (int)zts_bsd_accept(arg1, arg2, arg3);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_accept(int jarg1, char* jarg2, int jarg3, void* jarg4)
{
    int arg1;
    char* arg2 = (char*)0;
    int arg3;
    unsigned short* arg4 = (unsigned short*)0;
    arg1 = (int)jarg1;
    arg2 = (char*)jarg2;
    arg3 = (int)jarg3;
    arg4 = (unsigned short*)jarg4;
    return zts_accept(arg1, arg2, arg3, arg4);
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_tcp_client(char* jarg1, int jarg2)
{
    int jresult;
    char* arg1 = (char*)0;
    int arg2;
    int result;
    arg1 = (char*)jarg1;
    arg2 = (int)jarg2;
    result = (int)zts_tcp_client((char const*)arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_tcp_server(char* jarg1, int jarg2, char* jarg3, int jarg4, void* jarg5)
{
    char* arg1 = (char*)0;
    int arg2;
    char* arg3 = (char*)0;
    int arg4;
    unsigned short* arg5 = (unsigned short*)0;
    arg1 = (char*)jarg1;
    arg2 = (int)jarg2;
    arg3 = (char*)jarg3;
    arg4 = (int)jarg4;
    arg5 = (unsigned short*)jarg5;
    return zts_tcp_server((char const*)arg1, arg2, arg3, arg4, arg5);
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_udp_server(char* jarg1, int jarg2)
{
    int jresult;
    char* arg1 = (char*)0;
    int arg2;
    int result;
    arg1 = (char*)jarg1;
    arg2 = (int)jarg2;
    result = (int)zts_udp_server((char const*)arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_udp_client(char* jarg1)
{
    int jresult;
    char* arg1 = (char*)0;
    int result;
    arg1 = (char*)jarg1;
    result = (int)zts_udp_client((char const*)arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_setsockopt(int jarg1, int jarg2, int jarg3, void* jarg4, unsigned short jarg5)
{
    int jresult;
    int arg1;
    int arg2;
    int arg3;
    void* arg4 = (void*)0;
    zts_socklen_t arg5;
    int result;
    arg1 = (int)jarg1;
    arg2 = (int)jarg2;
    arg3 = (int)jarg3;
    arg4 = (void*)jarg4;
    arg5 = (zts_socklen_t)jarg5;
    result = (int)zts_bsd_setsockopt(arg1, arg2, arg3, (void const*)arg4, arg5);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_getsockopt(int jarg1, int jarg2, int jarg3, void* jarg4, void* jarg5)
{
    int jresult;
    int arg1;
    int arg2;
    int arg3;
    void* arg4 = (void*)0;
    zts_socklen_t* arg5 = (zts_socklen_t*)0;
    int result;
    arg1 = (int)jarg1;
    arg2 = (int)jarg2;
    arg3 = (int)jarg3;
    arg4 = (void*)jarg4;
    arg5 = (zts_socklen_t*)jarg5;
    result = (int)zts_bsd_getsockopt(arg1, arg2, arg3, arg4, arg5);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_getsockname(int jarg1, zts_sockaddr* jarg2, void* jarg3)
{
    int jresult;
    int arg1;
    zts_sockaddr* arg2 = (zts_sockaddr*)0;
    zts_socklen_t* arg3 = (zts_socklen_t*)0;
    int result;
    arg1 = (int)jarg1;
    arg2 = (zts_sockaddr*)jarg2;
    arg3 = (zts_socklen_t*)jarg3;
    result = (int)zts_bsd_getsockname(arg1, arg2, arg3);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_getpeername(int jarg1, zts_sockaddr* jarg2, void* jarg3)
{
    int jresult;
    int arg1;
    zts_sockaddr* arg2 = (zts_sockaddr*)0;
    zts_socklen_t* arg3 = (zts_socklen_t*)0;
    int result;
    arg1 = (int)jarg1;
    arg2 = (zts_sockaddr*)jarg2;
    arg3 = (zts_socklen_t*)jarg3;
    result = (int)zts_bsd_getpeername(arg1, arg2, arg3);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_close(int jarg1)
{
    int jresult;
    int arg1;
    int result;
    arg1 = (int)jarg1;
    result = (int)zts_bsd_close(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_select(int jarg1, void* jarg2, void* jarg3, void* jarg4, void* jarg5)
{
    int jresult;
    int arg1;
    zts_fd_set* arg2 = (zts_fd_set*)0;
    zts_fd_set* arg3 = (zts_fd_set*)0;
    zts_fd_set* arg4 = (zts_fd_set*)0;
    zts_timeval* arg5 = (zts_timeval*)0;
    int result;
    arg1 = (int)jarg1;
    arg2 = (zts_fd_set*)jarg2;
    arg3 = (zts_fd_set*)jarg3;
    arg4 = (zts_fd_set*)jarg4;
    arg5 = (zts_timeval*)jarg5;
    result = (int)zts_bsd_select(arg1, arg2, arg3, arg4, arg5);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_fcntl(int jarg1, int jarg2, int jarg3)
{
    int jresult;
    int arg1;
    int arg2;
    int arg3;
    int result;
    arg1 = (int)jarg1;
    arg2 = (int)jarg2;
    arg3 = (int)jarg3;
    result = (int)zts_bsd_fcntl(arg1, arg2, arg3);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_poll(void* jarg1, unsigned int jarg2, int jarg3)
{
    int jresult;
    zts_pollfd* arg1 = (zts_pollfd*)0;
    zts_nfds_t arg2;
    int arg3;
    int result;
    arg1 = (zts_pollfd*)jarg1;
    arg2 = (zts_nfds_t)jarg2;
    arg3 = (int)jarg3;
    result = (int)zts_bsd_poll(arg1, arg2, arg3);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_ioctl(int jarg1, unsigned long jarg2, void* jarg3)
{
    int jresult;
    int arg1;
    unsigned long arg2;
    void* arg3 = (void*)0;
    int result;
    arg1 = (int)jarg1;
    arg2 = (unsigned long)jarg2;
    arg3 = (void*)jarg3;
    result = (int)zts_bsd_ioctl(arg1, arg2, arg3);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_send(int jarg1, void* jarg2, unsigned long jarg3, int jarg4)
{
    int arg1;
    void* arg2 = (void*)0;
    size_t arg3;
    int arg4;
    arg1 = (int)jarg1;
    arg2 = (void*)jarg2;
    arg3 = (size_t)jarg3;
    arg4 = (int)jarg4;
    return zts_bsd_send(arg1, (void const*)arg2, arg3, arg4);
}

SWIGEXPORT int SWIGSTDCALL
CSharp_zts_bsd_sendto(int jarg1, void* jarg2, unsigned long jarg3, int jarg4, zts_sockaddr* jarg5, unsigned short jarg6)
{
    int arg1;
    void* arg2 = (void*)0;
    size_t arg3;
    int arg4;
    zts_sockaddr* arg5 = (zts_sockaddr*)0;
    zts_socklen_t arg6;
    arg1 = (int)jarg1;
    arg2 = (void*)jarg2;
    arg3 = (size_t)jarg3;
    arg4 = (int)jarg4;
    arg5 = (zts_sockaddr*)jarg5;
    arg6 = (zts_socklen_t)jarg6;
    return zts_bsd_sendto(arg1, (void const*)arg2, arg3, arg4, (zts_sockaddr const*)arg5, arg6);
}

SWIGEXPORT void* SWIGSTDCALL CSharp_new_zts_iovec()
{
    void* jresult;
    zts_iovec* result = 0;

    result = (zts_iovec*)new zts_iovec();
    jresult = (void*)result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_sendmsg(int jarg1, void* jarg2, int jarg3)
{
    int arg1;
    zts_msghdr* arg2 = (zts_msghdr*)0;
    int arg3;
    arg1 = (int)jarg1;
    arg2 = (zts_msghdr*)jarg2;
    arg3 = (int)jarg3;
    return zts_bsd_sendmsg(arg1, (zts_msghdr const*)arg2, arg3);
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_recv(int jarg1, void* jarg2, unsigned long jarg3, int jarg4)
{
    int arg1;
    void* arg2 = (void*)0;
    size_t arg3;
    int arg4;
    arg1 = (int)jarg1;
    arg2 = (void*)jarg2;
    arg3 = (size_t)jarg3;
    arg4 = (int)jarg4;
    return zts_bsd_recv(arg1, arg2, arg3, arg4);
}

SWIGEXPORT int SWIGSTDCALL
CSharp_zts_bsd_recvfrom(int jarg1, void* jarg2, unsigned long jarg3, int jarg4, zts_sockaddr* jarg5, void* jarg6)
{
    int arg1;
    void* arg2 = (void*)0;
    size_t arg3;
    int arg4;
    zts_sockaddr* arg5 = (zts_sockaddr*)0;
    zts_socklen_t* arg6 = (zts_socklen_t*)0;
    arg1 = (int)jarg1;
    arg2 = (void*)jarg2;
    arg3 = (size_t)jarg3;
    arg4 = (int)jarg4;
    arg5 = (zts_sockaddr*)jarg5;
    arg6 = (zts_socklen_t*)jarg6;
    return zts_bsd_recvfrom(arg1, arg2, arg3, arg4, arg5, arg6);
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_recvmsg(int jarg1, void* jarg2, int jarg3)
{
    int arg1;
    zts_msghdr* arg2 = (zts_msghdr*)0;
    int arg3;
    arg1 = (int)jarg1;
    arg2 = (zts_msghdr*)jarg2;
    arg3 = (int)jarg3;
    return zts_bsd_recvmsg(arg1, arg2, arg3);
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_read(int jarg1, void* jarg2, unsigned long jarg3)
{
    int arg1;
    void* arg2 = (void*)0;
    size_t arg3;
    arg1 = (int)jarg1;
    arg2 = (void*)jarg2;
    arg3 = (size_t)jarg3;
    return zts_bsd_read(arg1, arg2, arg3);
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_readv(int jarg1, void* jarg2, int jarg3)
{
    int arg1;
    zts_iovec* arg2 = (zts_iovec*)0;
    int arg3;
    arg1 = (int)jarg1;
    arg2 = (zts_iovec*)jarg2;
    arg3 = (int)jarg3;
    return zts_bsd_readv(arg1, (zts_iovec const*)arg2, arg3);
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_write(int jarg1, void* jarg2, unsigned long jarg3)
{
    int arg1;
    void* arg2 = (void*)0;
    size_t arg3;
    arg1 = (int)jarg1;
    arg2 = (void*)jarg2;
    arg3 = (size_t)jarg3;
    return zts_bsd_write(arg1, (void const*)arg2, arg3);
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_writev(int jarg1, void* jarg2, int jarg3)
{
    int arg1;
    zts_iovec* arg2 = (zts_iovec*)0;
    int arg3;
    arg1 = (int)jarg1;
    arg2 = (zts_iovec*)jarg2;
    arg3 = (int)jarg3;
    return zts_bsd_writev(arg1, (zts_iovec const*)arg2, arg3);
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_bsd_shutdown(int jarg1, int jarg2)
{
    int jresult;
    int arg1;
    int arg2;
    int result;
    arg1 = (int)jarg1;
    arg2 = (int)jarg2;
    result = (int)zts_bsd_shutdown(arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT size_t SWIGSTDCALL CSharp_zts_get_data_available(int fd)
{
    return zts_get_data_available(fd);
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_set_no_delay(int jarg1, int jarg2)
{
    int jresult;
    int arg1;
    int arg2;
    int result;
    arg1 = (int)jarg1;
    arg2 = (int)jarg2;
    result = (int)zts_set_no_delay(arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_get_no_delay(int jarg1)
{
    int jresult;
    int arg1;
    int result;
    arg1 = (int)jarg1;
    result = (int)zts_get_no_delay(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_set_linger(int jarg1, int jarg2, int jarg3)
{
    int jresult;
    int arg1;
    int arg2;
    int arg3;
    int result;
    arg1 = (int)jarg1;
    arg2 = (int)jarg2;
    arg3 = (int)jarg3;
    result = (int)zts_set_linger(arg1, arg2, arg3);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_get_linger_enabled(int jarg1)
{
    int jresult;
    int arg1;
    int result;
    arg1 = (int)jarg1;
    result = (int)zts_get_linger_enabled(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_get_linger_value(int jarg1)
{
    int jresult;
    int arg1;
    int result;
    arg1 = (int)jarg1;
    result = (int)zts_get_linger_value(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_set_reuse_addr(int jarg1, int jarg2)
{
    int jresult;
    int arg1;
    int arg2;
    int result;
    arg1 = (int)jarg1;
    arg2 = (int)jarg2;
    result = (int)zts_set_reuse_addr(arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_get_reuse_addr(int jarg1)
{
    int jresult;
    int arg1;
    int result;
    arg1 = (int)jarg1;
    result = (int)zts_get_reuse_addr(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_set_recv_timeout(int jarg1, int jarg2, int jarg3)
{
    int jresult;
    int arg1;
    int arg2;
    int arg3;
    int result;
    arg1 = (int)jarg1;
    arg2 = (int)jarg2;
    arg3 = (int)jarg3;
    result = (int)zts_set_recv_timeout(arg1, arg2, arg3);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_get_recv_timeout(int jarg1)
{
    int jresult;
    int arg1;
    int result;
    arg1 = (int)jarg1;
    result = (int)zts_get_recv_timeout(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_set_send_timeout(int jarg1, int jarg2, int jarg3)
{
    int jresult;
    int arg1;
    int arg2;
    int arg3;
    int result;
    arg1 = (int)jarg1;
    arg2 = (int)jarg2;
    arg3 = (int)jarg3;
    result = (int)zts_set_send_timeout(arg1, arg2, arg3);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_get_send_timeout(int jarg1)
{
    int jresult;
    int arg1;
    int result;
    arg1 = (int)jarg1;
    result = (int)zts_get_send_timeout(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_set_send_buf_size(int jarg1, int jarg2)
{
    int jresult;
    int arg1;
    int arg2;
    int result;
    arg1 = (int)jarg1;
    arg2 = (int)jarg2;
    result = (int)zts_set_send_buf_size(arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_get_send_buf_size(int jarg1)
{
    int jresult;
    int arg1;
    int result;
    arg1 = (int)jarg1;
    result = (int)zts_get_send_buf_size(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_set_recv_buf_size(int jarg1, int jarg2)
{
    int jresult;
    int arg1;
    int arg2;
    int result;
    arg1 = (int)jarg1;
    arg2 = (int)jarg2;
    result = (int)zts_set_recv_buf_size(arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_get_recv_buf_size(int jarg1)
{
    int jresult;
    int arg1;
    int result;
    arg1 = (int)jarg1;
    result = (int)zts_get_recv_buf_size(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_set_ttl(int jarg1, int jarg2)
{
    int jresult;
    int arg1;
    int arg2;
    int result;
    arg1 = (int)jarg1;
    arg2 = (int)jarg2;
    result = (int)zts_set_ttl(arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_get_ttl(int jarg1)
{
    int jresult;
    int arg1;
    int result;
    arg1 = (int)jarg1;
    result = (int)zts_get_ttl(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_set_blocking(int jarg1, int jarg2)
{
    int jresult;
    int arg1;
    int arg2;
    int result;
    arg1 = (int)jarg1;
    arg2 = (int)jarg2;
    result = (int)zts_set_blocking(arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_get_blocking(int jarg1)
{
    int jresult;
    int arg1;
    int result;
    arg1 = (int)jarg1;
    result = (int)zts_get_blocking(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_set_keepalive(int jarg1, int jarg2)
{
    int jresult;
    int arg1;
    int arg2;
    int result;
    arg1 = (int)jarg1;
    arg2 = (int)jarg2;
    result = (int)zts_set_keepalive(arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_get_keepalive(int jarg1)
{
    int jresult;
    int arg1;
    int result;
    arg1 = (int)jarg1;
    result = (int)zts_get_keepalive(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT void* SWIGSTDCALL CSharp_zts_bsd_gethostbyname(char* jarg1)
{
    void* jresult;
    char* arg1 = (char*)0;
    zts_hostent* result = 0;
    arg1 = (char*)jarg1;
    result = (zts_hostent*)zts_bsd_gethostbyname((char const*)arg1);
    jresult = (void*)result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_dns_set_server(unsigned char jarg1, void* jarg2)
{
    int jresult;
    uint8_t arg1;
    zts_ip_addr* arg2 = (zts_ip_addr*)0;
    int result;
    arg1 = (uint8_t)jarg1;
    arg2 = (zts_ip_addr*)jarg2;
    result = (int)zts_dns_set_server(arg1, (zts_ip_addr const*)arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT void* SWIGSTDCALL CSharp_zts_dns_get_server(unsigned char jarg1)
{
    void* jresult;
    uint8_t arg1;
    zts_ip_addr* result = 0;
    arg1 = (uint8_t)jarg1;
    result = (zts_ip_addr*)zts_dns_get_server(arg1);
    jresult = (void*)result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_core_lock_obtain()
{
    int jresult;
    int result;

    result = (int)zts_core_lock_obtain();
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_core_lock_release()
{
    int jresult;
    int result;

    result = (int)zts_core_lock_release();
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_core_query_addr_count(unsigned long long jarg1)
{
    int jresult;
    uint64_t arg1;
    int result;
    arg1 = (uint64_t)jarg1;
    result = (int)zts_core_query_addr_count(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_core_query_addr(unsigned long long jarg1, int jarg2, char* jarg3, int jarg4)
{
    int jresult;
    uint64_t arg1;
    int arg2;
    char* arg3 = (char*)0;
    int arg4;
    int result;
    arg1 = (uint64_t)jarg1;
    arg2 = (int)jarg2;
    arg3 = (char*)jarg3;
    arg4 = (int)jarg4;
    result = (int)zts_core_query_addr(arg1, arg2, arg3, arg4);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_core_query_route_count(unsigned long long jarg1)
{
    int jresult;
    uint64_t arg1;
    int result;
    arg1 = (uint64_t)jarg1;
    result = (int)zts_core_query_route_count(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_core_query_route(
    unsigned long long jarg1,
    int jarg2,
    char* jarg3,
    char* jarg4,
    int jarg5,
    void* jarg6,
    void* jarg7)
{
    int jresult;
    uint64_t arg1;
    int arg2;
    char* arg3 = (char*)0;
    char* arg4 = (char*)0;
    int arg5;
    uint16_t* arg6 = (uint16_t*)0;
    uint16_t* arg7 = (uint16_t*)0;
    int result;
    arg1 = (uint64_t)jarg1;
    arg2 = (int)jarg2;
    arg3 = (char*)jarg3;
    arg4 = (char*)jarg4;
    arg5 = (int)jarg5;
    arg6 = (uint16_t*)jarg6;
    arg7 = (uint16_t*)jarg7;
    result = (int)zts_core_query_route(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_core_query_path_count(unsigned long long jarg1)
{
    int jresult;
    uint64_t arg1;
    int result;
    arg1 = (uint64_t)jarg1;
    result = (int)zts_core_query_path_count(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_core_query_path(unsigned long long jarg1, int jarg2, char* jarg3, int jarg4)
{
    int jresult;
    uint64_t arg1;
    int arg2;
    char* arg3 = (char*)0;
    int arg4;
    int result;
    arg1 = (uint64_t)jarg1;
    arg2 = (int)jarg2;
    arg3 = (char*)jarg3;
    arg4 = (int)jarg4;
    result = (int)zts_core_query_path(arg1, arg2, arg3, arg4);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_core_query_mc_count(unsigned long long jarg1)
{
    int jresult;
    uint64_t arg1;
    int result;
    arg1 = (uint64_t)jarg1;
    result = (int)zts_core_query_mc_count(arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_core_query_mc(unsigned long long jarg1, int jarg2, void* jarg3, void* jarg4)
{
    int jresult;
    uint64_t arg1;
    int arg2;
    uint64_t* arg3 = (uint64_t*)0;
    uint32_t* arg4 = (uint32_t*)0;
    int result;
    arg1 = (uint64_t)jarg1;
    arg2 = (int)jarg2;
    arg3 = (uint64_t*)jarg3;
    arg4 = (uint32_t*)jarg4;
    result = (int)zts_core_query_mc(arg1, arg2, arg3, arg4);
    jresult = result;
    return jresult;
}

SWIGEXPORT void SWIGSTDCALL CSharp_zts_util_delay(long jarg1)
{
    long arg1;
    arg1 = (long)jarg1;
    zts_util_delay(arg1);
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_util_get_ip_family(char* jarg1)
{
    int jresult;
    char* arg1 = (char*)0;
    int result;
    arg1 = (char*)jarg1;
    result = (int)zts_util_get_ip_family((char const*)arg1);
    jresult = result;
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_util_ipstr_to_saddr(char* jarg1, int jarg2, zts_sockaddr* jarg3, void* jarg4)
{
    int jresult;
    char* arg1 = (char*)0;
    int arg2;
    zts_sockaddr* arg3 = (zts_sockaddr*)0;
    zts_socklen_t* arg4 = (zts_socklen_t*)0;
    int result;
    arg1 = (char*)jarg1;
    arg2 = (int)jarg2;
    arg3 = (zts_sockaddr*)jarg3;
    arg4 = (zts_socklen_t*)jarg4;
    result = (int)zts_util_ipstr_to_saddr((char const*)arg1, arg2, arg3, arg4);
    jresult = result;
    return jresult;
}

SWIGEXPORT char* SWIGSTDCALL CSharp_zts_ipaddr_ntoa(void* jarg1)
{
    char* jresult;
    zts_ip_addr* arg1 = (zts_ip_addr*)0;
    char* result = 0;
    arg1 = (zts_ip_addr*)jarg1;
    result = (char*)zts_ipaddr_ntoa((zts_ip_addr const*)arg1);
    jresult = SWIG_csharp_string_callback((const char*)result);
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_ipaddr_aton(char* jarg1, void* jarg2)
{
    int jresult;
    char* arg1 = (char*)0;
    zts_ip_addr* arg2 = (zts_ip_addr*)0;
    int result;
    arg1 = (char*)jarg1;
    arg2 = (zts_ip_addr*)jarg2;
    result = (int)zts_ipaddr_aton((char const*)arg1, arg2);
    jresult = result;
    return jresult;
}

SWIGEXPORT char* SWIGSTDCALL CSharp_zts_inet_ntop(int jarg1, void* jarg2, char* jarg3, unsigned short jarg4)
{
    char* jresult;
    int arg1;
    void* arg2 = (void*)0;
    char* arg3 = (char*)0;
    zts_socklen_t arg4;
    char* result = 0;
    arg1 = (int)jarg1;
    arg2 = (void*)jarg2;
    arg3 = (char*)jarg3;
    arg4 = (zts_socklen_t)jarg4;
    result = (char*)zts_inet_ntop(arg1, (void const*)arg2, arg3, arg4);
    jresult = SWIG_csharp_string_callback((const char*)result);
    return jresult;
}

SWIGEXPORT int SWIGSTDCALL CSharp_zts_inet_pton(int jarg1, char* jarg2, void* jarg3)
{
    int jresult;
    int arg1;
    char* arg2 = (char*)0;
    void* arg3 = (void*)0;
    int result;
    arg1 = (int)jarg1;
    arg2 = (char*)jarg2;
    arg3 = (void*)jarg3;
    result = (int)zts_inet_pton(arg1, (char const*)arg2, arg3);
    jresult = result;
    return jresult;
}

#ifdef __cplusplus
}
#endif
