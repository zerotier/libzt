#
# Makefile for ZeroTier SDK on OSX
#
# Targets
#   all: build every target possible on host system, plus tests
#   check: reports OK/FAIL of built targets
#   tests: build only test applications for host system
#   clean: removes all built files, objects, other trash

ifeq ($(origin CC),default)
	CC=$(shell if [ -e /usr/bin/clang ]; then echo clang; else echo gcc; fi)
endif
ifeq ($(origin CXX),default)
	CXX=$(shell if [ -e /usr/bin/clang++ ]; then echo clang++; else echo g++; fi)
endif

INCLUDES=
DEFS=
LIBS=
ARCH_FLAGS=-arch x86_64

include objects.mk

# Disable codesign since open source users will not have ZeroTier's certs
CODESIGN=echo
PRODUCTSIGN=echo
CODESIGN_APP_CERT=
CODESIGN_INSTALLER_CERT=

# Debug output for ZeroTier service
ifeq ($(ZT_DEBUG),1)
	DEFS+=-DZT_TRACE
	CFLAGS+=-Wall -g -pthread $(INCLUDES) $(DEFS)
	STRIP=echo
	# The following line enables optimization for the crypto code, since
	# C25519 in particular is almost UNUSABLE in heavy testing without it.
#ext/lz4/lz4.o node/Salsa20.o node/SHA512.o node/C25519.o node/Poly1305.o: CFLAGS = -Wall -O2 -g -pthread $(INCLUDES) $(DEFS)
else
	CFLAGS?=-Ofast -fstack-protector
	CFLAGS+=$(ARCH_FLAGS) -Wall -flto -fPIE -pthread -mmacosx-version-min=10.7 -DNDEBUG -Wno-unused-private-field $(INCLUDES) $(DEFS)
	STRIP=strip
endif

# Debug output for the SDK
# Specific levels can be controlled in src/SDK_Debug.h
ifeq ($(SDK_DEBUG),1)
	DEFS+=-DSDK_DEBUG -g
endif

CXXFLAGS=$(CFLAGS) -fno-rtti

# Build everything
all: osx ios android check

# Build all iOS targets
ios: ios_app_framework ios_unity3d_bundle

# Build all OSX targets
osx: osx_app_framework osx_unity3d_bundle osx_shared_lib

# Build all Android targets 
# Chip architectures can be specified in integrations/android/android_jni_lib/java/jni/Application.mk
android: android_jni_lib

# TODO: CHECK if XCODE TOOLS are installed
# Build frameworks for application development
osx_app_framework:
	cd $(INT)/apple/ZeroTierSDK_Apple; xcodebuild -scheme ZeroTierSDK_OSX build SYMROOT="../../../$(BUILD)/osx_app_framework"
	cp docs/osx_zt_sdk.md $(BUILD)/osx_app_framework/README.md
ios_app_framework:
	cd $(INT)/apple/ZeroTierSDK_Apple; xcodebuild -scheme ZeroTierSDK_iOS build SYMROOT="../../../$(BUILD)/ios_app_framework"
	cp docs/ios_zt_sdk.md $(BUILD)/ios_app_framework/README.md
# Build bundles for Unity integrations
osx_unity3d_bundle:
	cd $(INT)/apple/ZeroTierSDK_Apple; xcodebuild -scheme ZeroTierSDK_Unity3D_OSX build SYMROOT="../../../$(BUILD)/osx_unity3d_bundle"
	cp docs/osx_unity3d_zt_sdk.md $(BUILD)/osx_unity3d_bundle/README.md
ios_unity3d_bundle:
	cd $(INT)/apple/ZeroTierSDK_Apple; xcodebuild -scheme ZeroTierSDK_Unity3D_iOS build SYMROOT="../../../$(BUILD)/ios_unity3d_bundle"
	cp docs/ios_unity3d_zt_sdk.md $(BUILD)/ios_unity3d_bundle/README.md
