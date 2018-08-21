#
# ZeroTier SDK - Network Virtualization Everywhere
# Copyright (C) 2011-2017  ZeroTier, Inc.  https://www.zerotier.com/
# 
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
# 
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
# 
#  --
# 
#  You can be released from the requirements of the license by purchasing
#  a commercial license. Buying such a license is mandatory as soon as you
#  develop commercial closed-source software that incorporates or links
#  directly against ZeroTier software without disclosing the source code
#  of your own application.
# 

cmake_minimum_required (VERSION 3.0)
project (zt)

set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_SUPPRESS_REGENERATION true)
set (PROJ_DIR ${PROJECT_SOURCE_DIR})
set (CMAKE_BINARY_DIR ${PROJECT_SOURCE_DIR}/bin)
set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set (INCLUDE_PATH ${PROJ_DIR}/include)

# -----------------------------------------------------------------------------
# |                                 BUILD CONFIG                              |
# -----------------------------------------------------------------------------

# Default build type: Release

# Release - Optimization and no debug info
# Debug - No optimization, debug info
# RelWithDebInfo - Release optimizations and debug info
# MinSizeRel - Similar to Release but with optimizations to minimize size

if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Release)
endif ()

set (SILENCE "-Wno-unused-parameter -Wno-unused-variable -Wno-missing-field-initializers")

# -----------------------------------------------------------------------------
# |                     PLATFORM/FEATURE AND IDE DETECTION                    |
# -----------------------------------------------------------------------------

if (JNI EQUAL 1)
	set (ADD_JNI TRUE)
endif ()
if (${CMAKE_SYSTEM_NAME} MATCHES "Android")
	set (BUILDING_ANDROID TRUE)
endif ()
if (WIN32)
	set (BUILDING_WIN32 TRUE)
endif ()
if ("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
	set (BUILDING_WIN64 TRUE)
endif ()
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set (BUILDING_DARWIN TRUE)
endif ()
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set (BUILDING_LINUX TRUE)
endif ()
if (${CMAKE_GENERATOR} STREQUAL "Xcode")
	set (IN_XCODE TRUE)
	set(XCODE_EMIT_EFFECTIVE_PLATFORM_NAME ON)
	#set_target_properties (${STATIC_LIB_NAME} PROPERTIES XCODE_ATTRIBUTE_MY_BUILD_ONLY_ACTIVE_ARCH YES)
	#set_target_properties (${STATIC_LIB_NAME} PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "${MY_BUILD_ONLY_ACTIVE_ARCH})
endif ()
if (NOT BUILDING_ANDROID AND NOT IN_XCODE AND NOT BUILD_TESTS EQUAL 0)
	set(SHOULD_BUILD_TESTS TRUE)
endif ()

if (BUILDING_WIN32 OR BUILDING_WIN64 OR MSVC)
	set (BUILDING_WIN TRUE)
endif ()

# -----------------------------------------------------------------------------
# |                                   LWIP PORT                               |
# -----------------------------------------------------------------------------

if (UNIX)
	set (LWIP_PORT_DIR ${PROJ_DIR}/ext/lwip-contrib/ports/unix/port)
endif ()

if (BUILDING_WIN)
	set (LWIP_PORT_DIR ${PROJ_DIR}/ext/lwip-contrib/ports/win32)
endif ()

# -----------------------------------------------------------------------------
# |                                LIBRARY NAMES                              |
# -----------------------------------------------------------------------------

set (STATIC_LIB_NAME ${PROJECT_NAME}-static)
set (STATIC_LIB_OUTPUT_NAME ${PROJECT_NAME})
set (DYNAMIC_LIB_NAME ${PROJECT_NAME}-shared)
set (DYNAMIC_LIB_OUTPUT_NAME ${PROJECT_NAME})

if (BUILDING_DARWIN)
	set (XCODE_FRAMEWORK_NAME ${PROJECT_NAME})
	set (STATIC_LIB_NAME ${PROJECT_NAME}-static)
	set (STATIC_LIB_OUTPUT_NAME ${PROJECT_NAME})
	set (DYNAMIC_LIB_NAME ${PROJECT_NAME}-shared)
	set (DYNAMIC_LIB_OUTPUT_NAME ${PROJECT_NAME})
endif ()

if (BUILDING_ANDROID)
	set (STATIC_LIB_NAME ${PROJECT_NAME}-static)
	set (STATIC_LIB_OUTPUT_NAME ${PROJECT_NAME})
	set (DYNAMIC_LIB_NAME ${PROJECT_NAME}-shared)
	set (DYNAMIC_LIB_OUTPUT_NAME ${PROJECT_NAME})
endif ()

if (BUILDING_WIN)
	set (STATIC_LIB_NAME ${PROJECT_NAME}-static)
	set (STATIC_LIB_OUTPUT_NAME ${PROJECT_NAME})
	set (DYNAMIC_LIB_NAME ${PROJECT_NAME}-shared)
	set (DYNAMIC_LIB_OUTPUT_NAME ${PROJECT_NAME})
endif ()

# -----------------------------------------------------------------------------
# |                                    FLAGS                                  |
# -----------------------------------------------------------------------------

set (LIBZT_FLAGS "-DZT_SDK=1")
set (LIBZT_FLAGS_DEBUG "-DZT_SDK=1 -DLIBZT_TRACE=1 -DLWIP_DEBUG=1 -DLIBZT_DEBUG=1 -DNS_TRACE=1 -DNS_DEBUG=1")

if (BUILDING_WIN)
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc -DNOMINMAX")
else ()
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector")
	set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
	set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fstack-protector")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LIBZT_FLAGS} ${SILENCE} -O3 -Wall -Wextra -std=c++11")
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${LIBZT_FLAGS_DEBUG} ${SILENCE} -std=c++11")
	set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${LIBZT_FLAGS} ${SILENCE} -O3 -std=c++11")
