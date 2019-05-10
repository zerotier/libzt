cmake_minimum_required (VERSION 3.0)
project (zt)

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
if (IOS_FRAMEWORK)
	set (BUILDING_IOS TRUE)
endif ()
if (BUILDING_DARWIN AND NOT IOS_FRAMEWORK)
	set (BUILDING_MACOS TRUE)
endif ()
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set (BUILDING_LINUX TRUE)
endif ()
if (${CMAKE_GENERATOR} STREQUAL "Xcode")
	set (IN_XCODE TRUE)
	set(XCODE_EMIT_EFFECTIVE_PLATFORM_NAME ON)
	#set_target_properties (${STATIC_LIB_NAME}
	#	PROPERTIES XCODE_ATTRIBUTE_MY_BUILD_ONLY_ACTIVE_ARCH YES)
	#set_target_properties (${STATIC_LIB_NAME}
	#	PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "${MY_BUILD_ONLY_ACTIVE_ARCH})
endif ()
if (BUILDING_WIN32 OR BUILDING_WIN64 OR MSVC)
	set (BUILDING_WIN TRUE)
endif ()
if (NOT BUILDING_ANDROID AND NOT IN_XCODE)
	set(SHOULD_BUILD_TESTS TRUE)
endif ()
if (BUILDING_WIN32 OR BUILDING_WIN64 OR MSVC)
	set (BUILDING_WIN TRUE)
endif ()

# -----------------------------------------------------------------------------
# |                                BUILD TYPES                                |
# -----------------------------------------------------------------------------

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "debug")
	set (DEBUG_BUILD ON)
	set (CMAKE_VERBOSE_MAKEFILE ON)
    set (CMAKE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)
	set (LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/lib)
endif()
if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "release")
	set (RELEASE_BUILD ON)
	set (CMAKE_VERBOSE_MAKEFILE OFF)
	set (CMAKE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)
	set (LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/lib)
endif()

set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
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

set (SILENCE "${SILENCE} -Wno-unused-parameter")
set (SILENCE "${SILENCE} -Wno-format")
set (SILENCE "${SILENCE} -Wno-tautological-constant-out-of-range-compare ")
set (SILENCE "${SILENCE} -Wno-macro-redefined")
set (SILENCE "${SILENCE} -Wno-parentheses-equality")
set (SILENCE "${SILENCE} -Wno-sign-compare")
set (SILENCE "${SILENCE} -Wno-unused-variable")
set (SILENCE "${SILENCE} -Wno-missing-field-initializers")
set (SILENCE "${SILENCE} -Wno-unused-parameter")
#set (SILENCE "${SILENCE} -Wno-nullability-completeness")
#set (SILENCE "${SILENCE} -Wno-expansion-to-defined")

set (ZT_FLAGS "${ZT_FLAGS} -DZT_USE_MINIUPNPC=1")
set (ZT_FLAGS "${ZT_FLAGS} -DZT_SOFTWARE_UPDATE_DEFAULT=0")
set (ZT_FLAGS "${ZT_FLAGS} -D_USING_LWIP_DEFINITIONS_=0")
set (ZT_FLAGS "${ZT_FLAGS} -DZT_SDK=1")

if (DEBUG_BUILD)
	set (LWIP_FLAGS "${LWIP_FLAGS} -DLWIP_DBG_TYPES_ON=128")
	set (LWIP_FLAGS "${LWIP_FLAGS} -DSOCKETS_DEBUG=128")
	set (LWIP_FLAGS "${LWIP_FLAGS} -DLWIP_STATS_LARGE=1")
	set (LWIP_FLAGS "${LWIP_FLAGS} -DLWIP_STATS=1")
	set (LWIP_FLAGS "${LWIP_FLAGS} -DLINK_STATS=1")
	set (LWIP_FLAGS "${LWIP_FLAGS} -DETHARP_STATS=1")
	set (LWIP_FLAGS "${LWIP_FLAGS} -DIPFRAG_STATS=1")
	set (LWIP_FLAGS "${LWIP_FLAGS} -DIP_STATS=1")
	set (LWIP_FLAGS "${LWIP_FLAGS} -DICMP_STATS=1")
	set (LWIP_FLAGS "${LWIP_FLAGS} -DIGMP_STATS=1")
	set (LWIP_FLAGS "${LWIP_FLAGS} -DUDP_STATS=1")
	set (LWIP_FLAGS "${LWIP_FLAGS} -DTCP_STATS=1")
	set (LWIP_FLAGS "${LWIP_FLAGS} -DSYS_STATS=1")
	set (LWIP_FLAGS "${LWIP_FLAGS} -DIP6_STATS=1")
	set (LWIP_FLAGS "${LWIP_FLAGS} -DICMP6_STATS=1")
	set (LWIP_FLAGS "${LWIP_FLAGS} -DIP6_FRAG_STATS=1")
	set (LWIP_FLAGS "${LWIP_FLAGS} -DMLD6_STATS=1")
	set (LWIP_FLAGS "${LWIP_FLAGS} -DND6_STATS=1")
