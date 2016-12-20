LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ZTSDK := $(ZT1)/..
ZT := $(ZTSDK)/zerotierone
PICO := $(ZTSDK)/ext/picotcp

LOCAL_MODULE := ZeroTierOneJNI

LOCAL_C_INCLUDES := $(PICO)/modules
LOCAL_C_INCLUDES += $(PICO)/include
LOCAL_C_INCLUDES += $(PICO)/build/include
LOCAL_C_INCLUDES += $(PICO)/build/include/arch

LOCAL_C_INCLUDES += $(ZTSDK)/src
LOCAL_C_INCLUDES += $(ZTSDK)/src/stack_drivers/picotcp
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

# picoTCP files
LOCAL_SRC_FILES += \
	$(PICO)/stack/pico_device.c \
	$(PICO)/stack/pico_frame.c \
	$(PICO)/stack/pico_md5.c \
	$(PICO)/stack/pico_protocol.c \
	$(PICO)/stack/pico_socket_multicast.c \
	$(PICO)/stack/pico_socket.c \
	$(PICO)/stack/pico_stack.c \
	$(PICO)/stack/pico_tree.c \

# picoTCP files
LOCAL_SRC_FILES += \
	$(PICO)/modules/pico_aodv.c \
	$(PICO)/modules/pico_arp.c \
	$(PICO)/modules/pico_dev_loop.c \
	$(PICO)/modules/pico_dev_mock.c \
	$(PICO)/modules/pico_dev_null.c \
	$(PICO)/modules/pico_dev_tun.c \
	$(PICO)/modules/pico_dns_client.c \
	$(PICO)/modules/pico_dns_common.c \
	$(PICO)/modules/pico_dns_sd.c \
	$(PICO)/modules/pico_fragments.c \
	$(PICO)/modules/pico_hotplug_detection.c \
	$(PICO)/modules/pico_icmp4.c \
	$(PICO)/modules/pico_icmp6.c \
	$(PICO)/modules/pico_igmp.c \
	$(PICO)/modules/pico_ipfilter.c \
	$(PICO)/modules/pico_ipv4.c \
	$(PICO)/modules/pico_ipv6_nd.c \
	$(PICO)/modules/pico_ipv6.c \
	$(PICO)/modules/pico_mdns.c \
	$(PICO)/modules/pico_mld.c \
	$(PICO)/modules/pico_mm.c \
	$(PICO)/modules/pico_nat.c \
	$(PICO)/modules/pico_olsr.c \
	$(PICO)/modules/pico_posix.c \
	$(PICO)/modules/pico_slaacv4.c \
	$(PICO)/modules/pico_sntp_client.c \
	$(PICO)/modules/pico_socket_tcp.c \
	$(PICO)/modules/pico_socket_udp.c \
	$(PICO)/modules/pico_strings.c \
	$(PICO)/modules/pico_tcp.c \
	$(PICO)/modules/pico_tftp.c \
	$(PICO)/modules/pico_udp.c

# Netcon files
LOCAL_SRC_FILES += \
	$(ZTSDK)/src/rpc.c \
	$(ZTSDK)/src/proxy.cpp \
	$(ZTSDK)/src/sockets.c \
	$(ZTSDK)/src/service.cpp \
	$(ZTSDK)/src/tap.cpp \
	$(ZTSDK)/src/stack_drivers/picotcp/picotcp.cpp


include $(BUILD_SHARED_LIBRARY)
