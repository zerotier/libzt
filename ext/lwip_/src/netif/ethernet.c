/**
 * @file
 * Ethernet common functions
 * 
 * @defgroup ethernet Ethernet
 * @ingroup callbackstyle_api
 */

/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * Copyright (c) 2003-2004 Leon Woestenberg <leon.woestenberg@axon.tv>
 * Copyright (c) 2003-2004 Axon Digital Design B.V., The Netherlands.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 */

#include "lwip/opt.h"

#if LWIP_ARP || LWIP_ETHERNET

#include "netif/ethernet.h"
#include "lwip/def.h"
#include "lwip/stats.h"
#include "lwip/etharp.h"
#include "lwip/ip.h"
#include "lwip/snmp.h"

#include <string.h>

#include "netif/ppp/ppp_opts.h"
#if PPPOE_SUPPORT
#include "netif/ppp/pppoe.h"
#endif /* PPPOE_SUPPORT */

const struct eth_addr ethbroadcast = {{0xff,0xff,0xff,0xff,0xff,0xff}};
const struct eth_addr ethzero = {{0,0,0,0,0,0}};






/**
 * @ingroup lwip_nosys
 * Process received ethernet frames. Using this function instead of directly
 * calling ip_input and passing ARP frames through etharp in ethernetif_input,
 * the ARP cache is protected from concurrent access.\n
 * Don't call directly, pass to netif_add() and call netif->input().
 *
 * @param p the received packet, p->payload pointing to the ethernet header
 * @param netif the network interface on which the packet was received
 * 
 * @see @ref LWIP_HOOK_UNKNOWN_ETH_PROTOCOL,
 * @ref ETHARP_SUPPORT_VLAN and
 * @ref LWIP_HOOK_VLAN_CHECK.
 */
err_t
ethernet_input(struct pbuf *p, struct netif *netif)
{
  struct eth_hdr* ethhdr;
  u16_t type;
#if LWIP_ARP || ETHARP_SUPPORT_VLAN
  s16_t ip_hdr_offset = SIZEOF_ETH_HDR;
#endif /* LWIP_ARP || ETHARP_SUPPORT_VLAN */

  if (p->len <= SIZEOF_ETH_HDR) {
    /* a packet with only an ethernet header (or less) is not valid for us */
    ETHARP_STATS_INC(etharp.proterr);
    ETHARP_STATS_INC(etharp.drop);
    goto free_and_return;
  }

  /* points to packet payload, which starts with an Ethernet header */
  ethhdr = (struct eth_hdr *)p->payload;
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE,
    ("ethernet_input: dest:%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F", src:%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F", type:%"X16_F"\n",
     (unsigned)ethhdr->dest.addr[0], (unsigned)ethhdr->dest.addr[1], (unsigned)ethhdr->dest.addr[2],
     (unsigned)ethhdr->dest.addr[3], (unsigned)ethhdr->dest.addr[4], (unsigned)ethhdr->dest.addr[5],
     (unsigned)ethhdr->src.addr[0], (unsigned)ethhdr->src.addr[1], (unsigned)ethhdr->src.addr[2],
     (unsigned)ethhdr->src.addr[3], (unsigned)ethhdr->src.addr[4], (unsigned)ethhdr->src.addr[5],
     (unsigned)htons(ethhdr->type)));

  type = ethhdr->type;
