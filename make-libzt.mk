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
BUILDPATH=build/$(OSTYPE)
LIBPATH=lib
LWIPCONTRIBDIR=ext/lwip-contrib

# default
CFLAGS+=-Wall

# Windows
ifeq ($(OSTYPE),mingw32_nt-6.2)
ARTOOL=ar
ARFLAGS=rcs

CC=gcc
CXX=g++
CFLAGS=
CXXFLAGS+=-fpermissive -Wno-unknown-pragmas -Wno-pointer-arith -Wno-deprecated-declarations -Wno-conversion-null
WINDEFS=-lws2_32 -lshlwapi -liphlpapi -static -static-libgcc -static-libstdc++
LWIPARCHINCLUDE=$(LWIPCONTRIBDIR)/ports/win32/include
STATIC_LIB=libzt.a
SHARED_LIB=libzt.dll
endif
# Darwin
ifeq ($(OSTYPE),darwin)
CFLAGS+=-fvisibility=hidden -fstack-protector
ARTOOL=libtool
ARFLAGS=-static
LWIPARCHINCLUDE=$(LWIPCONTRIBDIR)/ports/unix/include
STATIC_LIB=libzt.a
SHARED_LIB=libzt.dylib
endif
# Linux
ifeq ($(OSTYPE),linux)
CFLAGS+=-fvisibility=hidden -fstack-protector
ARTOOL=ar
ARFLAGS=rcs
LWIPARCHINCLUDE=$(LWIPCONTRIBDIR)/ports/unix/include
STATIC_LIB=libzt.a
SHARED_LIB=libzt.so
endif
# FreeBSD
ifeq ($(OSTYPE),freebsd)
CFLAGS+=-fvisibility=hidden -fstack-protector
ARTOOL=ar
ARFLAGS=rcs
LWIPARCHINCLUDE=$(LWIPCONTRIBDIR)/ports/unix/include
STATIC_LIB=libzt.a
SHARED_LIB=libzt.so
endif
# OpenBSD
ifeq ($(OSTYPE),openbsd)
CFLAGS+=-fvisibility=hidden -fstack-protector
ARTOOL=ar
ARFLAGS=rcs
LWIPARCHINCLUDE=$(LWIPCONTRIBDIR)/ports/unix/include
STATIC_LIB=libzt.a
SHARED_LIB=libzt.so
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
# Determine if PIC is needed for the intended build target
ifeq ($(MAKECMDGOALS),static_lib)
	CFLAGS?=
	#-fPIE
endif
ifeq ($(MAKECMDGOALS),shared_lib)
	CFLAGS?=
	#-fPIC
endif
# ZeroTier core debug and tracing
ifeq ($(ZT_DEBUG),1)
	CFLAGS?=-g 
	ZT_DEFS?=-DZT_TRACE=1
	STRIP=echo
else
	CFLAGS?=-Ofast
endif
# For consistency. ZT_DEBUG=1 will also turn this on
ifeq ($(ZT_TRACE),1)
	ZT_DEFS?=-DZT_TRACE
endif
# Don't optimize, add debug symbols
ifeq ($(LIBZT_DEBUG),1)
	CFLAGS?=-g
	STRIP=echo
else
	CFLAGS?=-Ofast
endif
# Turns on file/function/line debug output logging
ifeq ($(LIBZT_TRACE),1)
	LIBZT_DEFS?=-DLIBZT_TRACE
endif
# Experimental stack drivers which interface via raw API's 
ifeq ($(LIBZT_RAW),1)
	LIBZT_DEFS?=-DLIBZT_RAW=1
endif
# Debug the userspace stack
ifeq ($(NS_DEBUG),1)
	CFLAGS?=-g
	STRIP=echo
endif

# Build with address sanitization library for advanced debugging (clang)
# TODO: Add GCC version as well
ifeq ($(LIBZT_SANITIZE),1)
	SANFLAGS+=-x c++ -g -fsanitize=address -DASAN_OPTIONS=symbolize=1 \
		-DASAN_SYMBOLIZER_PATH=$(shell which llvm-symbolizer)
