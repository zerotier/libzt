LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ZTSDK := ../../../../src
ZT := ../../../../zerotierone

LOCAL_MODULE := ZeroTierOneJNI
LOCAL_C_INCLUDES := $(ZT1)/include
LOCAL_C_INCLUDES += $(ZT1)/ext/lwip/src/include
LOCAL_C_INCLUDES += $(ZT1)/ext/lwip/src/include/ipv4
LOCAL_C_INCLUDES += $(ZT1)/node
LOCAL_C_INCLUDES += $(ZT1)/
LOCAL_C_INCLUDES += $(ZT1)/service
LOCAL_C_INCLUDES += $(ZT1)/osdep
LOCAL_C_INCLUDES += $(ZTSDK)/src

LOCAL_LDLIBS := -llog
# LOCAL_CFLAGS := -g

# Netcon files
LOCAL_SRC_FILES := \
	$(ZTSDK)/SDK_RPC.c \
	$(ZTSDK)/SDK_Proxy.cpp \
	$(ZTSDK)/SDK_ServiceSetup.cpp \
	$(ZTSDK)/SDK_EthernetTap.cpp

# ZeroTierOne ext files
LOCAL_SRC_FILES += \
	$(ZT1)/ext/lz4/lz4.c \
	$(ZT1)/ext/json-parser/json.c \
	$(ZT1)/ext/http-parser/http_parser.c \

# ZeroTierOne files
LOCAL_SRC_FILES += \
	$(ZT1)/service/OneService.cpp \
	$(ZT1)/service/ControlPlane.cpp \
	$(ZT1)/node/C25519.cpp \
	$(ZT1)/node/CertificateOfMembership.cpp \
	$(ZT1)/node/DeferredPackets.cpp \
	$(ZT1)/node/Dictionary.cpp \
	$(ZT1)/node/Identity.cpp \
	$(ZT1)/node/IncomingPacket.cpp \
	$(ZT1)/node/InetAddress.cpp \
	$(ZT1)/node/Multicaster.cpp \
	$(ZT)/node/Network.cpp \
	$(ZT)/node/NetworkConfig.cpp \
	$(ZT)/node/Node.cpp \
	$(ZT)/node/OutboundMulticast.cpp \
	$(ZT)/node/Packet.cpp \
	$(ZT)/node/Path.cpp \
	$(ZT)/node/Peer.cpp \
	$(ZT)/node/Poly1305.cpp \
	$(ZT)/node/Salsa20.cpp \
	$(ZT)/node/SelfAwareness.cpp \
	$(ZT)/node/SHA512.cpp \
	$(ZT)/node/Switch.cpp \
	$(ZT)/node/Topology.cpp \
	$(ZT)/node/Utils.cpp \
	$(ZT)/osdep/Http.cpp \
	$(ZT)/osdep/OSUtils.cpp \
	$(ZT)/osdep/BackgroundResolver.cpp

# lwIP api files
LOCAL_SRC_FILES += \
	$(ZT)/ext/lwip/src/api/api_lib.c \
	$(ZT)/ext/lwip/src/api/api_msg.c \
	$(ZT)/ext/lwip/src/api/err.c \
	$(ZT)/ext/lwip/src/api/netbuf.c \
	$(ZT)/ext/lwip/src/api/netdb.c \
	$(ZT)/ext/lwip/src/api/netifapi.c \
	$(ZT)/ext/lwip/src/api/sockets.c \
	$(ZT)/ext/lwip/src/api/tcpip.c

# lwIP core files
LOCAL_SRC_FILES += \
	$(ZT)/ext/lwip/src/core/def.c \
	$(ZT)/ext/lwip/src/core/dhcp.c \
	$(ZT)/ext/lwip/src/core/dns.c \
	$(ZT)/ext/lwip/src/core/init.c \
	$(ZT)/ext/lwip/src/core/mem.c \
	$(ZT)/ext/lwip/src/core/memp.c \
	$(ZT)/ext/lwip/src/core/netif.c \
	$(ZT)/ext/lwip/src/core/pbuf.c \
	$(ZT)/ext/lwip/src/core/raw.c \
	$(ZT)/ext/lwip/src/core/stats.c \
	$(ZT)/ext/lwip/src/core/sys.c \
	$(ZT)/ext/lwip/src/core/tcp_in.c \
	$(ZT)/ext/lwip/src/core/tcp_out.c \
	$(ZT)/ext/lwip/src/core/tcp.c \
	$(ZT)/ext/lwip/src/core/timers.c \
	$(ZT)/ext/lwip/src/core/udp.c

# lwIP core/ip4 files
LOCAL_SRC_FILES += \
	$(ZT)/ext/lwip/src/core/ipv4/autoip.c \
	$(ZT)/ext/lwip/src/core/ipv4/icmp.c \
	$(ZT)/ext/lwip/src/core/ipv4/igmp.c \
	$(ZT)/ext/lwip/src/core/ipv4/inet_chksum.c \
	$(ZT)/ext/lwip/src/core/ipv4/inet.c \
	$(ZT)/ext/lwip/src/core/ipv4/ip_addr.c \
	$(ZT)/ext/lwip/src/core/ipv4/ip_frag.c \
	$(ZT)/ext/lwip/src/core/ipv4/ip.c \

# lwIP netif files
LOCAL_SRC_FILES += \
	$(ZT)/ext/lwip/src/netif/etharp.c \
	$(ZT)/ext/lwip/src/netif/ethernetif.c \
	$(ZT)/ext/lwip/src/netif/slipif.c

# JNI Files
LOCAL_SRC_FILES += \
	com_zerotierone_sdk_Node.cpp \
	ZT_jniutils.cpp \
	ZT_jnilookup.cpp

include $(BUILD_SHARED_LIBRARY)