# TODO: CHECK if ANDROID/GRADLE TOOLS are installed
# Build library for Android Unity integrations
# Build JNI library for Android app integration
android_jni_lib:
	cd $(INT)/android/android_jni_lib/proj; ./gradlew assembleDebug
	# copy binary into example android project dir
	# mv $(INT)/android/android_jni_lib/java/libs/* build
	mv $(INT)/android/android_jni_lib/java/libs/* $(INT)/android/example_app/app/src/main/jniLibs
	# cd build; for res_f in *; do mv "$res_f" "android_jni_lib_$res_f"; done
	# cp docs/android_zt_sdk.md $(BUILD)/README.md

remove_only_intermediates:
	-find . -type f \( -name '*.o' -o -name '*.so' \) -delete

osx_shared_lib: remove_only_intermediates $(OBJS)
	mkdir -p $(BUILD)/osx_shared_lib
	# Need to selectively rebuild one.cpp and OneService.cpp with ZT_SERVICE_NETCON and ZT_ONE_NO_ROOT_CHECK defined, and also NetconEthernetTap
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DSDK -DZT_ONE_NO_ROOT_CHECK -Iext/lwip/src/include -Iext/lwip/src/include/ipv4 -Iext/lwip/src/include/ipv6 -Izerotierone/osdep -Izerotierone/node -Isrc -o $(BUILD)/zerotier-sdk-service $(OBJS) zerotierone/service/OneService.cpp src/SDK_EthernetTap.cpp src/SDK_Proxy.cpp zerotierone/one.cpp -x c src/SDK_RPC.c $(LDLIBS) -ldl
	# Build liblwip.so which must be placed in ZT home for zerotier-sdk-service to work
	make -f make-liblwip.mk
	# Use gcc not clang to build standalone intercept library since gcc is typically used for libc and we want to ensure maximal ABI compatibility
	cd src ; gcc $(DEFS) -O2 -Wall -std=c99 -fPIC -fno-common -dynamiclib -flat_namespace -DVERBOSE -D_GNU_SOURCE -DNETCON_INTERCEPT -I. -I../zerotierone/node -nostdlib -shared -o libztintercept.so SDK_Sockets.c SDK_Intercept.c SDK_Debug.c SDK_RPC.c -ldl
	mv src/libztintercept.so $(BUILD)/osx_shared_lib/libztintercept.so
	ln -sf zerotier-sdk-service zerotier-cli
	ln -sf zerotier-sdk-service zerotier-idtool
	cp docs/osx_zt_sdk.md $(BUILD)/osx_shared_lib/README.md


prep:
	cp $(INT)/android/android_jni_lib/java/libs/* build

# Check for the presence of built frameworks/bundles/libaries
check:
	./check.sh $(BUILD)/lwip/liblwip.so

	./check.sh $(BUILD)/osx_shared_lib/libztintercept.so
	./check.sh $(BUILD)/osx_unity3d_bundle/Debug/ZeroTierSDK_Unity3D_OSX.bundle
	./check.sh $(BUILD)/osx_app_framework/Debug/ZeroTierSDK_OSX.framework

	./check.sh $(BUILD)/ios_app_framework/Debug-iphoneos/ZeroTierSDK_iOS.framework
	./check.sh $(BUILD)/ios_unity3d_bundle/Debug-iphoneos/ZeroTierSDK_Unity3D_iOS.bundle

	./check.sh $(BUILD)/
	./check.sh $(BUILD)/android_jni_lib/arm64-v8a/libZeroTierJNI.so
	./check.sh $(BUILD)/android_jni_lib/armeabi/libZeroTierJNI.so
	./check.sh $(BUILD)/android_jni_lib/armeabi-v7a/libZeroTierJNI.so
	./check.sh $(BUILD)/android_jni_lib/mips/libZeroTierJNI.so
	./check.sh $(BUILD)/android_jni_lib/mips64/libZeroTierJNI.so
	./check.sh $(BUILD)/android_jni_lib/x86/libZeroTierJNI.so
	./check.sh $(BUILD)/android_jni_lib/x86_64/libZeroTierJNI.so

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
	-rm -rf zerotier-cli zerotier-idtool
	-rm -rf $(BUILD)/*
	-find . -type f \( -name '*.o' -o -name '*.so' -o -name '*.o.d' -o -name '*.out' -o -name '*.log' \) -delete
	# android JNI lib project
	-cd $(INT)/android/android_jni_lib/proj; ./gradlew clean
	-rm -rf $(INT)/android/android_jni_lib/proj/.gradle
	-rm -rf $(INT)/android/android_jni_lib/proj/.idea
	-rm -rf $(INT)/android/android_jni_lib/proj/build

	# example android app project
	-cd $(INT)/android/example_app; ./gradlew clean
	-rm -rf $(INT)/android/example_app/.idea
	-rm -rf $(INT)/android/example_app/.gradle
