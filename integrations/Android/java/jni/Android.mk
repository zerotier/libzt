LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := ZeroTierOneJNI
LOCAL_C_INCLUDES := $(ZT1)/include
LOCAL_C_INCLUDES += $(ZT1)/ext/lwip/src/include
LOCAL_C_INCLUDES += $(ZT1)/ext/lwip/src/include/ipv4
LOCAL_C_INCLUDES += $(ZT1)/netcon
LOCAL_C_INCLUDES += $(ZT1)/service
LOCAL_C_INCLUDES += $(ZT1)/osdep
LOCAL_C_INCLUDES += $(ZT1)/node

LOCAL_LDLIBS := -llog
# LOCAL_CFLAGS := -g

# Netcon files
LOCAL_SRC_FILES := \
	$(ZT1)/netcon/NetconRPC.c \
	$(ZT1)/netcon/NetconProxy.cpp \
	$(ZT1)/netcon/NetconServiceSetup.cpp \
	$(ZT1)/netcon/NetconEthernetTap.cpp

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
	$(ZT1)/node/Network.cpp \
	$(ZT1)/node/NetworkConfig.cpp \
	$(ZT1)/node/Node.cpp \
	$(ZT1)/node/OutboundMulticast.cpp \
	$(ZT1)/node/Packet.cpp \
	$(ZT1)/node/Path.cpp \
	$(ZT1)/node/Peer.cpp \
	$(ZT1)/node/Poly1305.cpp \
	$(ZT1)/node/Salsa20.cpp \
	$(ZT1)/node/SelfAwareness.cpp \
	$(ZT1)/node/SHA512.cpp \
	$(ZT1)/node/Switch.cpp \
	$(ZT1)/node/Topology.cpp \
	$(ZT1)/node/Utils.cpp \
	$(ZT1)/osdep/Http.cpp \
	$(ZT1)/osdep/OSUtils.cpp \
	$(ZT1)/osdep/BackgroundResolver.cpp

# lwIP api files
LOCAL_SRC_FILES += \
	$(ZT1)/ext/lwip/src/api/api_lib.c \
	$(ZT1)/ext/lwip/src/api/api_msg.c \
	$(ZT1)/ext/lwip/src/api/err.c \
	$(ZT1)/ext/lwip/src/api/netbuf.c \
	$(ZT1)/ext/lwip/src/api/netdb.c \
	$(ZT1)/ext/lwip/src/api/netifapi.c \
	$(ZT1)/ext/lwip/src/api/sockets.c \
	$(ZT1)/ext/lwip/src/api/tcpip.c

# lwIP core files
LOCAL_SRC_FILES += \
	$(ZT1)/ext/lwip/src/core/def.c \
	$(ZT1)/ext/lwip/src/core/dhcp.c \
	$(ZT1)/ext/lwip/src/core/dns.c \
	$(ZT1)/ext/lwip/src/core/init.c \
	$(ZT1)/ext/lwip/src/core/mem.c \
	$(ZT1)/ext/lwip/src/core/memp.c \
	$(ZT1)/ext/lwip/src/core/netif.c \
	$(ZT1)/ext/lwip/src/core/pbuf.c \
	$(ZT1)/ext/lwip/src/core/raw.c \
	$(ZT1)/ext/lwip/src/core/stats.c \
	$(ZT1)/ext/lwip/src/core/sys.c \
	$(ZT1)/ext/lwip/src/core/tcp_in.c \
	$(ZT1)/ext/lwip/src/core/tcp_out.c \
	$(ZT1)/ext/lwip/src/core/tcp.c \
	$(ZT1)/ext/lwip/src/core/timers.c \
	$(ZT1)/ext/lwip/src/core/udp.c

# lwIP core/ip4 files
LOCAL_SRC_FILES += \
	$(ZT1)/ext/lwip/src/core/ipv4/autoip.c \
	$(ZT1)/ext/lwip/src/core/ipv4/icmp.c \
	$(ZT1)/ext/lwip/src/core/ipv4/igmp.c \
	$(ZT1)/ext/lwip/src/core/ipv4/inet_chksum.c \
	$(ZT1)/ext/lwip/src/core/ipv4/inet.c \
	$(ZT1)/ext/lwip/src/core/ipv4/ip_addr.c \
	$(ZT1)/ext/lwip/src/core/ipv4/ip_frag.c \
	$(ZT1)/ext/lwip/src/core/ipv4/ip.c \

# lwIP netif files
LOCAL_SRC_FILES += \
	$(ZT1)/ext/lwip/src/netif/etharp.c \
	$(ZT1)/ext/lwip/src/netif/ethernetif.c \
	$(ZT1)/ext/lwip/src/netif/slipif.c

# JNI Files
LOCAL_SRC_FILES += \
	com_zerotierone_sdk_Node.cpp \
	ZT_jniutils.cpp \
	ZT_jnilookup.cpp

include $(BUILD_SHARED_LIBRARY)