endif

# JNI (Java Native Interface)
ifeq ($(OSTYPE),darwin)
JNI_INCLUDES+=-I$(shell /usr/libexec/java_home)/include 
JNI_INCLUDES+=-I$(shell /usr/libexec/java_home)/include/$(OSTYPE) 
endif
ifeq ($(OSTYPE),linux)
JNI_INCLUDES+=-I$(shell dirname $(shell dirname $(shell readlink -f $(shell which javac))))/include 
JNI_INCLUDES+=-I$(shell dirname $(shell dirname $(shell readlink -f $(shell which javac))))/include/$(OSTYPE) 
endif

CXXFLAGS+=$(CFLAGS) -Wno-format -fno-rtti -std=c++11
ZT_DEFS+=-DZT_SDK -DZT_SOFTWARE_UPDATE_DEFAULT="\"disable\""
LIBZT_FILES:=src/VirtualTap.cpp src/libzt.cpp src/Utilities.cpp

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
	@mkdir -p $(BUILDPATH) obj
	$(CXX) $(CXXFLAGS) $(STACK_DRIVER_DEFS) $(ZT_DEFS) \
		$(ZT_INCLUDES) $(LIBZT_INCLUDES) -c $^ -o obj/$(@F)

%.o : %.c
	@mkdir -p $(BUILDPATH) obj
	$(CC) $(CFLAGS) -c $^ -o obj/$(@F)

core:
	cd zto; make core
	mv zto/libzerotiercore.a $(BUILDPATH)

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
	$(CXX) $(CXXFLAGS) -c src/libztJNI.cpp \
		$(ZT_DEFS) $(ZT_INCLUDES) $(STACK_INCLUDES) $(JNI_INCLUDES) $(LIBZT_DEFS) $(LIBZT_INCLUDES)

utilities:
	$(CXX) $(CXXFLAGS) -c src/SysUtils.cpp \
		$(LIBZT_INCLUDES)
	$(CXX) $(CXXFLAGS) -c src/Utilities.cpp \
		$(ZT_DEFS) $(ZT_INCLUDES) $(LIBZT_INCLUDES) $(STACK_INCLUDES)

remove_objs:
	rm -rf *.o

prereqs: remove_objs lwip lwip_driver libzt_socket_layer jni_socket_wrapper utilities

