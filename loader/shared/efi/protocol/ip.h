#ifndef __EFI_IP_H
#define __EFI_IP_H

typedef struct {
    uint8_t Addr[32];
} EFI_MAC_ADDRESS;

typedef struct {
    uint8_t Addr[4];
} EFI_IPv4_ADDRESS;

typedef struct {
    uint8_t Addr[16];
} EFI_IPv6_ADDRESS;

typedef union {
    uint32_t Addr[4];
    EFI_IPv4_ADDRESS v4;
    EFI_IPv6_ADDRESS v6;
} EFI_IP_ADDRESS;

#endif
