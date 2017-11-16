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

# Automagically pick clang or gcc, with preference for clang
# This is only done if we have not overridden these with an environment or CLI variable
ifeq ($(origin CC),default)
	CC=$(shell if [ -e /usr/bin/clang ]; then echo clang; else echo gcc; fi)
endif
ifeq ($(origin CXX),default)
	CXX=$(shell if [ -e /usr/bin/clang++ ]; then echo clang++; else echo g++; fi)
endif

OSTYPE=$(shell uname -s | tr '[A-Z]' '[a-z]')
BUILD=build/$(OSTYPE)
LWIPCONTRIBDIR=ext/lwip-contrib

# Windows
ifeq ($(OSTYPE),mingw32_nt-6.2)
ARTOOL=ar
ARFLAGS=rcs
CC=gcc
CXX=g++
CXXFLAGS+=-fpermissive -Wno-unknown-pragmas -Wno-pointer-arith -Wno-deprecated-declarations -Wno-conversion-null
WINDEFS=-lws2_32 -lshlwapi -liphlpapi -static -static-libgcc -static-libstdc++
LWIPARCHINCLUDE=$(LWIPCONTRIBDIR)/ports/win32/include
endif
# Darwin
ifeq ($(OSTYPE),darwin)
ARTOOL=libtool
ARFLAGS=-static
LWIPARCHINCLUDE=$(LWIPCONTRIBDIR)/ports/unix/include
endif
# Linux
ifeq ($(OSTYPE),linux)
ARTOOL=ar
ARFLAGS=rcs
LWIPARCHINCLUDE=$(LWIPCONTRIBDIR)/ports/unix/include
endif
# FreeBSD
ifeq ($(OSTYPE),freebsd)
ARTOOL=ar
ARFLAGS=rcs
LWIPARCHINCLUDE=$(LWIPCONTRIBDIR)/ports/unix/include
endif
# OpenBSD
ifeq ($(OSTYPE),openbsd)
ARTOOL=ar
ARFLAGS=rcs
LWIPARCHINCLUDE=$(LWIPCONTRIBDIR)/ports/unix/include
endif

##############################################################################
## Objects and includes                                                     ##
##############################################################################

include zto/objects.mk

ZTO_OBJS=\
	zto/node/C25519.o \
	zto/node/Capability.o \
	zto/node/CertificateOfMembership.o \
	zto/node/CertificateOfOwnership.o \
	zto/node/Identity.o \
	zto/node/IncomingPacket.o \
	zto/node/InetAddress.o \
	zto/node/Membership.o \
	zto/node/Multicaster.o \
	zto/node/Network.o \
	zto/node/NetworkConfig.o \
	zto/node/Node.o \
	zto/node/OutboundMulticast.o \
	zto/node/Packet.o \
	zto/node/Path.o \
	zto/node/Peer.o \
	zto/node/Poly1305.o \
	zto/node/Revocation.o \
	zto/node/Salsa20.o \
	zto/node/SelfAwareness.o \
	zto/node/SHA512.o \
	zto/node/Switch.o \
	zto/node/Tag.o \
	zto/node/Topology.o \
	zto/node/Trace.o \
	zto/node/Utils.o \
	zto/controller/EmbeddedNetworkController.o \
	zto/controller/DB.o \
	zto/controller/FileDB.o \
	zto/osdep/ManagedRoute.o \
	zto/osdep/Http.o \
	zto/osdep/OSUtils.o \
	zto/service/SoftwareUpdater.o \
	zto/service/OneService.o \
	zto/ext/http-parser/http_parser.o

ZT_INCLUDES+=-Iext \
	-Izto/osdep \
	-Izto/node \
	-Izto/service \
	-Izto/include \

LIBZT_INCLUDES+=-Iinclude \
	-I../include \
	-Isrc

##############################################################################
## General Build Configuration                                              ##
##############################################################################

#    ZT_ Configuration options for ZeroTier core 
# LIBZT_ Configuration options for libzt
#    NS_ Configuration options for userspace network stack

STRIP=strip

# ZeroTier debug and tracing
ifeq ($(ZT_DEBUG),1)
	CFLAGS?=-Wall -g -pthread
	STRIP=echo
else
	CFLAGS?=-Ofast -fstack-protector
	CFLAGS?=-Wall -fPIE -fvisibility=hidden -pthread
endif

ifeq ($(ZT_TRACE),1)
	ZT_DEFS+=-DZT_TRACE
endif

# libzt debuf and tracing
ifeq ($(LIBZT_DEBUG),1)
	CFLAGS?=-Wall -g -pthread
	STRIP=echo
else
	CFLAGS?=-Ofast -fstack-protector
	CFLAGS?=-Wall -fPIE -fvisibility=hidden -pthread
endif

ifeq ($(LIBZT_TRACE),1)
	LIBZT_DEFS+=-DLIBZT_DEBUG
endif
# For using experimental stack drivers which interface via raw API's 
ifeq ($(LIBZT_RAW),1)
	LIBZT_DEFS+=-DLIBZT_RAW=1
endif

ifeq ($(NS_DEBUG),1)
	CFLAGS+=-Wall -g
	STRIP=echo
endif

# Build with address sanitization library for advanced debugging (clang)
# TODO: Add GCC version as well
ifeq ($(LIBZT_SANITIZE),1)
	SANFLAGS+=-x c++ -g -fsanitize=address -DASAN_OPTIONS=symbolize=1 \
		-DASAN_SYMBOLIZER_PATH=$(shell which llvm-symbolizer)
endif

# JNI (Java Native Interface)
# jni.h
JNI_INCLUDES+=-I$(shell /usr/libexec/java_home)/include 
# jni_md.h
JNI_INCLUDES+=-I$(shell /usr/libexec/java_home)/include/$(OSTYPE)

CXXFLAGS+=$(CFLAGS) -Wno-format -fno-rtti -std=c++11
ZT_DEFS+=-DZT_SDK -DZT_SOFTWARE_UPDATE_DEFAULT="\"disable\""
LIBZT_FILES:=src/VirtualTap.cpp src/libzt.cpp src/Utilities.cpp
STATIC_LIB=$(BUILD)/libzt.a

##############################################################################
## Stack Configuration                                                      ##
##############################################################################

#ifeq ($(NO_STACK),1)
#STACK_DRIVER_DEFS+=-DNO_STACK
#endif

ifeq ($(NS_DEBUG),1)
	STACK_DEFS+=LWIP_DEBUG=1
endif

# picoTCP
STACK_INCLUDES+=-Iext/picotcp/include -Iext/picotcp/build/include
STACK_DRIVER_FILES:=src/picoTCP.cpp

# lwIP
LWIPDIR=ext/lwip/src
STACK_INCLUDES+=-I$(LWIPARCHINCLUDE) -Iext/lwip/src/include/lwip \
	-I$(LWIPDIR)/include \
	-I$(LWIPARCH)/include \
	-I$(LWIPDIR)/include/ipv4 \
	-I$(LWIPDIR) \
	-Iext
STACK_DRIVER_FILES:=src/lwIP.cpp

##############################################################################
## Targets                                                                  ##
##############################################################################

%.o : %.cpp
	@mkdir -p $(BUILD) obj
	$(CXX) $(CXXFLAGS) $(STACK_DRIVER_DEFS) $(ZT_DEFS) \
		$(ZT_INCLUDES) $(LIBZT_INCLUDES) -c $^ -o obj/$(@F)

%.o : %.c
	@mkdir -p $(BUILD) obj
	$(CC) $(CFLAGS) -c $^ -o obj/$(@F)

core:
	cd zto; make core
	mv zto/libzerotiercore.a $(BUILD)

picotcp:
	cd ext/picotcp; make lib ARCH=shared IPV4=1 IPV6=1

lwip:
	echo $(STACK_DEFS)
	make -f make-liblwip.mk liblwip.a LIBZT_IPV4=1 IPV4=1

lwip_driver:
	$(CXX) $(CXXFLAGS) -c src/lwIP.cpp \
		$(ZT_DEFS) $(ZT_INCLUDES) $(STACK_INCLUDES) $(LIBZT_DEFS) $(LIBZT_INCLUDES) -DZT_DRIVER_MODULE

