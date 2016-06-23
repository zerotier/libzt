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

# For internal use only -- signs everything with ZeroTier's developer cert
ifeq ($(ZT_OFFICIAL_RELEASE),1)
	DEFS+=-DZT_OFFICIAL_RELEASE -DZT_AUTO_UPDATE
	ZT_USE_MINIUPNPC=1
	CODESIGN=codesign
	PRODUCTSIGN=productsign
	CODESIGN_APP_CERT="Developer ID Application: ZeroTier Networks LLC (8ZD9JUCZ4V)"
	CODESIGN_INSTALLER_CERT="Developer ID Installer: ZeroTier Networks LLC (8ZD9JUCZ4V)"
endif

# Debug mode -- dump trace output, build binary with -g
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

# Debug output for SDK
# Specific levels can be controlled in src/debug.h
ifeq ($(SDK_DEBUG),1)
	DEFS+=-DSDK_DEBUG
endif

CXXFLAGS=$(CFLAGS) -fno-rtti

all: osx_app_framework ios_app_framework osx_unity3d_bundle ios_unity3d_bundle android_jni_lib osx_shared_lib check

# TODO: CHECK if XCODE TOOLS are installed
# Build frameworks for application development
osx_app_framework:
	cd integrations/apple/ZeroTierSDK_Apple; xcodebuild -scheme ZeroTierSDK_OSX build SYMROOT="../../../build/osx_app_framework"
	cp docs/osx_zt_sdk.md build/osx_app_framework/README.md
ios_app_framework:
	cd integrations/apple/ZeroTierSDK_Apple; xcodebuild -scheme ZeroTierSDK_iOS build SYMROOT="../../../build/ios_app_framework"
	cp docs/ios_zt_sdk.md build/ios_app_framework/README.md
# Build bundles for Unity integrations
osx_unity3d_bundle:
	cd integrations/apple/ZeroTierSDK_Apple; xcodebuild -scheme ZeroTierSDK_Unity3D_OSX build SYMROOT="../../../build/osx_unity3d_bundle"
	cp docs/osx_unity3d_zt_sdk.md build/osx_unity3d_bundle/README.md
ios_unity3d_bundle:
	cd integrations/apple/ZeroTierSDK_Apple; xcodebuild -scheme ZeroTierSDK_Unity3D_iOS build SYMROOT="../../../build/ios_unity3d_bundle"
	cp docs/ios_unity3d_zt_sdk.md build/ios_unity3d_bundle/README.md
# TODO: CHECK if ANDROID/GRADLE TOOLS are installed
# Build library for Android Unity integrations
# Build JNI library for Android app integration
android_jni_lib:
	cd integrations/android/android_jni_lib/proj; ./gradlew assembleDebug
#	cd integrations/android/android_jni_lib/java/libs/; for f in *; do mv "$f" "android_jni_lib_$f"; done
	mv integrations/android/android_jni_lib/java/libs/* build
	cp docs/android_zt_sdk.md build/README.md

osx_shared_lib: $(OBJS)
	rm -f *.o
	# Need to selectively rebuild one.cpp and OneService.cpp with ZT_SERVICE_NETCON and ZT_ONE_NO_ROOT_CHECK defined, and also NetconEthernetTap
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DZT_SDK -DZT_ONE_NO_ROOT_CHECK -Iext/lwip/src/include -Iext/lwip/src/include/ipv4 -Iext/lwip/src/include/ipv6 -Izerotierone/osdep -Izerotierone/node -Isrc -o build/zerotier-sdk-service $(OBJS) zerotierone/service/OneService.cpp src/SDK_EthernetTap.cpp src/SDK_Proxy.cpp zerotierone/one.cpp -x c src/SDK_RPC.c $(LDLIBS) -ldl
	# Build liblwip.so which must be placed in ZT home for zerotier-sdk-service to work
	make -f make-liblwip.mk
	# Use gcc not clang to build standalone intercept library since gcc is typically used for libc and we want to ensure maximal ABI compatibility
	cd src ; gcc $(DEFS) -O2 -Wall -std=c99 -fPIC -fno-common -dynamiclib -flat_namespace -DVERBOSE -D_GNU_SOURCE -DNETCON_INTERCEPT -I. -I../zerotierone/node -nostdlib -shared -o libztintercept.so SDK_Sockets.c SDK_Intercept.c SDK_Debug.c SDK_RPC.c -ldl
	mkdir -p build/osx_shared_lib
	cp src/libztintercept.so build/osx_shared_lib/libztintercept.so
	ln -sf zerotier-sdk-service zerotier-cli
	ln -sf zerotier-sdk-service zerotier-idtool
	cp docs/osx_zt_sdk.md build/osx_shared_lib/README.md

check:
	./check.sh build/lwip/liblwip.so

	./check.sh build/osx_shared_lib/libztintercept.so
	./check.sh build/osx_unity3d_bundle/Debug/ZeroTierSDK_Unity3D_OSX.bundle
	./check.sh build/osx_app_framework/Debug/ZeroTierSDK_OSX.framework

	./check.sh build/ios_app_framework/Debug-iphoneos/ZeroTierSDK_iOS.framework
	./check.sh build/ios_unity3d_bundle/Debug-iphoneos/ZeroTierSDK_Unity3D_iOS.bundle

	./check.sh build/
	./check.sh build/android_jni_lib/
	./check.sh build/android_jni_lib/
	./check.sh build/android_jni_lib/
	./check.sh build/android_jni_lib/
	./check.sh build/android_jni_lib/
	./check.sh build/android_jni_lib/
	./check.sh build/android_jni_lib/
	./check.sh build/android_jni_lib/

	./check.sh build/
	./check.sh build/
	
clean:
	rm -rf zerotier-cli zerotier-idtool

	rm -rf build/*
	find . -type f -name '*.o' -delete
	find . -type f -name '*.so' -delete
	find . -type f -name '*.o.d' -delete

	# android JNI lib project
	cd integrations/android/android_jni_lib/proj; ./gradlew clean
	rm -rf integrations/android/android_jni_lib/proj/.gradle
	rm -rf integrations/android/android_jni_lib/proj/.idea
	rm -rf integrations/android/android_jni_lib/proj/build

	# example android app project
	cd integrations/android/example_app; ./gradlew clean
	rm -rf integrations/android/example_app/.idea
	rm -rf integrations/android/example_app/.gradle
FORCE:
