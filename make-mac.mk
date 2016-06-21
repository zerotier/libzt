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
OBJS+=osdep/OSXEthernetTap.o

# Disable codesign since open source users will not have ZeroTier's certs
CODESIGN=echo
PRODUCTSIGN=echo
CODESIGN_APP_CERT=
CODESIGN_INSTALLER_CERT=

# Build with libminiupnpc by default for Mac
ZT_USE_MINIUPNPC?=1

# For internal use only -- signs everything with ZeroTier's developer cert
ifeq ($(ZT_OFFICIAL_RELEASE),1)
	DEFS+=-DZT_OFFICIAL_RELEASE -DZT_AUTO_UPDATE
	ZT_USE_MINIUPNPC=1
	CODESIGN=codesign
	PRODUCTSIGN=productsign
	CODESIGN_APP_CERT="Developer ID Application: ZeroTier Networks LLC (8ZD9JUCZ4V)"
	CODESIGN_INSTALLER_CERT="Developer ID Installer: ZeroTier Networks LLC (8ZD9JUCZ4V)"
endif

# Build with ZT_ENABLE_CLUSTER=1 to build with cluster support
ifeq ($(ZT_ENABLE_CLUSTER),1)
	DEFS+=-DZT_ENABLE_CLUSTER
endif

ifeq ($(ZT_AUTO_UPDATE),1)
	DEFS+=-DZT_AUTO_UPDATE
endif

ifeq ($(ZT_USE_MINIUPNPC),1)
	DEFS+=-DMACOSX -DZT_USE_MINIUPNPC -DMINIUPNP_STATICLIB -D_DARWIN_C_SOURCE -DMINIUPNPC_SET_SOCKET_TIMEOUT -DMINIUPNPC_GET_SRC_ADDR -D_BSD_SOURCE -D_DEFAULT_SOURCE -DOS_STRING=\"Darwin/15.0.0\" -DMINIUPNPC_VERSION_STRING=\"1.9\" -DUPNP_VERSION_STRING=\"UPnP/1.1\" -DENABLE_STRNATPMPERR
	OBJS+=ext/libnatpmp/natpmp.o ext/libnatpmp/getgateway.o ext/miniupnpc/connecthostport.o ext/miniupnpc/igd_desc_parse.o ext/miniupnpc/minisoap.o ext/miniupnpc/minissdpc.o ext/miniupnpc/miniupnpc.o ext/miniupnpc/miniwget.o ext/miniupnpc/minixml.o ext/miniupnpc/portlistingparse.o ext/miniupnpc/receivedata.o ext/miniupnpc/upnpcommands.o ext/miniupnpc/upnpdev.o ext/miniupnpc/upnperrors.o ext/miniupnpc/upnpreplyparse.o osdep/PortMapper.o
endif

# Build with ZT_ENABLE_NETWORK_CONTROLLER=1 to build with the Sqlite network controller
ifeq ($(ZT_ENABLE_NETWORK_CONTROLLER),1)
	DEFS+=-DZT_ENABLE_NETWORK_CONTROLLER
	LIBS+=-L/usr/local/lib -lsqlite3
	OBJS+=controller/SqliteNetworkController.o
endif

# Debug mode -- dump trace output, build binary with -g
ifeq ($(ZT_DEBUG),1)
	DEFS+=-DZT_TRACE
	CFLAGS+=-Wall -g -pthread $(INCLUDES) $(DEFS)
	STRIP=echo
	# The following line enables optimization for the crypto code, since
	# C25519 in particular is almost UNUSABLE in heavy testing without it.
ext/lz4/lz4.o node/Salsa20.o node/SHA512.o node/C25519.o node/Poly1305.o: CFLAGS = -Wall -O2 -g -pthread $(INCLUDES) $(DEFS)
else
	CFLAGS?=-Ofast -fstack-protector
	CFLAGS+=$(ARCH_FLAGS) -Wall -flto -fPIE -pthread -mmacosx-version-min=10.7 -DNDEBUG -Wno-unused-private-field $(INCLUDES) $(DEFS)
	STRIP=strip
endif

# Debug output for Network Containers 
# Specific levels can be controlled in netcon/common.inc.c
ifeq ($(SDK_DEBUG),1)
	DEFS+=-DSDK_DEBUG