#if ETHARP_SUPPORT_VLAN
  if (type == PP_HTONS(ETHTYPE_VLAN)) {
    struct eth_vlan_hdr *vlan = (struct eth_vlan_hdr*)(((char*)ethhdr) + SIZEOF_ETH_HDR);
    if (p->len <= SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR) {
      /* a packet with only an ethernet/vlan header (or less) is not valid for us */
      ETHARP_STATS_INC(etharp.proterr);
      ETHARP_STATS_INC(etharp.drop);
      goto free_and_return;
    }
#if defined(ETHARP_VLAN_CHECK) || defined(ETHARP_VLAN_CHECK_FN) /* if not, allow all VLANs */
#ifdef ETHARP_VLAN_CHECK_FN
    if (!ETHARP_VLAN_CHECK_FN(ethhdr, vlan)) {
#elif defined(ETHARP_VLAN_CHECK)
    if (VLAN_ID(vlan) != ETHARP_VLAN_CHECK) {
#endif
      /* silently ignore this packet: not for our VLAN */
      pbuf_free(p);
      return ERR_OK;
    }
#endif /* defined(ETHARP_VLAN_CHECK) || defined(ETHARP_VLAN_CHECK_FN) */
    type = vlan->tpid;
    ip_hdr_offset = SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR;
  }
#endif /* ETHARP_SUPPORT_VLAN */

#if LWIP_ARP_FILTER_NETIF
  netif = LWIP_ARP_FILTER_NETIF_FN(p, netif, htons(type));
#endif /* LWIP_ARP_FILTER_NETIF*/


            LWIP_DEBUGF(IP6_DEBUG,("switch(type)"));

ip6_input(p, netif);

  switch (type) {
#if LWIP_ARP
    /* IP packet? */
    case PP_HTONS(ETHTYPE_IP):
      if (!(netif->flags & NETIF_FLAG_ETHARP)) {
        LWIP_DEBUGF(IP6_DEBUG,("ETHTYPE_IP -- !(netif->flags & NETIF_FLAG_ETHARP)\n"));
        goto free_and_return;
      }
#if ETHARP_TRUST_IP_MAC
      /* update ARP table */
      LWIP_DEBUGF(IP6_DEBUG,("passing to etharp_ip_input()"));
      etharp_ip_input(netif, p);
#endif /* ETHARP_TRUST_IP_MAC */
      /* skip Ethernet header */
      if(pbuf_header(p, -ip_hdr_offset)) {
        LWIP_ASSERT("Can't move over header in packet", 0);
        LWIP_DEBUGF(IP6_DEBUG,("pbuf_header(p, -ip_hdr_offset)\n"));

        goto free_and_return;
      } else {
        /* pass to IP layer */
        LWIP_DEBUGF(IP6_DEBUG,("passing to ip_input()"));
        ip_input(p, netif);
      }
      break;
      
    case PP_HTONS(ETHTYPE_ARP):
      if (!(netif->flags & NETIF_FLAG_ETHARP)) {
                LWIP_DEBUGF(IP6_DEBUG,("ETHTYPE_ARP --- !(netif->flags & NETIF_FLAG_ETHARP)\n"));

        goto free_and_return;
      }
      /* pass p to ARP module */
      //etharp_arp_input(netif, (struct eth_addr*)(netif->hwaddr), p);
      break;
#endif /* LWIP_ARP */

#if PPPOE_SUPPORT
    case PP_HTONS(ETHTYPE_PPPOEDISC): /* PPP Over Ethernet Discovery Stage */
      pppoe_disc_input(netif, p);
      break;

    case PP_HTONS(ETHTYPE_PPPOE): /* PPP Over Ethernet Session Stage */
      pppoe_data_input(netif, p);
      break;
#endif /* PPPOE_SUPPORT */

    default:
      ETHARP_STATS_INC(etharp.proterr);
      ETHARP_STATS_INC(etharp.drop);
      LWIP_DEBUGF(IP6_DEBUG,("default\n"));

      goto free_and_return;
  }

  /* This means the pbuf is freed or consumed,
     so the caller doesn't have to free it again */
              LWIP_DEBUGF(IP6_DEBUG,("freed already"));

  return ERR_OK;

free_and_return:
            LWIP_DEBUGF(IP6_DEBUG,("freeing..."));

  pbuf_free(p);
  return ERR_OK;
}

/**
 * @ingroup ethernet
 * Send an ethernet packet on the network using netif->linkoutput().
 * The ethernet header is filled in before sending.
 *
 * @see @ref LWIP_HOOK_VLAN_SET
 *
 * @param netif the lwIP network interface on which to send the packet
 * @param p the packet to send. pbuf layer must be @ref PBUF_LINK.
 * @param src the source MAC address to be copied into the ethernet header
 * @param dst the destination MAC address to be copied into the ethernet header
 * @param eth_type ethernet type (@ref eth_type)
 * @return ERR_OK if the packet was sent, any other err_t on failure
 */
err_t
ethernet_output(struct netif* netif, struct pbuf* p,
                const struct eth_addr* src, const struct eth_addr* dst,
                u16_t eth_type)
{
  struct eth_hdr* ethhdr;
  u16_t eth_type_be = htons(eth_type);

#if ETHARP_SUPPORT_VLAN && defined(LWIP_HOOK_VLAN_SET)
  s32_t vlan_prio_vid = LWIP_HOOK_VLAN_SET(netif, p, src, dst, eth_type);
  if (vlan_prio_vid >= 0) {
    struct eth_vlan_hdr* vlanhdr;

    LWIP_ASSERT("prio_vid must be <= 0xFFFF", vlan_prio_vid <= 0xFFFF);

    if (pbuf_header(p, SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR) != 0) {
      goto pbuf_header_failed;
    }
    vlanhdr = (struct eth_vlan_hdr*)(((u8_t*)p->payload) + SIZEOF_ETH_HDR);
    vlanhdr->tpid     = eth_type_be;
    vlanhdr->prio_vid = htons((u16_t)vlan_prio_vid);

    eth_type_be = PP_HTONS(ETHTYPE_VLAN);
  } else
#endif /* ETHARP_SUPPORT_VLAN && defined(LWIP_HOOK_VLAN_SET) */
  {
    if (pbuf_header(p, SIZEOF_ETH_HDR) != 0) {
      goto pbuf_header_failed;
    }
  }

  ethhdr = (struct eth_hdr*)p->payload;
  ethhdr->type = eth_type_be;
  ETHADDR32_COPY(&ethhdr->dest, dst);
  ETHADDR16_COPY(&ethhdr->src,  src);

  LWIP_ASSERT("netif->hwaddr_len must be 6 for ethernet_output!",
    (netif->hwaddr_len == ETH_HWADDR_LEN));
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE,
    ("ethernet_output: sending packet %p\n", (void *)p));

  /* send the packet */
  return netif->linkoutput(netif, p);

pbuf_header_failed:
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
    ("ethernet_output: could not allocate room for header.\n"));
  LINK_STATS_INC(link.lenerr);
  return ERR_BUF;
}

#endif /* LWIP_ARP || LWIP_ETHERNET */
