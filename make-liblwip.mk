#
# Copyright (c) 2001, 2002 Swedish Institute of Computer Science.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
# SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
# OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
# OF SUCH DAMAGE.
#
# This file is part of the lwIP TCP/IP stack.
#
# Author: Adam Dunkels <adam@sics.se>
#

CONTRIBDIR=ext/contrib
LWIPARCH=$(CONTRIBDIR)/ports/unix

#Set this to where you have the lwip core module checked out from CVS
#default assumes it's a dir named lwip at the same level as the contrib module
LWIPDIR=ext/lwip/src

CCDEP=clang++

# Automagically pick clang or gcc, with preference for clang
# This is only done if we have not overridden these with an environment or CLI variable
ifeq ($(origin CCX),default)
	CCX=$(shell if [ -e /usr/bin/clang++ ]; then echo clang++; else echo g++; fi)
endif

CFLAGS=-O3 -g -Wall -fPIC

CFLAGS:=$(CFLAGS) -I$(LWIPDIR)/include -I$(LWIPARCH)/include -I$(LWIPDIR) -I. -Iext -Iinclude

# COREFILES, CORE4FILES: The minimum set of files needed for lwIP.
COREFILES=$(LWIPDIR)/core/init.c \
	$(LWIPDIR)/core/def.c \
	$(LWIPDIR)/core/dns.c \
	$(LWIPDIR)/core/inet_chksum.c \
	$(LWIPDIR)/core/ip.c \
	$(LWIPDIR)/core/mem.c \
	$(LWIPDIR)/core/memp.c \
	$(LWIPDIR)/core/netif.c \
	$(LWIPDIR)/core/pbuf.c \
	$(LWIPDIR)/core/raw.c \
	$(LWIPDIR)/core/stats.c \
	$(LWIPDIR)/core/sys.c \
	$(LWIPDIR)/core/tcp.c \
	$(LWIPDIR)/core/tcp_in.c \
	$(LWIPDIR)/core/tcp_out.c \
	$(LWIPDIR)/core/timeouts.c \
	$(LWIPDIR)/core/udp.c
CORE4FILES=$(LWIPDIR)/core/ipv4/autoip.c \
	$(LWIPDIR)/core/ipv4/dhcp.c \
	$(LWIPDIR)/core/ipv4/etharp.c \
	$(LWIPDIR)/core/ipv4/icmp.c \
	$(LWIPDIR)/core/ipv4/igmp.c \
	$(LWIPDIR)/core/ipv4/ip4_frag.c \
	$(LWIPDIR)/core/ipv4/ip4.c \
	$(LWIPDIR)/core/ipv4/ip4_addr.c
CORE6FILES=$(LWIPDIR)/core/ipv6/ethip6.c \
	$(LWIPDIR)/core/ipv6/icmp6.c \
	$(LWIPDIR)/core/ipv6/inet6.c \
	$(LWIPDIR)/core/ipv6/ip6.c \
	$(LWIPDIR)/core/ipv6/ip6_addr.c \
	$(LWIPDIR)/core/ipv6/ip6_frag.c \
	$(LWIPDIR)/core/ipv6/mld6.c \
	$(LWIPDIR)/core/ipv6/dhcp6.c \
	$(LWIPDIR)/core/ipv6/nd6.c
	# APIFILES: The files which implement the sequential and socket APIs.
APIFILES=$(LWIPDIR)/api/err.c
#$(LWIPDIR)/api/api_lib.c \
	$(LWIPDIR)/api/api_msg.c \
	 \
	$(LWIPDIR)/api/netbuf.c \
	$(LWIPDIR)/api/netdb.c \
	$(LWIPDIR)/api/netifapi.c \
	$(LWIPDIR)/api/sockets.c \
	$(LWIPDIR)/api/tcpip.c
# NETIFFILES: Files implementing various generic network interface functions
NETIFFILES=$(LWIPDIR)/netif/ethernet.c 
# SIXLOWPAN: 6LoWPAN
SIXLOWPAN=$(LWIPDIR)/netif/lowpan6.c \
# ARCHFILES: Architecture specific files.
ARCHFILES=$(wildcard $(LWIPARCH)/*.c $(LWIPARCH)tapif.c $(LWIPARCH)/netif/list.c $(LWIPARCH)/netif/tcpdump.c)
# LWIPFILES: All the above.
LWIPFILES=$(COREFILES) $(NETIFFILES) $(ARCHFILES) $(APIFILES)

ifeq ($(LIBZT_IPV4),1)
	LWIPFILES+=$(CORE4FILES)
	CFLAGS+=-DLIBZT_IPV4=1 -DLWIP_IPV4 -DLWIP_IPV6=0 -DIPv4
endif
ifeq ($(LIBZT_IPV6),1)
	LWIPFILES+=$(CORE6FILES)
	CFLAGS+=-DLIBZT_IPV6=1 -DLWIP_IPV6 -DLWIP_IPV4=0 -DIPv6 
endif
ifeq ($(LWIP_DEBUG),1)
	CFLAGS+=-DLWIP_DEBUG=1
endif

LWIPFILESW=$(wildcard $(LWIPFILES))
LWIPOBJS=$(notdir $(LWIPFILESW:.c=.o))

%.o:
	$(CXX) $(CFLAGS) -c $(<:.o=.c) -o obj/$@

all:
.PHONY: all

clean:
	rm -rf .depend
	rm -f *.o *.s .depend* *.core core

depend dep: .depend
include .depend

liblwip.a: clean $(LWIPOBJS)
	#libtool -static -o $@ $^

.depend: $(LWIPFILES)
	$(CCDEP) $(CFLAGS) -MM $^ > .depend || rm -f .depend