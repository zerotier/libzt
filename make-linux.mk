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
JNI_LIB_NAME       = libzt.jnilib
STATIC_LIB         = $(BUILD)/$(STATIC_LIB_NAME)
SHARED_JNI_LIB     = $(BUILD)/$(JNI_LIB_NAME)
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
	CFLAGS?=-Ofast -g -fstack-protector
	CFLAGS+=-Wall -fPIE -fvisibility=hidden -pthread $(INCLUDES) $(DEFS)
	#CFLAGS+=$(ARCH_FLAGS) -Wall -flto -fPIC -pthread -mmacosx-version-min=10.7 -DNDEBUG -Wno-unused-private-field $(INCLUDES) $(DEFS)
	STRIP=strip
endif

CXXFLAGS=$(CFLAGS) -Wno-format -fno-rtti -std=c++11 -DZT_SOFTWARE_UPDATE_DEFAULT="\"disable\""

INCLUDES+= -Iext \
	-I$(ZTO)/osdep \
	-I$(ZTO)/node \
	-I$(ZTO)/service \
	-I$(ZTO)/include \
	-I$(ZTO)/controller \
	-I../$(ZTO)/osdep \
	-I../$(ZTO)/node \
	-I../$(ZTO)/service \
	-I. \
	-Isrc \
	-Iinclude \

COMMON_LIBS = -lpthread

##############################################################################
## User Build Flags                                                         ##
##############################################################################

CXXFLAGS+=-DZT_SDK

# Debug option, prints filenames, lines, functions, arguments, etc
# Also enables debug symbols for debugging with tools like gdb, etc
ifeq ($(SDK_DEBUG),1)
	CXXFLAGS+=-g
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

# default stack (picoTCP)
STACK_PICO=1
ifeq ($(NO_STACK)$(STACK_LWIP),1)
STACK_PICO=0
endif

# picoTCP default protocol versions
ifeq ($(STACK_PICO),1)
ifeq ($(LIBZT_IPV4)$(LIBZT_IPV6),1)
ifeq ($(LIBZT_IPV4),1)
CXXFLAGS+=-DLIBZT_IPV4
STACK_FLAGS+=IPV4=1
endif
ifeq ($(LIBZT_IPV6),1)
CXXFLAGS+=-DLIBZT_IPV6
STACK_FLAGS+=IPV6=1
endif
else
CXXFLAGS+=-DLIBZT_IPV4 -DLIBZT_IPV6
STACK_FLAGS+=IPV6=1 IPV4=1
endif
endif

# lwIP default protocol versions
ifeq ($(STACK_LWIP),1)
ifeq ($(LIBZT_IPV4)$(LIBZT_IPV6),1)
ifeq ($(LIBZT_IPV4),1)
CXXFLAGS+=-DLIBZT_IPV4 -DLWIP_IPV4=1
STACK_FLAGS+=LIBZT_IPV4=1
endif
ifeq ($(LIBZT_IPV6),1)
CXXFLAGS+=-DLIBZT_IPV6 -DLWIP_IPV6=1
STACK_FLAGS+=LIBZT_IPV6=1
endif
else
CXXFLAGS+=-DLIBZT_IPV4 -DLWIP_IPV4=1
STACK_FLAGS+=LIBZT_IPV4=1
endif
endif

LIBZT_FILES:=src/SocketTap.cpp src/libzt.cpp src/Utilities.cpp

ifeq ($(STACK_PICO),1)
CXXFLAGS+=-DSTACK_PICO
STACK_LIB:=libpicotcp.a
STACK_DIR:=ext/picotcp
STACK_LIB:=$(STACK_DIR)/build/lib/$(STACK_LIB)
STACK_DRIVER_FILES:=src/picoTCP.cpp
INCLUDES+=-Iext/picotcp/include -Iext/picotcp/build/include
endif

ifeq ($(STACK_LWIP),1)
STACK_DRIVER_FLAGS+=-DLWIP_PREFIX_BYTEORDER_FUNCS
CXXFLAGS+=-DSTACK_LWIP
STACK_DRIVER_FILES:=src/lwIP.cpp
LWIPARCH=$(CONTRIBDIR)/ports/unix
LWIPDIR=ext/lwip/src
INCLUDES+=-Iext/lwip/src/include/lwip \
	-I$(LWIPDIR)/include \
	-I$(LWIPARCH)/include \
	-I$(LWIPDIR)/include/ipv4 \
	-I$(LWIPDIR) \
	-Iext
endif

ifeq ($(NO_STACK),1)
CXXFLAGS+=-DNO_STACK
endif

all: 

%.o : %.cpp
	@mkdir -p $(BUILD) obj
	$(CXX) $(CXXFLAGS) -c $^ -o obj/$(@F)

%.o : %.c
	@mkdir -p $(BUILD) obj
	$(CC) $(CFLAGS) -c $^ -o obj/$(@F)

##############################################################################
## User-Space Stack                                                         ##
##############################################################################

picotcp:
	cd $(STACK_DIR); make lib ARCH=shared IPV4=1 IPV6=1

lwip:
	echo $(STACK_FLAGSs)
	-make -f make-liblwip.mk liblwip.a $(STACK_FLAGS)

##############################################################################
## Static Libraries                                                         ##
##############################################################################

ifeq ($(STACK_PICO),1)
static_lib: picotcp $(ZTO_OBJS)
	@mkdir -p $(BUILD) obj
	$(CXX) $(CXXFLAGS) $(LIBZT_FILES) $(STACK_DRIVER_FILES) -c
	mv *.o obj
	mv ext/picotcp/build/lib/*.o obj
	mv ext/picotcp/build/modules/*.o obj
	ar rcs -o $(STATIC_LIB) obj/*.o $(STACK_LIB)
endif
ifeq ($(STACK_LWIP),1)
static_lib: lwip $(ZTO_OBJS)
	@mkdir -p $(BUILD) obj
	$(CXX) $(CXXFLAGS) $(STACK_DRIVER_FLAGS) $(STACK_INCLUDES) $(LIBZT_FILES) $(STACK_DRIVER_FILES) -c 
	mv *.o obj
	ar rcs -o $(STATIC_LIB) obj/*.o $(STACK_LIB)
endif
# for layer-2 only (this will omit all userspace network stack code)
ifeq ($(NO_STACK),1)
static_lib: $(ZTO_OBJS)
	@mkdir -p $(BUILD) obj
	$(CXX) $(CXXFLAGS) $(LIBZT_FILES) -c
	mv *.o obj
	ar rcs -o $(STATIC_LIB) obj/*.o
endif

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
	@-$(CXX) $(CXXFLAGS) $(UNIT_TEST_INCLUDES) $(INCLUDES) -o $@ $< $(UNIT_TEST_LIBS)
	@-./check.sh $@

tests: $(UNIT_TEST_OBJ_FILES)

##############################################################################
## Misc                                                                     ##
##############################################################################

clean:
	-rm -rf $(BUILD)/*
	-find . -type f \( -name '*.a' -o -name '*.o' -o -name '*.so' -o -name '*.o.d' -o -name '*.out' -o -name '*.log' -o -name '*.dSYM' \) -delete

check:
	-./check.sh $(PICO_LIB)
	-./check.sh $(STATIC_LIB)

	