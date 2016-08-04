#
# Makefile for ZeroTier SDK on Linux
#
# Targets
#   all: build every target possible on host system, plus tests
#   check: reports OK/FAIL of built targets
#   tests: build only test applications for host system
#   clean: removes all built files, objects, other trash

# Automagically pick clang or gcc, with preference for clang
# This is only done if we have not overridden these with an environment or CLI variable
ifeq ($(origin CC),default)
	CC=$(shell if [ -e /usr/bin/clang ]; then echo clang; else echo gcc; fi)
endif
ifeq ($(origin CXX),default)
	CXX=$(shell if [ -e /usr/bin/clang++ ]; then echo clang++; else echo g++; fi)
endif

#UNAME_M=$(shell $(CC) -dumpmachine | cut -d '-' -f 1)

INCLUDES?=
DEFS?=
LDLIBS?=

include objects.mk

ifeq ($(ZT_DEBUG),1)
	DEFS+=-DZT_TRACE
	CFLAGS+=-Wall -g -pthread $(INCLUDES) $(DEFS)
	CXXFLAGS+=-Wall -g -pthread $(INCLUDES) $(DEFS)
	LDFLAGS=-ldl
	STRIP?=echo
	# The following line enables optimization for the crypto code, since
	# C25519 in particular is almost UNUSABLE in -O0 even on a 3ghz box!
ext/lz4/lz4.o node/Salsa20.o node/SHA512.o node/C25519.o node/Poly1305.o: CFLAGS = -Wall -O2 -g -pthread $(INCLUDES) $(DEFS)
else
	CFLAGS?=-O3 -fstack-protector
	CFLAGS+=-Wall -fPIE -fvisibility=hidden -pthread $(INCLUDES) -DNDEBUG $(DEFS)
	CXXFLAGS?= -fstack-protector
	CXXFLAGS+=-Wall -Wreorder -fPIE -fvisibility=hidden -fno-rtti -pthread $(INCLUDES) -DNDEBUG $(DEFS)
	LDFLAGS=-ldl -pie -Wl,-z,relro,-z,now
	STRIP?=strip
	STRIP+=--strip-all
endif

# Debug output for ZeroTier service
ifeq ($(ZT_TRACE),1)
	DEFS+=-DZT_TRACE
endif

# Debug output for lwIP
ifeq ($(SDK_LWIP_DEBUG),1)
	LWIP_FLAGS:=SDK_LWIP_DEBUG=1
endif

# Debug output for the SDK
# Specific levels can be controlled in src/SDK_Debug.h
ifeq ($(SDK_DEBUG),1)
	DEFS+=-DSDK_DEBUG -g
endif
# Log debug chatter to file, path is determined by environment variable ZT_SDK_LOGFILE
ifeq ($(SDK_DEBUG_LOG_TO_FILE),1)
	DEFS+=-DSDK_DEBUG_LOG_TO_FILE
endif

all: remove_only_intermediates linux_shared_lib check

remove_only_intermediates:
	-find . -type f \( -name '*.o' -o -name '*.so' \) -delete