else ()
	set (LWIP_FLAGS "${LWIP_FLAGS} -DLWIP_DBG_TYPES_ON=0")
endif ()

set (DEBUG_OPTIMIZATION "-O3")
set (RELEASE_OPTIMIZATION "-O3")

if (BUILDING_WIN)
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc -DNOMINMAX")
else ()
	set (CMAKE_C_FLAGS
		"${CMAKE_C_FLAGS} \
		 ${ZT_FLAGS} \
		 -fstack-protector")

	set (CMAKE_C_FLAGS_DEBUG
		"${CMAKE_C_FLAGS_DEBUG} \
		 ${DEBUG_OPTIMIZATION} \
		 -DLWIP_DEBUG=1 -DLIBZT_DEBUG=1")

	set (CMAKE_C_FLAGS_RELEASE
		"${SILENCE} \
		 ${CMAKE_C_FLAGS_RELEASE} \
		 ${RELEASE_OPTIMIZATION} \
		 -fstack-protector")

	set (CMAKE_CXX_FLAGS
		"${CMAKE_CXX_FLAGS} \
		 ${ZT_FLAGS} -Wall -Wextra -std=c++11")

	set (CMAKE_CXX_FLAGS_DEBUG
		"${CMAKE_CXX_FLAGS_DEBUG} \
		 ${DEBUG_OPTIMIZATION} \
		 -DLWIP_DEBUG=1 -DLIBZT_DEBUG=1")

	set (CMAKE_CXX_FLAGS_RELEASE
		"${CMAKE_CXX_FLAGS_RELEASE} \
		 ${SILENCE} \
		 ${RELEASE_OPTIMIZATION}")
endif ()

if (BUILDING_LINUX AND NOT BUILDING_ANDROID)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lpthread")
endif ()

# -----------------------------------------------------------------------------
# |                                      JNI                                  |
# -----------------------------------------------------------------------------

if (SDK_JNI OR BUILDING_ANDROID)
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
		if (BUILDING_MACOS)
			include_directories ("${JNI_INCLUDE_DIR}/darwin")
		endif ()
		if (BUILDING_LINUX)
			include_directories ("${JNI_INCLUDE_DIR}/linux")
		endif ()
	else ()
			message (STATUS "JNI not found")
	endif ()
	if (JNI_FOUND)
		add_definitions (-DSDK_JNI=1)
	endif ()
endif () # SDK_JNI

# -----------------------------------------------------------------------------
# |                            SOURCE FILE GLOBS                              |
# -----------------------------------------------------------------------------

set (PROJ_DIR ${PROJECT_SOURCE_DIR})
set (LWIP_SRC_DIR "${PROJ_DIR}/ext/lwip/src")
set (ZTO_SRC_DIR "${PROJ_DIR}/ext/ZeroTierOne")
set (LIBZT_SRC_DIR "${PROJ_DIR}/src")

