#ifndef __NET_H
#define __NET_H

#include "netif/ethernetif.h"

/** Initialise networking */
void net_init(bool auto_dhcp);
/** Update default interface */
void net_pull(void);

#endif
