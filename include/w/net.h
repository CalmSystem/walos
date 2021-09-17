#ifndef __MOD_NET_H
#define __MOD_NET_H
#include "types.h"

typedef struct {
	uint8_t addr[6];
} w_mac_addr;

typedef struct {
	uint8_t addr[4];
} w_ip4_addr;
typedef struct {
	uint8_t addr[16];
} w_ip6_addr;
typedef union {
	w_ip4_addr v4;
	w_ip6_addr v6;
} w_ip_addr;

#endif