endif ()

# -----------------------------------------------------------------------------
# |                            SOURCE FILE GLOBS                              |
# -----------------------------------------------------------------------------

set (LWIP_SRC_DIR ${PROJ_DIR}/ext/lwip/src)
set (ZTO_SRC_DIR ${PROJ_DIR}/ext/ZeroTierOne)
set (LIBZT_SRC_DIR ${PROJ_DIR}/src)

file (GLOB lwipSrcGlob
	${LWIP_SRC_DIR}/netif/*.c
	${LWIP_SRC_DIR}/api/*.c
	${LWIP_PORT_DIR}/sys_arch.c
	${LWIP_SRC_DIR}/core/*.c
	${LWIP_SRC_DIR}/core/ipv4/*.c
	${LWIP_SRC_DIR}/core/ipv6/*.c)
list(REMOVE_ITEM lwipSrcGlob ${LWIP_SRC_DIR}/netif/slipif.c)

file (GLOB ztoSrcGlob
	${ZTO_SRC_DIR}/node/*.cpp
	${ZTO_SRC_DIR}/service/*.cpp
	${ZTO_SRC_DIR}/osdep/OSUtils.cpp
	${ZTO_SRC_DIR}/controller/*.cpp
	${ZTO_SRC_DIR}/osdep/ManagedRoute.cpp)

file (GLOB libztSrcGlob ${LIBZT_SRC_DIR}/*.cpp)

file (GLOB ExampleAppSrcGlob
	${PROJ_DIR}/examples/cpp/*.cpp
	${PROJ_DIR}/examples/cpp/ipv4simple/*.cpp
	${PROJ_DIR}/examples/cpp/ipv6simple/*.cpp
	${PROJ_DIR}/examples/cpp/ipv6adhoc/*.cpp
	${PROJ_DIR}/examples/cpp/sharedlib/*.cpp
	${PROJ_DIR}/examples/ztproxy/*.cpp)

include_directories (${LWIP_SRC_DIR}/include)
include_directories (${LWIP_PORT_DIR}/include)
include_directories (${LIBZT_SRC_DIR})

include_directories (${ZTO_SRC_DIR}/include)
include_directories (${ZTO_SRC_DIR}/osdep)
include_directories (${ZTO_SRC_DIR}/node)
include_directories (${ZTO_SRC_DIR}/service)

include_directories (${INCLUDE_PATH})

# header globs for xcode frameworks
file (GLOB frameworkPrivateHeaderGlob
	${INCLUDE_PATH}/libzt.h
	${INCLUDE_PATH}/libztDefs.h
	${INCLUDE_PATH}/libztDebug.h)
file (GLOB frameworkPublicHeaderGlob ${INCLUDE_PATH}/Xcode-Bridging-Header.h)
file (GLOB frameworkHeaderGlob ${frameworkPublicHeaderGlob} ${frameworkPrivateHeaderGlob})

# -----------------------------------------------------------------------------
# |                         PLATFORM-SPECIFIC CONFIG                          |
# -----------------------------------------------------------------------------

# ANDROID-specific
if (BUILDING_ANDROID)
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -DSOCKLEN_T_DEFINED=1")
	set (ANDROID_NDK /Users/$ENV{USER}/Library/Android/sdk/ndk-bundle/sysroot/usr/include/arm-linux-androideabi)
	include_directories (${ANDROID_NDK})
endif ()

# WINDOWS-specific MSVC flags and libraries
if (BUILDING_WIN)
	# 32-bit
	if(NOT BUILDING_WIN64)
		set (WINLIBDIR, "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.16299.0/um/x86")
	endif ()
	# 64-bit
	if(BUILDING_WIN64)
		set (WINLIBDIR, "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.16299.0/um/x64")
	endif ()
	find_library (ws2_32_LIBRARY_PATH NAMES WS2_32 HINTS ${WINLIBDIR})
	find_library (shlwapi_LIBRARY_PATH NAMES ShLwApi HINTS ${WINLIBDIR})
	set (iphlpapi_LIBRARY_PATH "${WINLIBDIR}/iphlpapi.Lib")
	message (STATUS ${WINLIBDIR})
	message (STATUS "WS2_32=${ws2_32_LIBRARY_PATH}")
	message (STATUS "ShLwApi=${shlwapi_LIBRARY_PATH}")
	message (STATUS "liphlpapi=${iphlpapi_LIBRARY_PATH}")
	add_definitions (-DZT_SDK=1)
	add_definitions (-DADD_EXPORTS=1)
endif ()

# -----------------------------------------------------------------------------
# |                                   JNI                                     |
# -----------------------------------------------------------------------------

if (ADD_JNI OR BUILDING_ANDROID)
	MESSAGE (STATUS "Looking for JNI headers")
	find_package (JNI)
	add_definitions (-DSDK_JNI=1)
	if (JNI_FOUND)
	    message (STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
	    message (STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
	    list (GET JNI_INCLUDE_DIRS 0 JNI_INCLUDE_DIR)
	    message (STATUS "jni path=${JNI_INCLUDE_DIR}")
	    include_directories ("${JNI_INCLUDE_DIR}")
	else ()
	    message (STATUS "JNI not found")
	endif ()
endif ()

# -----------------------------------------------------------------------------
# |                        OBJECTS-PIC (INTERMEDIATE)                         |
# -----------------------------------------------------------------------------

add_library (lwip_pic ${lwipSrcGlob})
set_target_properties (lwip_pic PROPERTIES POSITION_INDEPENDENT_CODE ON)
add_library (zto_pic ${ztoSrcGlob})
set_target_properties (zto_pic PROPERTIES POSITION_INDEPENDENT_CODE ON)
add_library (http_pic "${ZTO_SRC_DIR}/ext/http-parser/http_parser.c")
set_target_properties (http_pic PROPERTIES POSITION_INDEPENDENT_CODE ON)

# -----------------------------------------------------------------------------
# |                     OBJECT LIBRARIES (INTERMEDIATE)                       |
# -----------------------------------------------------------------------------

# lwip_obj
add_library (lwip_obj OBJECT ${lwipSrcGlob})
# zto_obj
add_library (zto_obj OBJECT ${ztoSrcGlob})
if (BUILDING_WIN)
	target_link_libraries (zto_obj ws2_32)
	target_link_libraries (zto_obj ${shlwapi_LIBRARY_PATH})
	target_link_libraries (zto_obj ${iphlpapi_LIBRARY_PATH})
endif ()
# http_obj
add_library (http_obj OBJECT "${ZTO_SRC_DIR}/ext/http-parser/http_parser.c")

# -----------------------------------------------------------------------------
# |                     LIBZT BUILD TARGETS (FINAL PRODUCT)                   |
# -----------------------------------------------------------------------------

# static
add_library (${STATIC_LIB_NAME} STATIC
	$<TARGET_OBJECTS:lwip_obj>
	$<TARGET_OBJECTS:zto_obj>
	$<TARGET_OBJECTS:http_obj> ${libztSrcGlob})
set_target_properties (${STATIC_LIB_NAME} PROPERTIES OUTPUT_NAME zt)

# dynamic
add_library (${DYNAMIC_LIB_NAME} SHARED ${libztSrcGlob})
message (STATUS ${libztSrcGlob})
target_link_libraries (${DYNAMIC_LIB_NAME} lwip_pic zto_pic http_pic)
set_target_properties (${DYNAMIC_LIB_NAME} PROPERTIES OUTPUT_NAME zt)
set_target_properties (${DYNAMIC_LIB_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS true)

if (BUILDING_ANDROID)
	target_link_libraries (${DYNAMIC_LIB_NAME} android log)
endif ()

if (BUILDING_WIN)
	target_link_libraries (${STATIC_LIB_NAME} ws2_32)
	target_link_libraries (${STATIC_LIB_NAME} ${shlwapi_LIBRARY_PATH})
	target_link_libraries (${STATIC_LIB_NAME} ${iphlpapi_LIBRARY_PATH})
	target_link_libraries (${DYNAMIC_LIB_NAME} ws2_32)
	target_link_libraries (${DYNAMIC_LIB_NAME} ${shlwapi_LIBRARY_PATH})
	target_link_libraries (${DYNAMIC_LIB_NAME} ${iphlpapi_LIBRARY_PATH})
endif ()

if (BUILDING_LINUX OR BUILDING_DARWIN)
	target_link_libraries (${STATIC_LIB_NAME} pthread)
	target_link_libraries (${DYNAMIC_LIB_NAME} pthread)
endif ()

# xcode framework
if (IN_XCODE)
	add_library(${XCODE_FRAMEWORK_NAME} STATIC
		$<TARGET_OBJECTS:lwip_obj>
		$<TARGET_OBJECTS:zto_obj>
		$<TARGET_OBJECTS:http_obj>
		${libztSrcGlob}
		${frameworkHeaderGlob})

	set_target_properties(${XCODE_FRAMEWORK_NAME} PROPERTIES
		FRAMEWORK TRUE
		FRAMEWORK_VERSION A
		DEFINES_MODULE TRUE
		MACOSX_FRAMEWORK_IDENTIFIER com.cmake.${XCODE_FRAMEWORK_NAME}
		MODULE_MAP "/Users/joseph/op/zt/libzt/module.modulemap"
		PUBLIC_HEADER "${frameworkHeaderGlob}"
		XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
	)

		#PUBLIC_HEADER "${INCLUDE_PATH}/Xcode-Bridging-Header.h"
		#PRIVATE_HEADER "${frameworkPrivateHeaderGlob}"
endif ()

# -----------------------------------------------------------------------------
# |                       TEST APPLICATIONS AND EXAMPLES                      |
# -----------------------------------------------------------------------------

if (SHOULD_BUILD_TESTS)
	foreach (testsourcefile ${ExampleAppSrcGlob})
	    string (REPLACE ".cpp" "" testname ${testsourcefile})
	    get_filename_component (testname ${testname} NAME)
	    add_executable (${testname} ${testsourcefile})
	if (BUILDING_WIN)
	    target_link_libraries (${testname} ${STATIC_LIB_NAME})
	else ()
		target_link_libraries (${testname} ${STATIC_LIB_NAME} pthread dl)
	endif ()
	endforeach (testsourcefile ${ExampleAppSrcGlob})

if (NOT BUILDING_WIN) # only necessary for raw driver development
	# selftest
	add_executable (selftest ${PROJ_DIR}/test/selftest.cpp)
	target_compile_options (selftest PRIVATE -D__SELFTEST__)
	if (BUILDING_WIN)
		target_link_libraries (selftest ${STATIC_LIB_NAME} ${ws2_32_LIBRARY_PATH} ${shlwapi_LIBRARY_PATH} ${iphlpapi_LIBRARY_PATH})
	else ()
		target_link_libraries (selftest ${STATIC_LIB_NAME} pthread)
	endif ()
	# nativetest
	add_executable (nativetest ${PROJ_DIR}/test/selftest.cpp)
	target_compile_options (nativetest PRIVATE -D__NATIVETEST__)
endif ()
endif ()
