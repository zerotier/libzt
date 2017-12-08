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

cmake_minimum_required (VERSION 2.8)
project (libzt)
set (libzt_VERSION_MAJOR 1)
set (libzt_VERSION_MINOR 0)

# --- dirs ---
set(PROJ_DIR ${PROJECT_SOURCE_DIR})
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

# ---include dirs ---

# lwip
set(LWIP_SRC_DIR ${PROJ_DIR}/ext/lwip/src)
include_directories("${LWIP_SRC_DIR}/include")
include_directories("${PROJ_DIR}/ext/lwip-contrib/ports/unix/include")
file(GLOB lwip_src_glob ${LWIP_SRC_DIR}/netif/*.c ${LWIP_SRC_DIR}/api/*.c ${PROJ_DIR}/ext/lwip-contrib/ports/unix/port/sys_arch.c  ${LWIP_SRC_DIR}/core/*.c ${LWIP_SRC_DIR}/core/ipv4/*.c ${LWIP_SRC_DIR}/core/ipv6/*.c)
add_library(lwip ${lwip_src_glob})

# zto
set(ZTO_SRC_DIR ${PROJ_DIR}/zto)
include_directories("${PROJ_DIR}/zto/include")
include_directories("${PROJ_DIR}/zto/osdep")
include_directories("${PROJ_DIR}/zto/node")
include_directories("${PROJ_DIR}/zto/service")
file(GLOB zto_src_glob "${ZTO_SRC_DIR}/node/*.cpp" "${ZTO_SRC_DIR}/service/*.cpp" "${ZTO_SRC_DIR}/osdep/OSUtils.cpp" "${ZTO_SRC_DIR}/controller/*.cpp" "${ZTO_SRC_DIR}/osdep/ManagedRoute.cpp")
add_library(zto ${zto_src_glob})
add_dependencies(zto zt)
add_library(httpparser "${ZTO_SRC_DIR}/ext/http-parser/http_parser.c")
target_compile_options(zto PRIVATE -std=c++11 -DZT_SDK=1)
target_link_libraries(zto httpparser zt)

# libzt
set(LIBZT_SRC_DIR ${PROJ_DIR}/src)
include_directories("${LIBZT_SRC_DIR}")
include_directories("${PROJ_DIR}/include")
file(GLOB libzt_src_glob ${LIBZT_SRC_DIR}/*.cpp)
add_library(zt STATIC ${libzt_src_glob})
target_compile_options(zt PRIVATE -std=c++11 -DZT_SDK=1 -DLIBZT_TRACE=1)
target_link_libraries(zt zto)
target_link_libraries(zt lwip)
set_target_properties(zt PROPERTIES OUTPUT_NAME zt)

# libzt shared
add_library(ztshared SHARED ${libzt_src_glob})
target_compile_options(ztshared PRIVATE -std=c++11 -DZT_SDK=1 -DLIBZT_TRACE=1)
target_link_libraries(ztshared zto)
target_link_libraries(ztshared lwip)
set_target_properties(ztshared PROPERTIES OUTPUT_NAME zt)


# --- executables ---

# test
add_executable(ipv4simple ${PROJ_DIR}/examples/bindings/cpp/ipv4simple/server.cpp)
target_link_libraries(ipv4simple lwip zto zt pthread)
target_compile_options(ipv4simple PRIVATE -std=c++11 -DLIBZT_TRACE=1)


# --- CONFIGURATION

if (LIBZT_TRACE EQUAL 1)
endif()

if (LIBZT_DEBUG EQUAL 1)
endif()

if (ZT_TRACE EQUAL 1)
endif()

if (ZT_DEBUG EQUAL 1)
endif()

if (NS_TRACE EQUAL 1)
endif()

if (NS_DEBUG EQUAL 1)
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
target_compile_options(ztshared PRIVATE -std=c++11 -DZT_SDK=1 -DLIBZT_TRACE=1 -DSDK_JNI=1)
# JNI
option (USE_JNI
        "Use Java JNI for shared/dynamic libraries" ON) 
endif()