file (GLOB ztcoreSrcGlob
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

# header globs for xcode frameworks
file (GLOB frameworkPrivateHeaderGlob include/ZeroTier.h include/ZeroTierConstants.h)
file (GLOB frameworkPublicHeaderGlob include/Xcode-Bridging-Header.h)
file (GLOB frameworkHeaderGlob ${frameworkPublicHeaderGlob} ${frameworkPrivateHeaderGlob})

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

# TODO: Should separate this into its own ios.cmake file

if (IOS_FRAMEWORK)
	# Controllers probably won't be run from iPhones, so we can omit JSON support
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOMIT_JSON_SUPPORT=1")
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DOMIT_JSON_SUPPORT=1")
	set(DEVROOT
		"/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer")
	if (IOS_ARM64)
		set (CMAKE_OSX_ARCHITECTURES arm64)
		set (SDKVER "11.4")
	endif ()
	if (IOS_ARMV7)
		# armv7 armv7s
		set (CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD_32_BIT)")
		set (SDKVER "10.0")
	endif ()

	set(SDKROOT "${DEVROOT}/SDKs/iPhoneOS${SDKVER}.sdk")
	if(EXISTS ${SDKROOT})
		set(CMAKE_OSX_SYSROOT "${SDKROOT}")
	else()
		message("Warning, iOS Base SDK path not found: " ${SDKROOT})
	endif()
endif ()

if (MACOS_FRAMEWORK)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOMIT_JSON_SUPPORT=1")
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DOMIT_JSON_SUPPORT=1")
	include_directories ("/Library/Developer/CommandLineTools/SDKs/MacOSX10.14.sdk/usr/include/")
endif ()

# -----------------------------------------------------------------------------
# |                     OBJECT LIBRARIES (INTERMEDIATE)                       |
# -----------------------------------------------------------------------------

# zto_obj
add_library (zto_obj OBJECT ${ztcoreSrcGlob})
set_target_properties (zto_obj PROPERTIES
	COMPILE_FLAGS "${ZT_FLAGS}")
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
target_compile_definitions(miniupnpc_obj
	PRIVATE ZT_USE_MINIUPNPC MINIUPNP_STATICLIB _DARWIN_C_SOURCE
	MINIUPNPC_SET_SOCKET_TIMEOUT MINIUPNPC_GET_SRC_ADDR _BSD_SOURCE
	_DEFAULT_SOURCE MINIUPNPC_VERSION_STRING=\"2.0\"
	UPNP_VERSION_STRING=\"UPnP/1.1\" ENABLE_STRNATPMPERR
	OS_STRING=\"Darwin/15.0.0\")
if (BUILDING_DARWIN AND NOT IOS_FRAMEWORK)
	target_compile_definitions(miniupnpc_obj PRIVATE MACOSX)
endif ()

# lwip_obj
add_library (lwip_obj OBJECT ${lwipSrcGlob})
set_target_properties (lwip_obj PROPERTIES COMPILE_FLAGS "${LWIP_FLAGS}")

# libzt_obj
add_library (libzt_obj OBJECT ${libztSrcGlob})
set_target_properties (libzt_obj PROPERTIES COMPILE_FLAGS "${ZT_FLAGS}")

# PIC

# zto_pic
add_library (zto_pic ${ztcoreSrcGlob})
set_target_properties (zto_pic PROPERTIES
	COMPILE_FLAGS "${ZT_FLAGS}"
	POSITION_INDEPENDENT_CODE ON)

# libnatpmp_pic
add_library (natpmp_pic ${libnatpmpSrcGlob})
set_target_properties (natpmp_pic PROPERTIES
	POSITION_INDEPENDENT_CODE ON)

# miniupnpc_pic
add_library (miniupnpc_pic ${libminiupnpcSrcGlob})
target_compile_definitions(miniupnpc_pic
	PRIVATE MACOSX ZT_USE_MINIUPNPC MINIUPNP_STATICLIB _DARWIN_C_SOURCE
	MINIUPNPC_SET_SOCKET_TIMEOUT MINIUPNPC_GET_SRC_ADDR _BSD_SOURCE
	_DEFAULT_SOURCE MINIUPNPC_VERSION_STRING=\"2.0\"
	UPNP_VERSION_STRING=\"UPnP/1.1\" ENABLE_STRNATPMPERR
	OS_STRING=\"Darwin/15.0.0\")
set_target_properties (miniupnpc_pic PROPERTIES
	POSITION_INDEPENDENT_CODE ON)

# lwip_pic
add_library (lwip_pic ${lwipSrcGlob})
set_target_properties (lwip_pic PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties (lwip_pic PROPERTIES COMPILE_FLAGS "${LWIP_FLAGS}")

# libzt_pic
add_library (zt_pic ${libztSrcGlob})
set_target_properties (zt_pic PROPERTIES
	COMPILE_FLAGS "${ZT_FLAGS}"
	POSITION_INDEPENDENT_CODE ON)

# -----------------------------------------------------------------------------
# |                      BUILD TARGETS (FINAL PRODUCT)                        |
# -----------------------------------------------------------------------------

# libztcore.a
add_library (ztcore STATIC $<TARGET_OBJECTS:zto_obj>)
set_target_properties (ztcore PROPERTIES
    OUTPUT_NAME ztcore
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
add_library (${STATIC_LIB_NAME} STATIC $<TARGET_OBJECTS:libzt_obj>
$<TARGET_OBJECTS:zto_obj>
$<TARGET_OBJECTS:libnatpmp_obj>
$<TARGET_OBJECTS:miniupnpc_obj>
$<TARGET_OBJECTS:lwip_obj> ${libztSrcGlob})
set_target_properties (${STATIC_LIB_NAME} PROPERTIES
    OUTPUT_NAME zt
    LIBRARY_OUTPUT_DIRECTORY ${INTERMEDIATE_LIBRARY_OUTPUT_PATH})
set_target_properties (${STATIC_LIB_NAME} PROPERTIES COMPILE_FLAGS "${ZT_FLAGS}")

# libzt.so/dylib/dll
add_library (${DYNAMIC_LIB_NAME} SHARED ${libztSrcGlob})
target_link_libraries (${DYNAMIC_LIB_NAME} zt_pic lwip_pic zto_pic natpmp_pic miniupnpc_pic)
set_target_properties (${DYNAMIC_LIB_NAME} PROPERTIES COMPILE_FLAGS "${ZT_FLAGS}")
set_target_properties (${DYNAMIC_LIB_NAME} PROPERTIES OUTPUT_NAME ${DYNAMIC_LIB_OUTPUT_NAME}
                                    WINDOWS_EXPORT_ALL_SYMBOLS true)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if (BUILDING_ANDROID)
	target_link_libraries (${DYNAMIC_LIB_NAME} android log)
endif ()

# xcode framework
if (IN_XCODE)
    include_directories (${frameworkHeaderGlob})

	add_library(${XCODE_FRAMEWORK_NAME} STATIC
		$<TARGET_OBJECTS:libzt_obj>
		$<TARGET_OBJECTS:zto_obj>
		$<TARGET_OBJECTS:libnatpmp_obj>
		$<TARGET_OBJECTS:miniupnpc_obj>
		$<TARGET_OBJECTS:lwip_obj>
		${libztSrcGlob}
		${frameworkHeaderGlob})

	set_target_properties(${XCODE_FRAMEWORK_NAME} PROPERTIES
		FRAMEWORK TRUE
		FRAMEWORK_VERSION A
		DEFINES_MODULE TRUE
		MACOSX_FRAMEWORK_IDENTIFIER com.cmake.${XCODE_FRAMEWORK_NAME}
		MODULE_MAP "~/op/zt/libzt/libzt_experimental/ports/module.modulemap"
		PUBLIC_HEADER "${frameworkHeaderGlob}"
		XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
	)
endif ()

# -----------------------------------------------------------------------------
# |                                EXECUTABLES                                |
# -----------------------------------------------------------------------------

if (SHOULD_BUILD_TESTS)
	# Minimal functional example
	#add_executable (example ${PROJ_DIR}/test/example.cpp)
	#target_link_libraries(example ${STATIC_LIB_NAME})

	# API test
	#add_executable (apitest ${PROJ_DIR}/test/apitest.cpp)
	#target_link_libraries(apitest ${STATIC_LIB_NAME})

	# Selftest
	#add_executable (selftest ${PROJ_DIR}/test/selftest.cpp)
	#target_link_libraries(selftest ${STATIC_LIB_NAME})
	#set_target_properties (selftest PROPERTIES COMPILE_FLAGS "-D__SELFTEST__")

	# client/server performance test
	#add_executable (client ${PROJ_DIR}/test/client.cpp)
	#target_link_libraries(client ${STATIC_LIB_NAME})
	#add_executable (server ${PROJ_DIR}/test/server.cpp)
	#target_link_libraries(server ${STATIC_LIB_NAME})

	# Simple Example
	#add_executable (simple ${PROJ_DIR}/test/simple.cpp)
	#target_link_libraries(simple ${STATIC_LIB_NAME})
endif ()

# -----------------------------------------------------------------------------
# |                                  INSTALL                                  |
# -----------------------------------------------------------------------------

set (PUBLIC_ZT_HEADERS
    ${PROJECT_SOURCE_DIR}/include/ZeroTier.h
    ${PROJECT_SOURCE_DIR}/include/ZeroTierConstants.h)

set_target_properties(${STATIC_LIB_NAME} PROPERTIES PUBLIC_HEADER "${PUBLIC_ZT_HEADERS}")
install (TARGETS ${STATIC_LIB_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include
)
install (TARGETS ${DYNAMIC_LIB_NAME}
        LIBRARY DESTINATION lib
)