endif

CXXFLAGS=$(CFLAGS) -fno-rtti

# Build frameworks for application development
# Build bundles for Unity integrations
all: 
	cd integrations/Apple/ZeroTierSDK_Apple; xcodebuild -scheme ZeroTierSDK_OSX build SYMROOT="../../../build/OSX_Framework"
	cd integrations/Apple/ZeroTierSDK_Apple; xcodebuild -scheme ZeroTierSDK_iOS build SYMROOT="../../../build/iOS_Framework"
	cd integrations/Apple/ZeroTierSDK_Apple; xcodebuild -scheme ZeroTierSDK_Unity3D_OSX build SYMROOT="../../../build/OSX_Unity_Bundle"
	cd integrations/Apple/ZeroTierSDK_Apple; xcodebuild -scheme ZeroTierSDK_Unity3D_iOS build SYMROOT="../../../build/iOS_Unity_Bundle"

sdk: $(OBJS)
	rm -f *.o
	# Need to selectively rebuild one.cpp and OneService.cpp with ZT_SERVICE_NETCON and ZT_ONE_NO_ROOT_CHECK defined, and also NetconEthernetTap
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DZT_SDK -DZT_ONE_NO_ROOT_CHECK -Iext/lwip/src/include -Iext/lwip/src/include/ipv4 -Iext/lwip/src/include/ipv6 -o zerotier-netcon-service $(OBJS) service/OneService.cpp netcon/NetconEthernetTap.cpp netcon/NetconProxy.cpp one.cpp -x c netcon/NetconRPC.c $(LDLIBS) -ldl
	# Build netcon/liblwip.so which must be placed in ZT home for zerotier-netcon-service to work
	cd netcon ; make -f make-liblwip.mk
	# Use gcc not clang to build standalone intercept library since gcc is typically used for libc and we want to ensure maximal ABI compatibility
	#cd netcon ; gcc $(DEFS) -O2 -Wall -std=c99 -fPIC -fno-common -dynamiclib -flat_namespace -DVERBOSE -D_GNU_SOURCE -DNETCON_INTERCEPT -I. -nostdlib -shared -o libztapi.so zt_api.c common.c RPC.c -ldl
	cd netcon ; gcc $(DEFS) -O2 -Wall -std=c99 -fPIC -fno-common -dynamiclib -flat_namespace -DVERBOSE -D_GNU_SOURCE -DNETCON_INTERCEPT -I. -nostdlib -shared -o libztintercept.so NetconSockets.c Intercept.c NetconDebug.c NetconRPC.c -ldl
	#cd netcon ; gcc $(DEFS) -O2 -Wall -std=c99 -fPIC -fno-common -dynamiclib -flat_namespace -DVERBOSE -D_GNU_SOURCE -DNETCON_INTERCEPT -I. -nostdlib -shared -o libztkq.so zt_api.c kq.c Intercept.c common.c RPC.c -ldl
	cp netcon/libztintercept.so libztintercept.so
	#cp netcon/libztapi.so libztapi.so
	#cp netcon/libztkq.so libztkq.so
	ln -sf zerotier-netcon-service zerotier-cli
	ln -sf zerotier-netcon-service zerotier-idtool

selftest: $(OBJS) selftest.o
	$(CXX) $(CXXFLAGS) -o zerotier-selftest selftest.o $(OBJS) $(LIBS)
	$(STRIP) zerotier-selftest

clean:
	rm -rf build
	# Remove junk generated by Android builds
	rm -rf netcon/Android/*.o.d
	rm -rf netcon/Netcon-Android/proj/app/build/intermediates
	rm -rf netcon/*.o netcon/*.so *.dSYM build-* *.pkg *.dmg *.o node/*.o controller/*.o service/*.o osdep/*.o ext/http-parser/*.o ext/lz4/*.o ext/json-parser/*.o $(OBJS) netcon/zerotier-intercept zerotier-netcon-service ztproxy libztapi.so libztkq.so libztintercept.so zerotier-one zerotier-idtool zerotier-selftest zerotier-cli ZeroTierOneInstaller-* mkworld

FORCE:
