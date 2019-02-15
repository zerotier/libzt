#
# ZeroTier SDK - Network Virtualization Everywhere
# Copyright (C) 2011-2019  ZeroTier, Inc.  https://www.zerotier.com/
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
#  along with this program. If not, see <http://www.gnu.org/licenses/>.
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

# Library and executable output paths
if (NOT CMAKE_BUILD_TYPE)
    message( FATAL_ERROR "Must specify CMAKE_BUILD_TYPE, CMake will exit." )
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set (CMAKE_BINARY_DIR ${PROJECT_SOURCE_DIR}/bin/debug)
endif()
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set (CMAKE_BINARY_DIR ${PROJECT_SOURCE_DIR}/bin/release)
endif()

set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set (INTERMEDIATE_LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib/intermediate)

# -----------------------------------------------------------------------------
# |                                LIBRARY NAMES                              |
# -----------------------------------------------------------------------------

if (IN_XCODE)
	set (XCODE_FRAMEWORK_NAME ${PROJECT_NAME})
endif ()

if (BUILDING_WIN)
	# Possibly a CMake limitation? -- Can't share target output names
	set (STATIC_LIB_NAME ${PROJECT_NAME}-static)
	set (STATIC_LIB_OUTPUT_NAME ${PROJECT_NAME}-static)
	set (DYNAMIC_LIB_NAME ${PROJECT_NAME}-shared)
	set (DYNAMIC_LIB_OUTPUT_NAME ${PROJECT_NAME}-shared)
else ()
	set (STATIC_LIB_NAME ${PROJECT_NAME}-static)
	set (STATIC_LIB_OUTPUT_NAME ${PROJECT_NAME})
	set (DYNAMIC_LIB_NAME ${PROJECT_NAME}-shared)
	set (DYNAMIC_LIB_OUTPUT_NAME ${PROJECT_NAME})
endif ()

# -----------------------------------------------------------------------------
# |                                    FLAGS                                  |
# -----------------------------------------------------------------------------

set (SILENCE "-Wno-sign-compare -Wno-unused-parameter -Wno-unused-variable -Wno-missing-field-initializers")
set (LIBZT_FLAGS "-D_USING_LWIP_DEFINITIONS_=1 -DZT_SDK")
set (ZTCORE_FLAGS "-DZT_USE_MINIUPNPC=1 -DZT_SOFTWARE_UPDATE_DEFAULT=0")

if (BUILDING_WIN)
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc -DNOMINMAX")
else ()
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LIBZT_FLAGS} -fstack-protector")
	set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${LIBZT_FLAGS} -DLWIP_DEBUG=1 -DLIBZT_DEBUG=1")
	set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${LIBZT_FLAGS} -fstack-protector")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SILENCE} ${LIBZT_FLAGS} -O3 -Wall -Wextra -std=c++11")
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${SILENCE} ${LIBZT_FLAGS} -std=c++11 -DLWIP_DEBUG=1 -DLIBZT_DEBUG=1")
	set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${SILENCE} ${LIBZT_FLAGS} -O3 -std=c++11")
endif ()

# -----------------------------------------------------------------------------
# |                     PLATFORM/FEATURE AND IDE DETECTION                    |
# -----------------------------------------------------------------------------

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
if (BUILDING_WIN32 OR BUILDING_WIN64 OR MSVC)
	set (BUILDING_WIN TRUE)
endif ()
if (NOT BUILDING_ANDROID AND NOT IN_XCODE AND NOT BUILD_TESTS EQUAL 0)
	set(SHOULD_BUILD_TESTS TRUE)
endif ()
if (BUILDING_WIN32 OR BUILDING_WIN64 OR MSVC)
	set (BUILDING_WIN TRUE)
endif ()

# -----------------------------------------------------------------------------
# |                                      JNI                                  |
# -----------------------------------------------------------------------------

MESSAGE (STATUS "Looking for JNI")

if (BUILDING_WIN)
	# We are only interested in finding jni.h: we do not care about extended JVM
	# functionality or the AWT library.
	#set(JAVA_AWT_LIBRARY NotNeeded)
	#set(JAVA_JVM_LIBRARY NotNeeded)
	#set(JAVA_INCLUDE_PATH2 NotNeeded)
	#set(JAVA_AWT_INCLUDE_PATH NotNeeded)
	set(JAVA_INCLUDE_PATH "C:\\Program Files\\Java\\jdk-10.0.2\\include")
endif ()

set(JAVA_AWT_LIBRARY NotNeeded)
set(JAVA_JVM_LIBRARY NotNeeded)
set(JAVA_INCLUDE_PATH2 NotNeeded)
set(JAVA_AWT_INCLUDE_PATH NotNeeded)
find_package(JNI REQUIRED)

if (JNI_FOUND)
    message (STATUS "JNI_INCLUDE_DIR=${JNI_INCLUDE_DIRS}")
    message (STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
    list (GET JNI_INCLUDE_DIRS 0 JNI_INCLUDE_DIR)
    message (STATUS "jni path=${JNI_INCLUDE_DIR}")
    include_directories ("${JNI_INCLUDE_DIR}")
    #include_directories ("${JNI_INCLUDE_DIRS}")
	if (BUILDING_WIN)
		include_directories ("${JNI_INCLUDE_DIR}\\win32")
	endif ()
	if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin") # on macOS
		include_directories ("${JNI_INCLUDE_DIR}/darwin")
	endif ()
else ()
	    message (STATUS "JNI not found")
endif ()

if ((BUILDING_ANDROID OR JNI) AND JNI_FOUND)
	add_definitions (-DSDK_JNI=1)
endif ()

# -----------------------------------------------------------------------------
# |                            SOURCE FILE GLOBS                              |
# -----------------------------------------------------------------------------

set (PROJ_DIR ${PROJECT_SOURCE_DIR})
set (LWIP_SRC_DIR "${PROJ_DIR}/ext/lwip/src")
set (ZTO_SRC_DIR "${PROJ_DIR}/ext/ZeroTierOne")
set (LIBZT_SRC_DIR "${PROJ_DIR}/src")

file (GLOB zerotiercoreSrcGlob
	${ZTO_SRC_DIR}/node/*.cpp
	${ZTO_SRC_DIR}/osdep/OSUtils.cpp
    ${ZTO_SRC_DIR}/osdep/PortMapper.cpp
	${ZTO_SRC_DIR}/osdep/ManagedRoute.cpp)

file (GLOB libnatpmpSrcGlob
	${ZTO_SRC_DIR}/ext/libnatpmp/natpmp.c
	${ZTO_SRC_DIR}/ext/libnatpmp/getgateway.c)

file (GLOB libminiupnpcSrcGlob
    ${ZTO_SRC_DIR}/ext/miniupnpc/connecthostport.c
    ${ZTO_SRC_DIR}/ext/miniupnpc/igd_desc_parse.c 
    ${ZTO_SRC_DIR}/ext/miniupnpc/minisoap.c
    ${ZTO_SRC_DIR}/ext/miniupnpc/minissdpc.c
    ${ZTO_SRC_DIR}/ext/miniupnpc/miniupnpc.c 
    ${ZTO_SRC_DIR}/ext/miniupnpc/miniwget.c
    ${ZTO_SRC_DIR}/ext/miniupnpc/minixml.c
    ${ZTO_SRC_DIR}/ext/miniupnpc/portlistingparse.c 
    ${ZTO_SRC_DIR}/ext/miniupnpc/receivedata.c
    ${ZTO_SRC_DIR}/ext/miniupnpc/upnpcommands.c
    ${ZTO_SRC_DIR}/ext/miniupnpc/upnpdev.c
    ${ZTO_SRC_DIR}/ext/miniupnpc/upnperrors.c
    ${ZTO_SRC_DIR}/ext/miniupnpc/upnpreplyparse.c)

file (GLOB libztSrcGlob ${LIBZT_SRC_DIR}/*.cpp)

if (UNIX)
	set (LWIP_PORT_DIR ${PROJ_DIR}/ext/lwip-contrib/ports/unix/port)
endif ()

if (BUILDING_WIN)
	set (LWIP_PORT_DIR ${PROJ_DIR}/ext/lwip-contrib/ports/win32)
endif ()

file (GLOB lwipSrcGlob
	${LWIP_SRC_DIR}/netif/*.c
	${LWIP_SRC_DIR}/api/*.c
	${LWIP_PORT_DIR}/sys_arch.c
	${LWIP_SRC_DIR}/core/*.c
	${LWIP_SRC_DIR}/core/ipv4/*.c
	${LWIP_SRC_DIR}/core/ipv6/*.c)
list(REMOVE_ITEM lwipSrcGlob ${LWIP_SRC_DIR}/netif/slipif.c)

# -----------------------------------------------------------------------------
# |                                  INCLUDES                                 |
# -----------------------------------------------------------------------------

include_directories (${ZTO_SRC_DIR})
include_directories (${ZTO_SRC_DIR}/node)
include_directories (${ZTO_SRC_DIR}/osdep)
include_directories (${ZTO_SRC_DIR}/include)
include_directories (${ZTO_SRC_DIR}/ext/miniupnpc)
include_directories (${ZTO_SRC_DIR}/ext/libnatpmp)
include_directories (${PROJ_DIR}/src)
include_directories (${PROJ_DIR}/include)
include_directories (${LWIP_SRC_DIR}/include)
include_directories (${LWIP_PORT_DIR}/include)
include_directories (${PROJ_DIR}/ext/concurrentqueue)

# -----------------------------------------------------------------------------
# |                     OBJECT LIBRARIES (INTERMEDIATE)                       |
# -----------------------------------------------------------------------------

# zto_obj
add_library (zto_obj OBJECT ${zerotiercoreSrcGlob})
set_target_properties (zto_obj PROPERTIES COMPILE_FLAGS "${SILENCE} -std=c++11 -DZT_USE_MINIUPNPC=1 -DZT_SOFTWARE_UPDATE_DEFAULT=0")
if (BUILDING_WIN)
	target_link_libraries (zto_obj ws2_32)
	target_link_libraries (zto_obj ${shlwapi_LIBRARY_PATH})
	target_link_libraries (zto_obj ${iphlpapi_LIBRARY_PATH})
endif ()

# libnatpmp_obj
add_library (libnatpmp_obj OBJECT ${libnatpmpSrcGlob})
set_target_properties (libnatpmp_obj PROPERTIES COMPILE_FLAGS "")

# miniupnpc_obj
add_library (miniupnpc_obj OBJECT ${libminiupnpcSrcGlob})
target_compile_definitions(miniupnpc_obj PRIVATE MACOSX ZT_USE_MINIUPNPC MINIUPNP_STATICLIB _DARWIN_C_SOURCE MINIUPNPC_SET_SOCKET_TIMEOUT MINIUPNPC_GET_SRC_ADDR _BSD_SOURCE _DEFAULT_SOURCE MINIUPNPC_VERSION_STRING=\"2.0\" UPNP_VERSION_STRING=\"UPnP/1.1\" ENABLE_STRNATPMPERR OS_STRING=\"Darwin/15.0.0\")

# lwip_obj
add_library (lwip_obj OBJECT ${lwipSrcGlob})
set_target_properties (lwip_obj PROPERTIES COMPILE_FLAGS "")

# libzt_obj
add_library (libzt_obj OBJECT ${libztSrcGlob})
set_target_properties (libzt_obj PROPERTIES COMPILE_FLAGS "-std=c++11")

add_library (lwip_pic ${lwipSrcGlob})
set_target_properties (lwip_pic PROPERTIES POSITION_INDEPENDENT_CODE ON)
add_library (zto_pic ${zerotiercoreSrcGlob})
set_target_properties (zto_pic PROPERTIES COMPILE_FLAGS "${SILENCE} -std=c++11")
set_target_properties (zto_pic PROPERTIES POSITION_INDEPENDENT_CODE ON)

# -----------------------------------------------------------------------------
# |                      BUILD TARGETS (FINAL PRODUCT)                        |
# -----------------------------------------------------------------------------

# libzerotiercore.a
add_library (zerotiercore STATIC $<TARGET_OBJECTS:zto_obj>)
set_target_properties (zerotiercore PROPERTIES
    OUTPUT_NAME zerotiercore
    LIBRARY_OUTPUT_DIRECTORY ${INTERMEDIATE_LIBRARY_OUTPUT_PATH})

# libnatpmp.a
add_library (natpmp STATIC $<TARGET_OBJECTS:libnatpmp_obj>)
set_target_properties (natpmp PROPERTIES
    OUTPUT_NAME natpmp
    LIBRARY_OUTPUT_DIRECTORY ${INTERMEDIATE_LIBRARY_OUTPUT_PATH})

# libminiupnpc.a
add_library (miniupnpc STATIC $<TARGET_OBJECTS:miniupnpc_obj>)
set_target_properties (miniupnpc PROPERTIES
    OUTPUT_NAME miniupnpc
    LIBRARY_OUTPUT_DIRECTORY ${INTERMEDIATE_LIBRARY_OUTPUT_PATH})

# liblwip.a
add_library (lwip STATIC $<TARGET_OBJECTS:lwip_obj>)
set_target_properties (lwip PROPERTIES
    OUTPUT_NAME lwip
    LIBRARY_OUTPUT_DIRECTORY ${INTERMEDIATE_LIBRARY_OUTPUT_PATH})

# libzt.a
add_library (zt STATIC $<TARGET_OBJECTS:libzt_obj>
$<TARGET_OBJECTS:zto_obj>
$<TARGET_OBJECTS:libnatpmp_obj>
$<TARGET_OBJECTS:miniupnpc_obj>
$<TARGET_OBJECTS:lwip_obj>)
set_target_properties (zt PROPERTIES
    OUTPUT_NAME zt
    LIBRARY_OUTPUT_DIRECTORY ${INTERMEDIATE_LIBRARY_OUTPUT_PATH})

# libzt.so/dylib/dll
add_library (zt-shared SHARED ${libztSrcGlob})
message (STATUS ${libztSrcGlob})
target_link_libraries (zt-shared lwip_pic zto_pic)
set_target_properties (zt-shared PROPERTIES COMPILE_FLAGS "${SILENCE} -std=c++11 -DZT_SDK")
set_target_properties (zt-shared PROPERTIES OUTPUT_NAME ${DYNAMIC_LIB_OUTPUT_NAME}
                                    WINDOWS_EXPORT_ALL_SYMBOLS true)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if (BUILDING_ANDROID)
	target_link_libraries (zt-shared android log)
endif ()

# -----------------------------------------------------------------------------
# |                                EXECUTABLES                                |
# -----------------------------------------------------------------------------

if (SHOULD_BUILD_TESTS)
	add_executable (example ${PROJ_DIR}/test/example.cpp)
	target_link_libraries(example zt)
	add_executable (selftest ${PROJ_DIR}/test/selftest.cpp)
	target_link_libraries(selftest zt)
	set_target_properties (selftest PROPERTIES COMPILE_FLAGS "${SILENCE} -D__SELFTEST__")

	# Simple client/server example using ZeroTier sockets
	add_executable (client ${PROJ_DIR}/test/client.cpp)
	target_link_libraries(client zt)
	set_target_properties (client PROPERTIES COMPILE_FLAGS "${SILENCE} -D__SELFTEST__")
	add_executable (server ${PROJ_DIR}/test/server.cpp)
	target_link_libraries(server zt)
	set_target_properties (server PROPERTIES COMPILE_FLAGS "${SILENCE} -D__SELFTEST__")

	# Native client/server
	add_executable (client_native ${PROJ_DIR}/test/client.cpp)
	target_link_libraries(client_native zt)
	set_target_properties (client_native PROPERTIES COMPILE_FLAGS "${SILENCE}")
	set_target_properties (client_native PROPERTIES OUTPUT_NAME client_native)
	add_executable (server_native ${PROJ_DIR}/test/server.cpp)
	target_link_libraries(server_native zt)
	set_target_properties (server_native PROPERTIES COMPILE_FLAGS "${SILENCE}")
	set_target_properties (server_native PROPERTIES OUTPUT_NAME server_native)
endif ()