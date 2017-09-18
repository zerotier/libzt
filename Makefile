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

# Darwin
ifeq ($(OSTYPE),darwin)
ARTOOL=libtool
ARFLAGS=-static
endif
# Linux
ifeq ($(OSTYPE),linux)
ARTOOL=ar
ARFLAGS=rcs
endif
# FreeBSD
ifeq ($(OSTYPE),freebsd)
ARTOOL=ar
ARFLAGS=rcs
endif
# OpenBSD
ifeq ($(OSTYPE),openbsd)
ARTOOL=ar
ARFLAGS=rcs
endif

##############################################################################
## Objects and includes                                                     ##
##############################################################################

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
	zto/controller/JSONDB.o \
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

ifeq ($(ZT_DEBUG),1)
	ZT_FLAGS+=-DZT_TRACE
	CFLAGS+=-Wall -g -pthread
	STRIP=echo
else
	CFLAGS?=-Ofast -g -fstack-protector
	CFLAGS+=-Wall -fPIE -fvisibility=hidden -pthread
	STRIP=strip
endif
ifeq ($(LIBZT_DEBUG),1)
	LIBZT_FLAGS+=-DLIBZT_DEBUG
endif
ifeq ($(NS_DEBUG),1)
	# specified in stack configuration section
endif
# Build with address sanitization library for advanced debugging (clang)
# TODO: Add GCC version as well
ifeq ($(LIBZT_SANITIZE),1)
	SANFLAGS+=-x c++ -O -g -fsanitize=address -DASAN_OPTIONS=symbolize=1 \
		-DASAN_SYMBOLIZER_PATH=$(shell which llvm-symbolizer)
endif

# JNI (Java Native Interface)
ifeq ($(SDK_JNI), 1)
	# jni.h
	INCLUDES+=-I$(shell /usr/libexec/java_home)/include 
	# jni_md.h
	INCLUDES+=-I$(shell /usr/libexec/java_home)/include/$(SYSTEM)
	CXXFLAGS+=-DSDK_JNI
endif

CXXFLAGS=$(CFLAGS) -Wno-format -fno-rtti -std=c++11
ZT_FLAGS+=-DZT_SDK -DZT_SOFTWARE_UPDATE_DEFAULT="\"disable\""
LIBZT_FLAGS+=
LIBZT_FILES:=src/VirtualTap.cpp src/libzt.cpp src/Utilities.cpp
STATIC_LIB=$(BUILD)/libzt.a

##############################################################################
## Stack Configuration                                                      ##
##############################################################################

# default stack (picoTCP)
STACK_PICO=1
ifeq ($(NO_STACK)$(STACK_LWIP),1)
STACK_PICO=0
endif

# picoTCP
ifeq ($(STACK_PICO),1)
# picoTCP default protocol versions
ifeq ($(NS_DEBUG),1)
	STACK_FLAGS+=
endif
ifeq ($(LIBZT_IPV4)$(LIBZT_IPV6),1)
ifeq ($(LIBZT_IPV4),1)
STACK_DRIVER_FLAGS+=-DLIBZT_IPV4
STACK_FLAGS+=IPV4=1
endif
ifeq ($(LIBZT_IPV6),1)
STACK_DRIVER_FLAGS+=-DLIBZT_IPV6
STACK_FLAGS+=IPV6=1
endif
else
STACK_DRIVER_FLAGS+=-DLIBZT_IPV4 -DLIBZT_IPV6
STACK_FLAGS+=IPV6=1 IPV4=1
endif
STACK_DRIVER_FLAGS+=-DSTACK_PICO
STACK_LIB:=libpicotcp.a
STACK_DIR:=ext/picotcp
STACK_LIB:=$(STACK_DIR)/build/lib/$(STACK_LIB)
STACK_DRIVER_FILES:=src/picoTCP.cpp
STACK_INCLUDES+=-Iext/picotcp/include -Iext/picotcp/build/include
endif

# lwIP
ifeq ($(STACK_LWIP),1)
# lwIP default protocol versions
ifeq ($(NS_DEBUG),1)
	STACK_FLAGS+=LWIP_DEBUG=1
endif
ifeq ($(LIBZT_IPV4)$(LIBZT_IPV6),1)
ifeq ($(LIBZT_IPV4),1)
STACK_DRIVER_FLAGS+=-DLIBZT_IPV4 -DLWIP_IPV4=1
STACK_FLAGS+=LIBZT_IPV4=1
endif
ifeq ($(LIBZT_IPV6),1)
STACK_DRIVER_FLAGS+=-DLIBZT_IPV6 -DLWIP_IPV6=1
STACK_FLAGS+=LIBZT_IPV6=1
endif
else
STACK_DRIVER_FLAGS+=-DLIBZT_IPV4 -DLWIP_IPV4=1
STACK_FLAGS+=LIBZT_IPV4=1
endif
STACK_DRIVER_FLAGS+=-DLWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS
STACK_DRIVER_FLAGS+=-DSTACK_LWIP
STACK_DRIVER_FILES:=src/lwIP.cpp
LWIPARCH=$(CONTRIBDIR)/ports/unix
LWIPDIR=ext/lwip/src
STACK_INCLUDES+=-Iext/lwip/src/include/lwip \
	-I$(LWIPDIR)/include \
	-I$(LWIPARCH)/include \
	-I$(LWIPDIR)/include/ipv4 \
	-I$(LWIPDIR) \
	-Iext
