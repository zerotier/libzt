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
project (libzt)

# --- SETUP

set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_SUPPRESS_REGENERATION true)
set (PROJ_DIR ${PROJECT_SOURCE_DIR})
set (CMAKE_BINARY_DIR ${PROJECT_SOURCE_DIR}/bin)
set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

# --- BUILD CONFIG

# Default build type: Release
if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Release)
endif ()

set (SILENCE "-Wno-unused-parameter -Wno-unused-variable -Wno-missing-field-initializers")

# Release - Optimization and no debug info
# Debug - No optimization, debug info
# RelWithDebInfo - Release optimizations and debug info
# MinSizeRel - Similar to Release but with optimizations to minimize size

set (LIBZT_FLAGS "-DZT_SDK=1")
set (LIBZT_FLAGS_DEBUG "-DZT_SDK=1 -DLIBZT_TRACE=1 -DLWIP_DEBUG=1 -DLIBZT_DEBUG=1 -DNS_TRACE=1 -DNS_DEBUG=1")

if (WIN32)
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc -DNOMINMAX")
else ()
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector")
	set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
	set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fstack-protector")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LIBZT_FLAGS} ${SILENCE} -O3 -Wall -Wextra -std=c++11")
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${LIBZT_FLAGS_DEBUG} ${SILENCE} -std=c++11")
	set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${LIBZT_FLAGS} ${SILENCE} -O3 -std=c++11")
	set (LWIP_PORT_DIR ${PROJ_DIR}/ext/lwip-contrib/ports/unix/port)
endif ()

# --- PLATFORM-SPECIFIC CONFIG

# ANDROID-specific
if (${CMAKE_SYSTEM_NAME} MATCHES "Android")
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -DSOCKLEN_T_DEFINED=1")
	include_directories (/Users/joseph/Library/Android/sdk/ndk-bundle/sysroot/usr/include/arm-linux-androideabi)
endif ()

