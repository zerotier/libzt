LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ZTSDK := $(ZT1)/..
ZT := $(ZTSDK)/zerotierone
LWIP := $(ZTSDK)/ext/lwip/src

LOCAL_MODULE := ZeroTierOneJNI

LOCAL_C_INCLUDES := $(LWIP)/include
LOCAL_C_INCLUDES += $(LWIP)/include/lwip
LOCAL_C_INCLUDES += $(LWIP)/include/lwip/priv

LOCAL_C_INCLUDES += $(ZTSDK)
LOCAL_C_INCLUDES += $(ZTSDK)/src
LOCAL_C_INCLUDES += $(ZTSDK)/src/stack_drivers/lwip
LOCAL_C_INCLUDES += $(ZTSDK)/src/stack_drivers

LOCAL_C_INCLUDES += $(ZT1)/include
LOCAL_C_INCLUDES += $(ZT1)/node
LOCAL_C_INCLUDES += $(ZT1)/
LOCAL_C_INCLUDES += $(ZT1)/service
LOCAL_C_INCLUDES += $(ZT1)/osdep

LOCAL_LDLIBS := -llog
# LOCAL_CFLAGS := -g

# ZeroTierOne ext files
LOCAL_SRC_FILES := \
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
	$(ZT)/osdep/ManagedRoute.cpp \
	$(ZT)/osdep/BackgroundResolver.cpp


#lwip
LOCAL_SRC_FILES += $(LWIP)/core/init.c \
	$(LWIP)/core/def.c \
	$(LWIP)/core/dns.c \
	$(LWIP)/core/inet_chksum.c \
	$(LWIP)/core/ip.c \
	$(LWIP)/core/mem.c \
	$(LWIP)/core/memp.c \
	$(LWIP)/core/netif.c \
	$(LWIP)/core/pbuf.c \
	$(LWIP)/core/raw.c \
	$(LWIP)/core/stats.c \
	$(LWIP)/core/sys.c \
	$(LWIP)/core/tcp.c \
	$(LWIP)/core/tcp_in.c \
	$(LWIP)/core/tcp_out.c \
	$(LWIP)/core/timeouts.c \
	$(LWIP)/core/udp.c

LOCAL_SRC_FILES += $(LWIP)/core/ipv4/autoip.c \
	$(LWIP)/core/ipv4/dhcp.c \
	$(LWIP)/core/ipv4/etharp.c \
	$(LWIP)/core/ipv4/icmp.c \
	$(LWIP)/core/ipv4/igmp.c \
	$(LWIP)/core/ipv4/ip4_frag.c \
	$(LWIP)/core/ipv4/ip4.c \
	$(LWIP)/core/ipv4/ip4_addr.c

#LOCAL_SRC_FILES += $(LWIP)/core/ipv6/dhcp6.c \
#	$(LWIP)/core/ipv6/ethip6.c \
#	$(LWIP)/core/ipv6/icmp6.c \
#	$(LWIP)/core/ipv6/inet6.c \
#	$(LWIP)/core/ipv6/ip6.c \
#	$(LWIP)/core/ipv6/ip6_addr.c \
#	$(LWIP)/core/ipv6/ip6_frag.c \
#	$(LWIP)/core/ipv6/mld6.c \
#	$(LWIP)/core/ipv6/nd6.c

# lwIP netif files
LOCAL_SRC_FILES += \
	$(LWIP)/netif/ethernetif.c \
	$(LWIP)/netif/ethernet.c 

# Netcon files
LOCAL_SRC_FILES += \
	$(ZTSDK)/src/rpc.c \
	$(ZTSDK)/src/proxy.cpp \
	$(ZTSDK)/src/sockets.c \
	$(ZTSDK)/src/service.cpp \
	$(ZTSDK)/src/tap.cpp \
	$(ZTSDK)/src/stack_drivers/lwip/lwip.cpp

include $(BUILD_SHARED_LIBRARY)