static_lib: prereqs $(ZTO_OBJS)
	mv *.o obj
	mkdir -p lib
	$(ARTOOL) $(ARFLAGS) obj/*.o -o $(LIBPATH)/$(STATIC_LIB)

shared_lib: prereqs $(ZTO_OBJS)
	mv *.o obj
	mkdir -p lib
	$(CXX) $(CXXFLAGS) obj/*.o $(SOLIBTYPE) -o $(LIBPATH)/$(SHARED_LIB) -lpthread

dynamic_lib: prereqs $(ZTO_OBJS)

.PHONY: err
err: 
	@echo $(ERR)

# Sanity Checks
ifdef JNI
ifndef SHARED
ERR='Advice: In order to use Java JNI you must specify SHARED=1'
lib: err
endif
endif


ifdef JNI
ZT_DEFS+=-DSDK_JNI
SOLIBTYPE=-shared
else
SOLIBTYPE=-shared
# user dynamiclib for macOS
endif
ifdef STATIC
CFLAGS+=
#-fPIE
lib: static_lib
endif
ifdef SHARED
CFLAGS+=
#-fPIC
lib: shared_lib
endif

# windows DLL
win_dll: lwip lwip_driver libzt_socket_layer utilities $(ZTO_OBJS)
	# First we use mingw to build our DLL
	@mkdir -p $(BUILDPATH) obj
	mv *.o obj
	windres -i res/libztdll.rc -o obj/libztdllres.o
	$(CXX) $(CXXFLAGS) -shared -o $(BUILDPATH)/libzt.dll obj/*.o -Wl,--output-def,$(BUILDPATH)/libzt.def,--out-implib,$(BUILDPATH)/libzt.a $(WINDEFS)
	$(STRIP) $(BUILDPATH)/libzt.dll
	# Then do the following to generate the MSVC DLL from the def file (which was generated from the MinGW DLL): 
	# lib /machine:x64 /def:libzt.def
	# or just execute: makelib

# ordinary shared library
#shared_lib: lwip lwip_driver libzt_socket_layer utilities $(ZTO_OBJS)
#	@mkdir -p $(BUILDPATH) obj
#	mv *.o obj
#	$(CXX) $(CXXFLAGS) -shared -o $(BUILDPATH)/libzt.so obj/*.o

# dynamic library for use with Java JNI, scala, etc
#shared_jni_lib: lwip lwip_driver libzt_socket_layer jni_socket_wrapper utilities $(ZTO_OBJS)
#	@mkdir -p $(BUILDPATH) obj
#	mv *.o obj
	#$(CXX) $(CXXFLAGS) -shared -o $(BUILDPATH)/libzt.so obj/*.o
#	$(CXX) $(CXXFLAGS) -dynamiclib obj/*.o -o $(BUILDPATH)/libzt.dylib -lpthread

# static library for use with Java JNI, scala, etc
#static_jni_lib: lwip lwip_driver libzt_socket_layer jni_socket_wrapper utilities $(ZTO_OBJS)
#	@mkdir -p $(BUILDPATH) obj
#	mv *.o obj
#	$(ARTOOL) $(ARFLAGS) -o $(BUILDPATH)/$(STATIC_LIB) obj/*.o

# static library
#static_lib: lwip lwip_driver libzt_socket_layer utilities $(ZTO_OBJS)
#	@mkdir -p $(BUILDPATH) obj
#	mv *.o obj
#	mv ext/picotcp/build/lib/*.o obj
#	mv ext/picotcp/build/modules/*.o obj
#	$(ARTOOL) $(ARFLAGS) -o $(BUILDPATH)/$(STATIC_LIB) obj/*.o

##############################################################################
## iOS/macOS App Frameworks                                                 ##
##############################################################################

ios_app_framework:
	cd examples/apple/ZeroTierSDK_Apple; xcodebuild -configuration Release \
		-scheme ZeroTierSDK_iOS build SYMROOT="../../../$(BUILDPATH)/ios_app_framework"
	cd examples/apple/ZeroTierSDK_Apple; xcodebuild -configuration Debug \
		-scheme ZeroTierSDK_iOS build SYMROOT="../../../$(BUILDPATH)/ios_app_framework"

macos_app_framework:
	cd examples/apple/ZeroTierSDK_Apple; xcodebuild -configuration Release \
		-scheme ZeroTierSDK_OSX build SYMROOT="../../../$(BUILDPATH)/macos_app_framework"
	cd examples/apple/ZeroTierSDK_Apple; xcodebuild -configuration Debug \
		-scheme ZeroTierSDK_OSX build SYMROOT="../../../$(BUILDPATH)/macos_app_framework"

##############################################################################
## Python module                                                            ##
##############################################################################

python_module:
	swig -cpperraswarn -python -c++ -o examples/python/libzt.cc examples/python/swig_libzt.i
	python examples/python/setup.py build_ext --inplace --swig-opts="-modern -I../../zto/include"

##############################################################################
## Unit Tests                                                               ##
##############################################################################

tests: selftest nativetest ztproxy ipv4simple ipv6simple ipv6adhoc
# intercept

ZT_UTILS:=zto/node/Utils.cpp -Izto/node

sample:
	$(CXX) $(CXXFLAGS) -D__SELFTEST__ $(STACK_DRIVER_DEFS) $(LIBZT_DEFS) \
		$(SANFLAGS) $(LIBZT_INCLUDES) $(ZT_INCLUDES) $(ZT_UTILS) test/sample.cpp -o \
		$(BUILDPATH)/sample -L$(LIBPATH) -lzt
selftest:
	$(CXX) $(CXXFLAGS) -D__SELFTEST__ $(STACK_DRIVER_DEFS) $(LIBZT_DEFS) \
		$(SANFLAGS) $(LIBZT_INCLUDES) $(ZT_INCLUDES) $(ZT_UTILS) test/selftest.cpp -o \
		$(BUILDPATH)/selftest -L$(LIBPATH) -lzt -lpthread
nativetest:
	$(CXX) $(CXXFLAGS) -D__NATIVETEST__ $(STACK_DRIVER_DEFS) $(SANFLAGS) \
		$(LIBZT_INCLUDES) $(ZT_INCLUDES) test/selftest.cpp -o $(BUILDPATH)/nativetest
ztproxy:
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(LIBZT_INCLUDES) $(LIBZT_DEFS) $(ZT_INCLUDES) \
		examples/apps/ztproxy/ztproxy.cpp -o $(BUILDPATH)/ztproxy $< -L$(LIBPATH) -lzt -lpthread $(WINDEFS)
intercept:
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(STACK_DRIVER_DEFS) $(LIBZT_INCLUDES) \
		$(ZT_INCLUDES) examples/intercept/intercept.cpp -D_GNU_SOURCE \
		-shared -o $(BUILDPATH)/intercept.so $< -ldl
ipv4simple:
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(LIBZT_INCLUDES) $(LIBZT_DEFS) \
		examples/bindings/cpp/ipv4simple/client.cpp -o $(BUILDPATH)/ipv4client -L$(LIBPATH) -lpthread -lzt $(WINDEFS)
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(LIBZT_INCLUDES) $(LIBZT_DEFS) \
		examples/bindings/cpp/ipv4simple/server.cpp -o $(BUILDPATH)/ipv4server -L$(LIBPATH) -lpthread -lzt $(WINDEFS)
ipv6simple:
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(LIBZT_INCLUDES) $(LIBZT_DEFS) \
		examples/bindings/cpp/ipv6simple/client.cpp -o $(BUILDPATH)/ipv6client -L$(LIBPATH) -lpthread -lzt $(WINDEFS)
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(LIBZT_INCLUDES) $(LIBZT_DEFS) \
		examples/bindings/cpp/ipv6simple/server.cpp -o $(BUILDPATH)/ipv6server -L$(LIBPATH) -lpthread -lzt $(WINDEFS)
ipv6adhoc:
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(LIBZT_INCLUDES) $(LIBZT_DEFS) \
		examples/bindings/cpp/ipv6adhoc/client.cpp -o $(BUILDPATH)/ipv6adhocclient -L$(LIBPATH) -lpthread -lzt $(WINDEFS)
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(LIBZT_INCLUDES) $(LIBZT_DEFS) \
		examples/bindings/cpp/ipv6adhoc/server.cpp -o $(BUILDPATH)/ipv6adhocserver -L$(LIBPATH) -lpthread -lzt $(WINDEFS)
dlltest:
	$(CXX) $(CXXFLAGS)

##############################################################################
## Installation and Uninstallation                                          ##
##############################################################################

.PHONY: install
install:
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	mkdir -p $(DESTDIR)$(PREFIX)/include
	cp $(BUILDPATH)/$(STATIC_LIB) $(DESTDIR)$(PREFIX)/lib/
	cp include/libzt.h $(DESTDIR)$(PREFIX)/include/

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/*.a
	rm -f $(DESTDIR)$(PREFIX)/include/*.h

##############################################################################
## Misc                                                                     ##
##############################################################################

.PHONY: clean
clean:
	-rm f $(LIBPATH)/*
	-rm -rf $(BUILDPATH)/*
	-rm f obj/*
	-rm f *.o *.s *.exp *.lib .depend* *.core core
	-rm -rf .depend
	-find . -type f \( -name '*.a' -o -name '*.o' -o -name '*.so' -o -name \
		'*.o.d' -o -name '*.out' -o -name '*.log' -o -name '*.dSYM' \) -delete	

