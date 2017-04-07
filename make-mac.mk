
# Automagically pick clang or gcc, with preference for clang
# This is only done if we have not overridden these with an environment or CLI variable
ifeq ($(origin CC),default)
	CC=$(shell if [ -e /usr/bin/clang ]; then echo clang; else echo gcc; fi)
endif
ifeq ($(origin CXX),default)
	CXX=$(shell if [ -e /usr/bin/clang++ ]; then echo clang++; else echo g++; fi)
endif

##############################################################################
## General Configuration                                                    ##
##############################################################################

include objects.mk

# Target output filenames
STATIC_LIB_NAME    = libzt.a
INTERCEPT_NAME     = libztintercept.so
SDK_SERVICE_NAME   = zerotier-sdk-service
ONE_SERVICE_NAME   = zerotier-one
PICO_LIB_NAME      = libpicotcp.a
#
STATIC_LIB         = $(BUILD)/$(STATIC_LIB_NAME)
SDK_INTERCEPT      = $(BUILD)/$(INTERCEPT_NAME)
SDK_SERVICE        = $(BUILD)/$(SDK_SERVICE_NAME)
ONE_SERVICE        = $(BUILD)/$(ONE_SERVICE_NAME)
PICO_LIB           = ext/picotcp/build/lib/$(PICO_LIB_NAME)

# Debug output for ZeroTier service
ifeq ($(ZT_DEBUG),1)
	DEFS+=-DZT_TRACE
	#CFLAGS+=-Wall -fPIE -fvisibility=hidden -pthread $(INCLUDES) $(DEFS)
	CFLAGS+=-Wall -g -pthread $(INCLUDES) $(DEFS)
	STRIP=echo
	# The following line enables optimization for the crypto code, since
	# C25519 in particular is almost UNUSABLE in heavy testing without it.
#ext/lz4/lz4.o node/Salsa20.o node/SHA512.o node/C25519.o node/Poly1305.o: CFLAGS = -Wall -O2 -g -pthread $(INCLUDES) $(DEFS)
else
	CFLAGS?=-Ofast -fstack-protector
	CFLAGS+=-Wall -fPIE -fvisibility=hidden -pthread $(INCLUDES) $(DEFS)
	#CFLAGS+=$(ARCH_FLAGS) -Wall -flto -fPIC -pthread -mmacosx-version-min=10.7 -DNDEBUG -Wno-unused-private-field $(INCLUDES) $(DEFS)
	STRIP=strip
endif

CXXFLAGS=$(CFLAGS) -Wno-format -fno-rtti -std=c++11 -DZT_SDK

INCLUDES+= -Iext \
	-I$(ZTO)/osdep \
	-I$(ZTO)/node \
	-I$(ZTO)/service \
	-I$(ZTO)/include \
	-I../$(ZTO)/osdep \
	-I../$(ZTO)/node \
	-I../$(ZTO)/service \
	-I. \
	-Isrc \
	-Iinclude \
	-Iext/picotcp/include \
	-Iext/picotcp/build/include

##############################################################################
## User Build Flags                                                         ##
##############################################################################

# Debug option, prints filenames, lines, functions, arguments, etc
# Also enables debug symbols for debugging with tools like gdb, etc
ifeq ($(SDK_DEBUG),1)
	SDK_FLAGS+=-DSDK_PICOTCP
	CXXFLAGS+=-g
	INCLUDES+= -I$(PICOTCP_DIR)/include \
		-I$(PICOTCP_DIR)/build/include \
		-Isrc/stack_drivers/picotcp
endif

##############################################################################
## Stack Configuration                                                      ##
##############################################################################

# Stack config flags
ifeq ($(SDK_PICOTCP),1)
	SDK_FLAGS+=-DSDK_PICOTCP
	INCLUDES+= -I$(PICOTCP_DIR)/include \
		-I$(PICOTCP_DIR)/build/include \
		-Isrc/stack_drivers/picotcp
endif
ifeq ($(SDK_IPV4),1)
	SDK_FLAGS+=-DSDK_IPV4
endif
ifeq ($(SDK_IPV6),1)
	SDK_FLAGS+=-DSDK_IPV6
endif


##############################################################################
## Files                                                                    ##
##############################################################################

STACK_DRIVER_FILES:=src/picoTCP.cpp
TAP_FILES:=src/SocketTap.cpp \
			src/SDKService.cpp

RPC_FILES:=src/RPC.c
SOCKET_API_FILES:=src/Socket.c

SDK_OBJS+= RPC.o \
			SocketTap.o \
			Socket.o \
			picoTCP.o \
			SDKService.o

PICO_OBJS+= ext/picotcp/build/lib/pico_device.o \
ext/picotcp/build/lib/pico_frame.o \
ext/picotcp/build/lib/pico_md5.o \
ext/picotcp/build/lib/pico_protocol.o \
ext/picotcp/build/lib/pico_socket_multicast.o \
ext/picotcp/build/lib/pico_socket.o \
ext/picotcp/build/lib/pico_stack.o \
ext/picotcp/build/lib/pico_tree.o

all: 

##############################################################################
## User-Space Stack                                                         ##
##############################################################################

picotcp:
	cd ext/picotcp; make lib ARCH=shared IPV4=1 IPV6=1

##############################################################################
## Static Libraries                                                         ##
##############################################################################

static_lib: picotcp $(ZTO_OBJS)
	$(CXX) $(CXXFLAGS) $(SDK_FLAGS) $(RPC_FILES) $(TAP_FILES) $(STACK_DRIVER_FILES) $(SOCKET_API_FILES) -c
	libtool -static -o $(STATIC_LIB) $(ZTO_OBJS) $(SDK_OBJS) $(PICO_LIB)

jni_static_lib: picotcp $(ZTO_OBJS)

##############################################################################
## Unit Tests                                                               ##
##############################################################################

static_lib_test:
	$(CXX) -Iinclude tests/socket/simple.cpp -o build/simple.out -Lbuild -lzt

##############################################################################
## Misc                                                                     ##
##############################################################################

clean:
	-rm -rf $(BUILD)/*
	-rm -rf zerotier-cli zerotier-idtool
	-find . -type f \( -name $(ONE_SERVICE_NAME) -o -name $(SDK_SERVICE_NAME) \) -delete
	-find . -type f \( -name '*.o' -o -name '*.so' -o -name '*.o.d' -o -name '*.out' -o -name '*.log' -o -name '*.dSYM' \) -delete

# Check for the presence of built frameworks/bundles/libaries
check:
	-./check.sh $(PICO_LIB)
	-./check.sh $(SDK_INTERCEPT)
	-./check.sh $(ONE_SERVICE)
	-./check.sh $(SDK_SERVICE)
	-./check.sh $(STATIC_LIB)
	

#osx_static_lib: pico $(ZTO_OBJS)
#	$(CXX) $(CXXFLAGS) $(STACK_FLAGS) $(DEFS) $(INCLUDES) $(ZTFLAGS) -DSDK_SERVICE -DSDK -DSDK_BUNDLED $(PICO_DRIVER_FILES) $(SDK_INTERCEPT_C_FILES) $(SDK_SERVICE_CPP_FILES) src/service.cpp -c 
#	libtool -static -o build/libzt.a picotcp.o proxy.o tap.o one.o OneService.o service.o sockets.o rpc.o intercept.o $(ZTO_OBJS)