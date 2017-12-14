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

# BUILD OUTPUTS:

#---Libraries
#
# bin/lib/libzt.a
# bin/lib/libzt.so
# bin/lib/libzt.dylib
#
#---Sample apps
#
# bin/ipv4server
# bin/ipv4client
# bin/ipv6server
# bin/ipv6client
# bin/ztproxy
#
#---Tests
#
# bin/selftest
# bin/nativetest


cmake_minimum_required (VERSION 2.8)
project (libzt)
set (libzt_VERSION_MAJOR 1)
set (libzt_VERSION_MINOR 0)
set (ZT_DEFS -std=c++11 "-DZT_SDK=1")
set (LIBZT_DEFS -std=c++11 "-DZT_SDK=1")

# --- dirs ---
set (PROJ_DIR ${PROJECT_SOURCE_DIR})
set (CMAKE_BINARY_DIR ${PROJECT_SOURCE_DIR}/bin)
set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

# ---include dirs ---

# (lwip) lwIP Userspace Networking Stack
set(LWIP_SRC_DIR ${PROJ_DIR}/ext/lwip/src)
include_directories(${LWIP_SRC_DIR}/include)
include_directories(${PROJ_DIR}/ext/lwip-contrib/ports/unix/include)
file(GLOB lwip_src_glob 
	${LWIP_SRC_DIR}/netif/*.c 
	${LWIP_SRC_DIR}/api/*.c 
	${PROJ_DIR}/ext/lwip-contrib/ports/unix/port/sys_arch.c
	${LWIP_SRC_DIR}/core/*.c 
	${LWIP_SRC_DIR}/core/ipv4/*.c 
	${LWIP_SRC_DIR}/core/ipv6/*.c)
add_library(lwip STATIC ${lwip_src_glob})
target_compile_options(lwip PRIVATE ${NS_DEFS})

add_library(lwip_pic STATIC ${lwip_src_glob})
target_compile_options(lwip_pic PRIVATE ${NS_DEFS})
set_target_properties(lwip_pic PROPERTIES POSITION_INDEPENDENT_CODE ON)



# (zto) ZeroTier Core Service
set(ZTO_SRC_DIR ${PROJ_DIR}/zto)
include_directories(${ZTO_SRC_DIR}/include)
include_directories(${ZTO_SRC_DIR}/osdep)
include_directories(${ZTO_SRC_DIR}/node)
include_directories(${ZTO_SRC_DIR}/service)
file(GLOB zto_src_glob 
	${ZTO_SRC_DIR}/node/*.cpp
	${ZTO_SRC_DIR}/service/*.cpp
	${ZTO_SRC_DIR}/osdep/OSUtils.cpp
	${ZTO_SRC_DIR}/controller/*.cpp
	${ZTO_SRC_DIR}/osdep/ManagedRoute.cpp)
add_library(zto STATIC ${zto_src_glob})
target_compile_options(zto PRIVATE ${ZT_DEFS})
target_link_libraries(zto http)
target_link_libraries(zto zt)

add_library(zto_pic STATIC ${zto_src_glob})
target_compile_options(zto_pic PRIVATE ${ZT_DEFS})
target_link_libraries(zto_pic http_pic)
target_link_libraries(zto_pic zt)
set_target_properties(zto_pic PROPERTIES POSITION_INDEPENDENT_CODE ON)




# (http) HTTP Control Plane for ZT Service
add_library(http STATIC "${ZTO_SRC_DIR}/ext/http-parser/http_parser.c")

add_library(http_pic STATIC "${ZTO_SRC_DIR}/ext/http-parser/http_parser.c")
set_target_properties(http_pic PROPERTIES POSITION_INDEPENDENT_CODE ON)



# (libzt) Static Library variant of ZeroTier paired with userspace stack and virtual tap interface
set (LIBZT_SRC_DIR ${PROJ_DIR}/src)
include_directories("${LIBZT_SRC_DIR}")
include_directories("${PROJ_DIR}/include")
file(GLOB libzt_src_glob ${LIBZT_SRC_DIR}/*.cpp)
add_library(zt ${libzt_src_glob})
target_compile_options(zt PRIVATE ${LIBZT_DEFS})
target_link_libraries(zt zto)
target_link_libraries(zt lwip)
set_target_properties(zt PROPERTIES OUTPUT_NAME zt)

# (libzt) Shared Library variant of ZeroTier paired with userspace stack and virtual tap interface
add_library(ztshared SHARED ${libzt_src_glob})
target_compile_options(ztshared PRIVATE ${LIBZT_DEFS})
target_link_libraries(ztshared zto_pic)
target_link_libraries(ztshared lwip_pic)
set_target_properties(ztshared PROPERTIES OUTPUT_NAME zt)


# --- executables ---

# Test Executables/Apps
add_executable(ipv4simple ${PROJ_DIR}/examples/bindings/cpp/ipv4simple/server.cpp)
target_link_libraries(ipv4simple lwip zto zt pthread)
target_compile_options(ipv4simple PRIVATE -std=c++11 -DLIBZT_TRACE=1)

# Clean up intermediate library targets
file(REMOVE {LIBRARY_OUTPUT_PATH}/libhttp.a)
file(REMOVE {LIBRARY_OUTPUT_PATH}/libhttp_pic.a)
file(REMOVE {LIBRARY_OUTPUT_PATH}/libzto.a)
file(REMOVE {LIBRARY_OUTPUT_PATH}/libzto_pic.a)
file(REMOVE {LIBRARY_OUTPUT_PATH}/liblwip.a)
file(REMOVE {LIBRARY_OUTPUT_PATH}/liblwip_pic.a)

# --- CONFIGURATION

if (LIBZT_TRACE EQUAL 1)
set (LIBZT_DEFS "${LIBZT_DEFS} -DLIBZT_TRACE=1")
endif()
if (LIBZT_DEBUG EQUAL 1)
set (LIBZT_DEFS "${LIBZT_DEFS} -DLIBZT_DEBUG=1")
endif()
if (ZT_TRACE EQUAL 1)
set (ZT_DEFS "${ZT_DEFS} -DZT_TRACE=1")
endif()
if (ZT_DEBUG EQUAL 1)
set (ZT_DEFS "${ZT_DEFS} -DZT_DEBUG=1")
endif()
if (NS_TRACE EQUAL 1)
set (NS_DEFS "${NS_DEFS} -DNS_TRACE=1")
endif()
if (NS_DEBUG EQUAL 1)
set (NS_DEFS "${NS_DEFS} -DNS_DEBUG=1")
endif()

if (JNI EQUAL 1)
MESSAGE(STATUS "Looking for JNI headers")
# --- Locate JNI headers 
find_package(JNI)
if (JNI_FOUND)
    message (STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
    message (STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
    list(GET JNI_INCLUDE_DIRS 0 JNI_INCLUDE_DIR)
    message (STATUS "chosen=${JNI_INCLUDE_DIR}")
    include_directories("${JNI_INCLUDE_DIR}")
else()
    message (STATUS "JNI not found")
endif()
target_compile_options(zt PRIVATE -std=c++11 -DZT_SDK=1 -DLIBZT_TRACE=1 -DSDK_JNI=1)
# JNI
option (USE_JNI
        "Use Java JNI for shared/dynamic libraries" ON) 
endif()