endif

ifeq ($(NO_STACK),1)
STACK_DRIVER_FLAGS+=-DNO_STACK
endif

##############################################################################
## Targets                                                                  ##
##############################################################################

%.o : %.cpp
	@mkdir -p $(BUILD) obj
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(STACK_DRIVER_FLAGS) $(ZT_FLAGS) \
		$(ZT_INCLUDES) $(STACK_INCLUDES) $(LIBZT_INCLUDES) -c $^ -o obj/$(@F)

%.o : %.c
	@mkdir -p $(BUILD) obj
	$(CC) $(CFLAGS) -c $^ -o obj/$(@F)

picotcp:
	cd ext/picotcp; make lib ARCH=shared IPV4=1 IPV6=1

lwip:
	make -f make-liblwip.mk liblwip.a $(STACK_FLAGS)

ifeq ($(STACK_PICO),1)
static_lib: picotcp $(ZTO_OBJS)
	@mkdir -p $(BUILD) obj
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(ZT_FLAGS) $(ZT_INCLUDES) $(LIBZT_FLAGS) \
		$(LIBZT_INCLUDES) $(STACK_INCLUDES) $(STACK_DRIVER_FLAGS) $(LIBZT_FILES) \
		$(STACK_DRIVER_FILES) -c 
	mv *.o obj
	mv ext/picotcp/build/lib/*.o obj
	mv ext/picotcp/build/modules/*.o obj
	$(ARTOOL) $(ARFLAGS) -o $(STATIC_LIB) obj/*.o
endif
ifeq ($(STACK_LWIP),1)
static_lib: lwip $(ZTO_OBJS)
	@mkdir -p $(BUILD) obj
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(ZT_FLAGS) $(ZT_INCLUDES) $(LIBZT_FLAGS) \
		$(LIBZT_INCLUDES) $(STACK_INCLUDES) $(STACK_DRIVER_FLAGS) $(LIBZT_FILES) \
		$(STACK_DRIVER_FILES) -c  
	mv *.o obj
	$(ARTOOL) $(ARFLAGS) -o $(STATIC_LIB) $(STACK_LIB) obj/*.o 
endif
# for layer-2 only (this will omit all userspace network stack code)
ifeq ($(NO_STACK),1)
static_lib: $(ZTO_OBJS)
	@mkdir -p $(BUILD) obj
	$(CXX) $(CXXFLAGS) $(LIBZT_FILES) -c
	mv *.o obj
	$(ARTOOL) $(ARFLAGS) -o $(STATIC_LIB) obj/*.o
endif

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
## Java JNI                                                                 ##
##############################################################################

shared_jni_lib: picotcp $(ZTO_OBJS)
	$(CXX) $(CXXFLAGS) $(TAP_FILES) $(STACK_DRIVER_FILES) $(ZTO_OBJS) $(INCLUDES) \
		$(PICO_LIB) -dynamiclib -o $(SHARED_JNI_LIB)

##############################################################################
## Unit Tests                                                               ##
##############################################################################

tests: selftest nativetest ztproxy
# intercept

selftest:
	$(CXX) $(CXXFLAGS) -D__SELFTEST__ $(STACK_DRIVER_FLAGS) $(LIBZT_FLAGS) \
		$(SANFLAGS) $(LIBZT_INCLUDES) $(ZT_INCLUDES) test/selftest.cpp -o \
		$(BUILD)/selftest -L$(BUILD) -lzt -lpthread
	@./check.sh $(BUILD)/selftest
nativetest:
	$(CXX) $(CXXFLAGS) -D__NATIVETEST__ $(STACK_DRIVER_FLAGS) $(SANFLAGS) \
		$(LIBZT_INCLUDES) $(ZT_INCLUDES) test/selftest.cpp -o $(BUILD)/nativetest
	@./check.sh $(BUILD)/nativetest
ztproxy:
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(LIBZT_INCLUDES) $(ZT_INCLUDES) \
		examples/ztproxy/ztproxy.cpp -o $(BUILD)/ztproxy $< -L$(BUILD) -lzt
	@./check.sh $(BUILD)/ztproxy
intercept:
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(STACK_DRIVER_FLAGS) $(LIBZT_INCLUDES) \
		$(ZT_INCLUDES) examples/intercept/intercept.cpp -D_GNU_SOURCE \
		-shared -o $(BUILD)/intercept.so $< -ldl
	@./check.sh $(BUILD)/intercept.so

##############################################################################
## Misc                                                                     ##
##############################################################################

standardize:
	vera++ --transform trim_right src/*.cpp
	vera++ --transform trim_right src/*.hpp
	vera++ --transform trim_right include/*.h
	vera++ --transform trim_right include/*.hpp

clean:
	-rm -rf .depend
	-rm -f *.o *.s .depend* *.core core
	-rm -rf $(BUILD)/*
	-find . -type f \( -name '*.a' -o -name '*.o' -o -name '*.so' -o -name \
		'*.o.d' -o -name '*.out' -o -name '*.log' -o -name '*.dSYM' \) -delete