picotcp_driver:
	$(CXX) $(CXXFLAGS) -c src/picoTCP.cpp \
		$(ZT_DEFS) $(ZT_INCLUDES) $(STACK_INCLUDES) $(LIBZT_DEFS) $(LIBZT_INCLUDES) -DZT_DRIVER_MODULE

libzt_socket_layer:
	$(CXX) $(CXXFLAGS) -c src/VirtualSocket.cpp \
		$(ZT_DEFS) $(ZT_INCLUDES) $(LIBZT_INCLUDES) $(LIBZT_DEFS)
	$(CXX) $(CXXFLAGS) -c src/VirtualSocketLayer.cpp \
		$(ZT_DEFS) $(ZT_INCLUDES) $(STACK_INCLUDES) $(LIBZT_INCLUDES) $(LIBZT_DEFS) 
	$(CXX) $(CXXFLAGS) -c src/VirtualTap.cpp \
		$(ZT_DEFS) $(ZT_INCLUDES) $(LIBZT_DEFS) $(LIBZT_INCLUDES) 
	$(CXX) $(CXXFLAGS) -c src/ZT1Service.cpp \
		$(ZT_DEFS) $(ZT_INCLUDES) $(LIBZT_INCLUDES) $(LIBZT_DEFS)
	$(CXX) $(CXXFLAGS) -c src/libzt.cpp \
		$(ZT_DEFS) $(ZT_INCLUDES) $(STACK_INCLUDES) $(LIBZT_DEFS) $(LIBZT_INCLUDES)
	$(CXX) $(CXXFLAGS) -c src/RingBuffer.cpp $(LIBZT_INCLUDES)

jni_socket_wrapper:
	$(CXX) $(CXXFLAGS) -DSDK_JNI -c src/libztJNI.cpp \
		$(ZT_DEFS) $(ZT_INCLUDES) $(STACK_INCLUDES) $(JNI_INCLUDES) $(LIBZT_DEFS) $(LIBZT_INCLUDES)

utilities:
	$(CXX) $(CXXFLAGS) -c src/SysUtils.cpp \
		$(LIBZT_INCLUDES)
	$(CXX) $(CXXFLAGS) -c src/Utilities.cpp \
		$(ZT_DEFS) $(ZT_INCLUDES) $(LIBZT_INCLUDES) $(STACK_INCLUDES)

