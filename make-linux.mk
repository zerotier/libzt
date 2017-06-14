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

##############################################################################
## VARIABLES                                                                ##
##############################################################################

include objects.mk

OSTYPE = $(shell uname -s | tr '[A-Z]' '[a-z]')

# Target output filenames
STATIC_LIB_NAME    = libzt.a
PICO_LIB_NAME      = libpicotcp.a
JNI_LIB_NAME       = libzt.jnilib
#
STATIC_LIB         = $(BUILD)/$(STATIC_LIB_NAME)
PICO_DIR           = ext/picotcp
PICO_LIB           = $(PICO_DIR)/build/lib/$(PICO_LIB_NAME)
SHARED_JNI_LIB     = $(BUILD)/$(JNI_LIB_NAME)
#
TEST_BUILD_DIR     = $(BUILD)
UNIT_TEST_SRC_DIR  = test

##############################################################################
## General Configuration                                                    ##
##############################################################################

# Debug output for ZeroTier service
ifeq ($(ZT_DEBUG),1)
	DEFS+=-DZT_TRACE
	CFLAGS+=-Wall -g -pthread $(INCLUDES) $(DEFS)
	STRIP=echo
	# The following line enables optimization for the crypto code, since
	# C25519 in particular is almost UNUSABLE in heavy testing without it.
#ext/lz4/lz4.o node/Salsa20.o node/SHA512.o node/C25519.o node/Poly1305.o: CFLAGS = -Wall -O2 -g -pthread $(INCLUDES) $(DEFS)
else
	CFLAGS?=-O2 -fstack-protector
	CFLAGS+=-Wall -fPIE -fvisibility=hidden -pthread $(INCLUDES) $(DEFS)
	#CFLAGS+=$(ARCH_FLAGS) -Wall -flto -fPIC -pthread -mmacosx-version-min=10.7 -DNDEBUG -Wno-unused-private-field $(INCLUDES) $(DEFS)
	STRIP=strip
endif

CXXFLAGS=$(CFLAGS) -Wno-format -fno-rtti -std=c++11

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

COMMON_LIBS = -lpthread

##############################################################################
## User Build Flags                                                         ##
##############################################################################

CXXFLAGS+=-DZT_SDK

# Debug option, prints filenames, lines, functions, arguments, etc
# Also enables debug symbols for debugging with tools like gdb, etc
ifeq ($(SDK_DEBUG),1)
	CXXFLAGS+=-DSDK_PICOTCP
	CXXFLAGS+=-g
	INCLUDES+= -I$(PICOTCP_DIR)/include \
		-I$(PICOTCP_DIR)/build/include \
		-Isrc/stack_drivers/picotcp
endif

# JNI (Java Native Interface)
ifeq ($(SDK_JNI), 1)
	# jni.h
	INCLUDES+=-I$(shell /usr/libexec/java_home)/include 
	# jni_md.h
	INCLUDES+=-I$(shell /usr/libexec/java_home)/include/$(SYSTEM)
	CXXFLAGS+=-DSDK_JNI
endif

##############################################################################
## Stack Configuration                                                      ##
##############################################################################

# Stack config flags
ifeq ($(SDK_PICOTCP),1)
	CXXFLAGS+=-DSDK_PICOTCP
	INCLUDES+= -I$(PICOTCP_DIR)/include \
		-I$(PICOTCP_DIR)/build/include \
		-Isrc/stack_drivers/picotcp
endif

CXXFLAGS+=-DSDK_IPV4
CXXFLAGS+=-DSDK_IPV6

##############################################################################
## Files                                                                    ##
##############################################################################

STACK_DRIVER_FILES:=src/picoTCP.cpp
TAP_FILES:=src/SocketTap.cpp \
	src/libzt.cpp \
	src/Utilities.cpp

SDK_OBJS+= SocketTap.o \
	picoTCP.o \
	libzt.o \
	Utilities.o

PICO_OBJS+= ext/picotcp/build/lib/pico_device.o \
	ext/picotcp/build/lib/pico_frame.o \
	ext/picotcp/build/lib/pico_md5.o \
	ext/picotcp/build/lib/pico_protocol.o \
	ext/picotcp/build/lib/pico_socket_multicast.o \
	ext/picotcp/build/lib/pico_socket.o \
	ext/picotcp/build/lib/pico_stack.o \
	ext/picotcp/build/lib/pico_tree.o

all: 

tests: unit_tests

##############################################################################
## User-Space Stack                                                         ##
##############################################################################

picotcp:
	cd $(PICO_DIR); make lib ARCH=shared IPV4=1 IPV6=1

##############################################################################
## Static Libraries                                                         ##
##############################################################################

static_lib: picotcp $(ZTO_OBJS)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(TAP_FILES) $(STACK_DRIVER_FILES) -c -DSDK_STATIC
	ar rcs -o $(STATIC_LIB) ext/picotcp/build/modules/*.o $(PICO_OBJS) $(ZTO_OBJS) $(SDK_OBJS) 

##############################################################################
## Java JNI                                                                 ##
##############################################################################

shared_jni_lib: picotcp $(ZTO_OBJS)
	$(CXX) $(CXXFLAGS) $(TAP_FILES) $(STACK_DRIVER_FILES) $(ZTO_OBJS) $(INCLUDES) $(PICO_LIB) -dynamiclib -o $(SHARED_JNI_LIB)

##############################################################################
## Unit Tests                                                               ##
##############################################################################

UNIT_TEST_SRC_FILES := $(wildcard  $(UNIT_TEST_SRC_DIR)/*.cpp)
UNIT_TEST_OBJ_FILES := $(addprefix $(TEST_BUILD_DIR)/,$(notdir $(UNIT_TEST_SRC_FILES:.cpp=)))
UNIT_TEST_INCLUDES  := -Iinclude
UNIT_TEST_LIBS      := -L$(BUILD) -lzt $(COMMON_LIBS)

$(TEST_BUILD_DIR)/%: $(UNIT_TEST_SRC_DIR)/%.cpp
	@mkdir -p $(TEST_BUILD_DIR)
	@-$(CXX) $(UNIT_TEST_INCLUDES) -o $@ $< $(UNIT_TEST_LIBS)
	@-./check.sh $@

unit_tests: $(UNIT_TEST_OBJ_FILES)

##############################################################################
## Misc                                                                     ##
##############################################################################

# Cleans only current $(OSTYPE)
clean:
	-rm -rf $(BUILD)/*
	-find $(PICO_DIR) -type f \( -name '*.o' -o -name '*.so' -o -name '*.a' \) -delete
	-find . -type f \( -name '*.o' -o -name '*.so' -o -name '*.o.d' -o -name '*.out' -o -name '*.log' -o -name '*.dSYM' \) -delete

# Clean everything
nuke:
	-rm -rf $(BUILD)/*
	-find . -type f \( -name '*.o' -o -name '*.so' -o -name '*.a' -o -name '*.o.d' -o -name '*.out' -o -name '*.log' -o -name '*.dSYM' \) -delete

check:
	-./check.sh $(PICO_LIB)
	-./check.sh $(STATIC_LIB)

	