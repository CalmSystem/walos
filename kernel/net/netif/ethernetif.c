/**
 * @file
 * Ethernet Interface Skeleton
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
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
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include "../opt.h"

#include "string.h"
#include "../def.h"
#include "../pbuf.h"
#include "../snmp.h"
#include "../etharp.h"
#include "../../pci_driver.h"
#include "e1000.h"

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
void
ethernetif_input(struct netif *netif)
{
  struct ifstate *state = netif->state;
  struct pbuf *p;

  /* move received packet into a new pbuf */
  p = state->linkinput(netif);
  /* if no packet could be read, silently ignore this */
  if (p != NULL) {
    /* pass all packets to ethernet_input, which decides what packets it supports */
    if (netif->input(p, netif) != ERR_OK) {
      LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
      pbuf_free(p);
      p = NULL;
    }
  }
}

static bool try_init(struct netif* netif, const struct eth_hw_handler_t* handler, err_t* out) {
  struct ifstate *state = netif->state;
  if (handler->vendor != state->pci.vendorId ||
      handler->device != state->pci.deviceId)
    return false;

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, handler->link_speed);

  netif->name[0] = handler->name[0];
  netif->name[1] = handler->name[1];
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
#if LWIP_IPV4
  netif->output = etharp_output;
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
  netif->linkoutput = handler->output;

  state->linkinput = handler->input;

  /* initialize the hardware */
#if LWIP_IPV6 && LWIP_IPV6_MLD
  /*
   * For hardware/netifs that implement MAC filtering.
   * All-nodes link-local is handled by default, so we must let the hardware know
   * to allow multicast packets in.
   * Should set mld_mac_filter previously. */
  if (netif->mld_mac_filter != NULL) {
    ip6_addr_t ip6_allnodes_ll;
    ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
    netif->mld_mac_filter(netif, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
  }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

  *out = handler->init(netif);
  return true;
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t
ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));
  LWIP_ASSERT("netif->state != NULL", (netif->state != NULL));

  struct ifstate *state = netif->state;

#if LWIP_NETIF_HOSTNAME
  /* Initialize inter_face hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  err_t ret = ERR_VAL;
  for (size_t i = 0; i < sizeof(e1000_handlers)/sizeof(e1000_handlers[0]); i++) {
    if (try_init(netif, &e1000_handlers[i], &ret) && ret == ERR_OK) {
      ethif_log(state->pci.pciId, DEBUG, "Using e1000", NULL);
      return ret;
    }
  }

  ethif_log(state->pci.pciId, ERROR, "No driver found 0x%04x/0x%04x",
    state->pci.vendorId, state->pci.deviceId);
  return ret;
}

err_t (*ethernetif_up_callback)(struct netif *netif) = NULL;

static void ethernet_pci_handle(const struct pci_device_info* info) {
#if !LWIP_SINGLE_NETIF
  for (struct netif *n = netif_list; n != NULL; n = n->next) {
    if (n->state && memcmp(&((struct ifstate*)n->state)->pci, info, sizeof(*info)) == 0)
      return;
  }
#else
  if (netif_default != NULL)
    return;
#endif

  struct netif *netif = malloc(sizeof(*netif));
  if (!netif) return;

  struct ifstate *state = malloc(sizeof(*state));
  if (!state) return;

  memcpy(&state->pci, info, sizeof(*info));
  netif->state = state;

  bool found = netif_add(netif, IP4_ADDR_ANY, IP4_ADDR_ANY, IP4_ADDR_ANY, state, ethernetif_init, netif_input);
  if (!found) {
    free(state);
    free(netif);
    return;
  }

#if LWIP_IPV6
  netif_create_ip6_linklocal_address(netif, 1);
  netif->ip6_autoconfig_enabled = 1;
  netif_set_status_callback(netif, netif_status_callback);
#endif
  netif_set_default(netif);
  netif_set_up(netif);

  ethif_log(info->pciId, INFO, "Up as If %d", netif->num);
  if (ethernetif_up_callback)
    ethernetif_up_callback(netif);
}

struct pci_driver ethernet_pci_driver = {
  ethernet_pci_handle, {.type={PCI_CLASS_NETWORK, 0, PCI_SERIAL_ANY}}, false
};