# windows DLL
win_dll: lwip lwip_driver libzt_socket_layer utilities $(ZTO_OBJS)
	# First we use mingw to build our DLL
	@mkdir -p $(BUILD) obj
	mv *.o obj
	windres -i res/libztdll.rc -o obj/libztdllres.o
	$(CXX) $(CXXFLAGS) -shared -o $(BUILD)/libzt.dll obj/*.o -Wl,--output-def,$(BUILD)/libzt.def,--out-implib,$(BUILD)/libzt.a $(WINDEFS)
	$(STRIP) $(BUILD)/libzt.dll
	# Then do the following to generate the MSVC DLL from the def file (which was generated from the MinGW DLL): 
	# lib /machine:x64 /def:libzt.def
	# or just execute: makelib

# ordinary shared library
shared_lib: lwip lwip_driver libzt_socket_layer utilities $(ZTO_OBJS)
	@mkdir -p $(BUILD) obj
	mv *.o obj
	$(CXX) $(CXXFLAGS) -shared -o $(BUILD)/libzt.so obj/*.o

# dynamic library for use with Java JNI, scala, etc
shared_jni_lib: lwip lwip_driver libzt_socket_layer jni_socket_wrapper utilities $(ZTO_OBJS)
	@mkdir -p $(BUILD) obj
	mv *.o obj
	$(CXX) $(CXXFLAGS) -dynamiclib -o $(BUILD)/libzt.dylib obj/*.o

# static library
static_lib: picotcp picotcp_driver lwip lwip_driver libzt_socket_layer utilities $(ZTO_OBJS)
	@mkdir -p $(BUILD) obj
	mv *.o obj
	mv ext/picotcp/build/lib/*.o obj
	mv ext/picotcp/build/modules/*.o obj
	$(ARTOOL) $(ARFLAGS) -o $(STATIC_LIB) obj/*.o

##############################################################################
## iOS/macOS App Frameworks                                                 ##
##############################################################################

ios_app_framework:
	cd examples/apple/ZeroTierSDK_Apple; xcodebuild -configuration Release \
		-scheme ZeroTierSDK_iOS build SYMROOT="../../../$(BUILD)/ios_app_framework"
	cd examples/apple/ZeroTierSDK_Apple; xcodebuild -configuration Debug \
		-scheme ZeroTierSDK_iOS build SYMROOT="../../../$(BUILD)/ios_app_framework"

macos_app_framework:
	cd examples/apple/ZeroTierSDK_Apple; xcodebuild -configuration Release \
		-scheme ZeroTierSDK_OSX build SYMROOT="../../../$(BUILD)/macos_app_framework"
	cd examples/apple/ZeroTierSDK_Apple; xcodebuild -configuration Debug \
		-scheme ZeroTierSDK_OSX build SYMROOT="../../../$(BUILD)/macos_app_framework"

##############################################################################
## Python module                                                            ##
##############################################################################

python_module:
	swig -cpperraswarn -python -c++ -o examples/python/libzt.cc examples/python/swig_libzt.i
	python examples/python/setup.py build_ext --inplace --swig-opts="-modern -I../../zto/include"

##############################################################################
## Unit Tests                                                               ##
##############################################################################

tests: selftest nativetest ztproxy simple
# intercept

ZT_UTILS:=zto/node/Utils.cpp -Izto/node

sample:
	$(CXX) $(CXXFLAGS) -D__SELFTEST__ $(STACK_DRIVER_DEFS) $(LIBZT_DEFS) \
		$(SANFLAGS) $(LIBZT_INCLUDES) $(ZT_INCLUDES) $(ZT_UTILS) test/sample.cpp -o \
		$(BUILD)/sample -L$(BUILD) -lzt
selftest:
	$(CXX) $(CXXFLAGS) -D__SELFTEST__ $(STACK_DRIVER_DEFS) $(LIBZT_DEFS) \
		$(SANFLAGS) $(LIBZT_INCLUDES) $(ZT_INCLUDES) $(ZT_UTILS) test/selftest.cpp -o \
		$(BUILD)/selftest -L$(BUILD) -lzt -lpthread
nativetest:
	$(CXX) $(CXXFLAGS) -D__NATIVETEST__ $(STACK_DRIVER_DEFS) $(SANFLAGS) \
		$(LIBZT_INCLUDES) $(ZT_INCLUDES) test/selftest.cpp -o $(BUILD)/nativetest
ztproxy:
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(LIBZT_INCLUDES) $(LIBZT_DEFS) $(ZT_INCLUDES) \
		examples/apps/ztproxy/ztproxy.cpp -o $(BUILD)/ztproxy $< -L$(BUILD) -lzt -lpthread $(WINDEFS)
intercept:
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(STACK_DRIVER_DEFS) $(LIBZT_INCLUDES) \
		$(ZT_INCLUDES) examples/intercept/intercept.cpp -D_GNU_SOURCE \
		-shared -o $(BUILD)/intercept.so $< -ldl
simple:
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(LIBZT_INCLUDES) $(LIBZT_DEFS) examples/bindings/cpp/simple_client_server/client.cpp -o $(BUILD)/client -L$(BUILD) -lpthread -lzt $(WINDEFS)
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(LIBZT_INCLUDES) $(LIBZT_DEFS) examples/bindings/cpp/simple_client_server/server.cpp -o $(BUILD)/server -L$(BUILD) -lpthread -lzt $(WINDEFS)
dlltest:
	$(CXX) $(CXXFLAGS)


##############################################################################
## Misc                                                                     ##
##############################################################################

standardize:
	vera++ --transform trim_right src/*.cpp
	vera++ --transform trim_right include/*.h

clean:
	-rm -rf .depend
	-rm -f *.o *.s *.exp *.lib .depend* *.core core
	-rm -rf $(BUILD)/*
	-rm -rf obj/*
	-find . -type f \( -name '*.a' -o -name '*.o' -o -name '*.so' -o -name \
		'*.o.d' -o -name '*.out' -o -name '*.log' -o -name '*.dSYM' \) -delete	

time:
	@date +"Build script finished on %F %T"