# WINDOWS-specific MSVC flags and libraries
if (WIN32)
	set (LWIP_PORT_DIR ${PROJ_DIR}/ext/lwip-contrib/ports/win32)
	# 32-bit
	if(NOT "${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
		set (WINLIBDIR, "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.16299.0/um/x86")
	endif ()
	# 64-bit
	if("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
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

# --- JNI

if (JNI EQUAL 1 OR ${CMAKE_SYSTEM_NAME} MATCHES "Android")
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

# --- BUILD TARGETS (FINAL PRODUCT)

# libzt
set (LIBZT_SRC_DIR ${PROJ_DIR}/src)
include_directories ("${LIBZT_SRC_DIR}")
include_directories ("${PROJ_DIR}/include")
file (GLOB libzt_src_glob ${LIBZT_SRC_DIR}/*.cpp)

# (static)
add_library (zt-static STATIC
	$<TARGET_OBJECTS:lwip_obj>
	$<TARGET_OBJECTS:zto_obj>
	$<TARGET_OBJECTS:http_obj> ${libzt_src_glob})
set_target_properties (zt-static PROPERTIES OUTPUT_NAME zt-static)

# (shared)
add_library (zt-shared SHARED ${libzt_src_glob})
message (STATUS ${libzt_src_glob})
target_link_libraries (zt-shared lwip_pic zto_pic http_pic)
set_target_properties (zt-shared PROPERTIES OUTPUT_NAME zt-shared)
set_target_properties (zt-shared PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS true)

if (${CMAKE_SYSTEM_NAME} MATCHES "Android")
	target_link_libraries (zt-shared android log)
endif ()

if (WIN32)
	target_link_libraries (zt-static ws2_32)
	target_link_libraries (zt-static ${shlwapi_LIBRARY_PATH})
	target_link_libraries (zt-static ${iphlpapi_LIBRARY_PATH})
	target_link_libraries (zt-shared ws2_32)
	target_link_libraries (zt-shared ${shlwapi_LIBRARY_PATH})
	target_link_libraries (zt-shared ${iphlpapi_LIBRARY_PATH})
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	target_link_libraries (zt-static pthread)
	target_link_libraries (zt-shared pthread)
endif ()

# --- OBJECT LIBRARIES (INTERMEDIATE)

# lwip_obj
set (LWIP_SRC_DIR ${PROJ_DIR}/ext/lwip/src)
include_directories (${LWIP_SRC_DIR}/include)
include_directories (${LWIP_PORT_DIR}/include)
file (GLOB lwip_src_glob 
	${LWIP_SRC_DIR}/netif/*.c 
	${LWIP_SRC_DIR}/api/*.c 
	${LWIP_PORT_DIR}/sys_arch.c
	${LWIP_SRC_DIR}/core/*.c 
	${LWIP_SRC_DIR}/core/ipv4/*.c 
	${LWIP_SRC_DIR}/core/ipv6/*.c)
list(REMOVE_ITEM lwip_src_glob ${LWIP_SRC_DIR}/netif/slipif.c)
add_library (lwip_obj OBJECT ${lwip_src_glob})
add_library (lwip_pic ${lwip_src_glob})
set_target_properties (lwip_pic PROPERTIES POSITION_INDEPENDENT_CODE ON)

# zto_obj
set (ZTO_SRC_DIR ${PROJ_DIR}/ext/ZeroTierOne)
include_directories (${ZTO_SRC_DIR}/include)
include_directories (${ZTO_SRC_DIR}/osdep)
include_directories (${ZTO_SRC_DIR}/node)
include_directories (${ZTO_SRC_DIR}/service)
file (GLOB zto_src_glob 
	${ZTO_SRC_DIR}/node/*.cpp
	${ZTO_SRC_DIR}/service/*.cpp
	${ZTO_SRC_DIR}/osdep/OSUtils.cpp
	${ZTO_SRC_DIR}/controller/*.cpp
	${ZTO_SRC_DIR}/osdep/ManagedRoute.cpp)
add_library (zto_obj OBJECT ${zto_src_glob})
if (WIN32)
	target_link_libraries (zto_obj ws2_32)
	target_link_libraries (zto_obj ${shlwapi_LIBRARY_PATH})
	target_link_libraries (zto_obj ${iphlpapi_LIBRARY_PATH})
endif ()
add_library (zto_pic ${zto_src_glob})
set_target_properties (zto_pic PROPERTIES POSITION_INDEPENDENT_CODE ON)

# http_obj
# HTTP Control Plane for ZT Service
add_library (http_obj OBJECT "${ZTO_SRC_DIR}/ext/http-parser/http_parser.c")
add_library (http_pic "${ZTO_SRC_DIR}/ext/http-parser/http_parser.c")
set_target_properties (http_pic PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --- TEST APPLICATIONS AND EXAMPLES

if ((NOT ${CMAKE_SYSTEM_NAME} MATCHES "Android") AND
	(NOT ${CMAKE_GENERATOR} STREQUAL "Xcode"))
	file (GLOB APP_SOURCES 
		${PROJ_DIR}/examples/cpp/ipv4simple/*.cpp 
		${PROJ_DIR}/examples/cpp/ipv6simple/*.cpp 
		${PROJ_DIR}/examples/cpp/ipv6adhoc/*.cpp 
		${PROJ_DIR}/examples/cpp/sharedlib/*.cpp 
		${PROJ_DIR}/examples/ztproxy/*.cpp
	)
	foreach (testsourcefile ${APP_SOURCES})
	    string (REPLACE ".cpp" "" testname ${testsourcefile})
	    get_filename_component (testname ${testname} NAME)
	    add_executable (${testname} ${testsourcefile})
	if (WIN32)
	    target_link_libraries (${testname} zt-static)
	else ()
		target_link_libraries (${testname} zt-static pthread dl)
	endif ()
	endforeach (testsourcefile ${APP_SOURCES})

if (NOT WIN32) # only necessary for raw driver development
	# selftest
	add_executable (selftest ${PROJ_DIR}/test/selftest.cpp)
	target_compile_options (selftest PRIVATE -D__SELFTEST__)
	if (WIN32)
		target_link_libraries (selftest zt-static ${ws2_32_LIBRARY_PATH} ${shlwapi_LIBRARY_PATH} ${iphlpapi_LIBRARY_PATH})
	else ()
		target_link_libraries (selftest zt-static pthread)
	endif ()
	# nativetest
	add_executable (nativetest ${PROJ_DIR}/test/selftest.cpp)
	target_compile_options (nativetest PRIVATE -D__NATIVETEST__)
endif ()
endif ()