# TODO: CHECK if ANDROID/GRADLE TOOLS are installed
# Build library for Android Unity integrations
# Build JNI library for Android app integration
android_jni_lib:
	cd $(INT)/android/android_jni_lib/proj; ./gradlew assembleDebug
	mkdir -p $(BUILD)/android_jni_lib
	cp docs/android_zt_sdk.md $(BUILD)/android_jni_lib/README.md
	mv -f $(INT)/android/android_jni_lib/java/libs/* $(BUILD)/android_jni_lib
	cp -R $(BUILD)/android_jni_lib/* $(INT)/android/example_app/app/src/main/jniLibs
	
# Build a dynamically-loadable library
linux_shared_lib: $(OBJS)
	mkdir -p $(BUILD)/linux_shared_lib
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(DEFS) -DSDK -DZT_ONE_NO_ROOT_CHECK -Iext/lwip/src/include -Iext/lwip/src/include/ipv4 -Iext/lwip/src/include/ipv6 -I$(ZT1)/osdep -I$(ZT1)/node -Isrc -o $(BUILD)/zerotier-sdk-service $(OBJS) $(ZT1)/service/OneService.cpp src/SDK_EthernetTap.cpp src/SDK_Proxy.cpp $(ZT1)/one.cpp -x c src/SDK_RPC.c $(LDLIBS) -ldl
	# Build liblwip.so which must be placed in ZT home for zerotier-netcon-service to work
	make -f make-liblwip.mk $(LWIP_FLAGS)
	# Use gcc not clang to build standalone intercept library since gcc is typically used for libc and we want to ensure maximal ABI compatibility
	cd src ; gcc $(DEFS) -O2 -Wall -std=c99 -fPIC -DVERBOSE -D_GNU_SOURCE -DSDK_INTERCEPT -I. -I../$(ZT1)/node -nostdlib -shared -o ../$(BUILD)/linux_shared_lib/libztintercept.so SDK_Sockets.c SDK_Intercept.c SDK_Debug.c SDK_RPC.c -ldl
	ln -sf zerotier-sdk-service $(BUILD)/zerotier-cli
	ln -sf zerotier-sdk-service $(BUILD)/zerotier-idtool

# Build vanilla ZeroTier One binary
one: $(OBJS) $(ZT1)/service/OneService.o $(ZT1)/one.o $(ZT1)/osdep/LinuxEthernetTap.o
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(BUILD)/zerotier-one $(OBJS) $(ZT1)/service/OneService.o $(ZT1)/one.o $(ZT1)/osdep/LinuxEthernetTap.o $(LDLIBS)
	$(STRIP) $(BUILD)/zerotier-one
	cp $(BUILD)/zerotier-one $(INT)/docker/docker_demo/zerotier-one


# Build the docker demo images
docker_demo: one linux_shared_lib
	mkdir -p $(BUILD)
	# Intercept library
	cp $(BUILD)/linux_shared_lib/libztintercept.so $(INT)/docker/docker_demo/libztintercept.so
	# SDK service
	cp $(BUILD)/zerotier-sdk-service $(INT)/docker/docker_demo/zerotier-sdk-service
	# lwIP network stack library
	cp $(BUILD)/lwip/liblwip.so $(INT)/docker/docker_demo/liblwip.so
	# CLI interface for ZeroTier
	cp $(BUILD)/zerotier-cli $(INT)/docker/docker_demo/zerotier-cli
	touch $(INT)/docker/docker_demo/docker_demo.name
	# Server image
	# This image will contain the server application and everything required to 
	# run the ZeroTier SDK service
	cd $(INT)/docker/docker_demo; docker build --tag="docker_demo" -f sdk_dockerfile .
	# Client image
	# This image is merely a test image designed to interact with the server image
	# in order to verify it's working properly
	cd $(INT)/docker/docker_demo; docker build --tag="docker_demo_monitor" -f monitor_dockerfile .

# Builds all docker test images
docker_images: one linux_shared_lib
	./tests/docker/build_images.sh

# Runs docker container tests
docker_test:
	./tests/docker/test.sh

# Checks the results of the docker tests
docker_check_test:
	./tests/docker/check.sh

# Check for the presence of built frameworks/bundles/libaries
check:
	./check.sh $(BUILD)/lwip/liblwip.so
	./check.sh $(BUILD)/linux_shared_lib/libztintercept.so
	./check.sh $(BUILD)/android_jni_lib/arm64-v8a/libZeroTierOneJNI.so
	./check.sh $(BUILD)/android_jni_lib/armeabi/libZeroTierOneJNI.so
	./check.sh $(BUILD)/android_jni_lib/armeabi-v7a/libZeroTierOneJNI.so
	./check.sh $(BUILD)/android_jni_lib/mips/libZeroTierOneJNI.so
	./check.sh $(BUILD)/android_jni_lib/mips64/libZeroTierOneJNI.so
	./check.sh $(BUILD)/android_jni_lib/x86/libZeroTierOneJNI.so
	./check.sh $(BUILD)/android_jni_lib/x86_64/libZeroTierOneJNI.so

# Tests
TEST_OBJDIR := $(BUILD)/tests
TEST_SOURCES := $(wildcard tests/*.c)
TEST_TARGETS := $(addprefix $(BUILD)/tests/$(OSTYPE).,$(notdir $(TEST_SOURCES:.c=.out)))

$(BUILD)/tests/$(OSTYPE).%.out: tests/%.c
	-$(CC) $(CC_FLAGS) -o $@ $<

$(TEST_OBJDIR):
	mkdir -p $(TEST_OBJDIR)

tests: $(TEST_OBJDIR) $(TEST_TARGETS)
	mkdir -p $(BUILD)/tests; 

clean:
	-rm -rf $(BUILD)/*
	-rm -rf $(INT)/Unity3D/Assets/Plugins/*
	-rm -rf zerotier-cli zerotier-idtool
	-find . -type f \( -name 'zerotier-one' -o -name 'zerotier-sdk-service' \) -delete
	-find . -type f \( -name '*.o' -o -name '*.so' -o -name '*.o.d' -o -name '*.out' -o -name '*.log' \) -delete
	# Remove junk generated by Android builds
	-cd $(INT)/android/android_jni_lib/proj; ./gradlew clean
	-rm -rf $(INT)/android/android_jni_lib/proj/build
