#ifndef __ETHERNETIF_PRIV_H
#define __ETHERNETIF_PRIV_H

#include "ethernetif.h"
#include "../netif.h"
#include "../../syslog.h"

err_t ethernetif_init(struct netif *netif);
void ethernetif_input(struct netif *netif);
extern err_t (*ethernetif_up_callback)(struct netif *netif);

void ethernet_log(const struct pci_device_info *info, const char *msg);

typedef struct pbuf *(*netif_linkinput_fn)(struct netif *netif);
#define IF_TDBUFNUM 32

/** netif.state of ethernetif handlers */
struct ifstate {
  struct pci_device_info pci;
  netif_linkinput_fn linkinput;
  void* mmio_addr;
  void* tx_desc;
  void** tx_buf[IF_TDBUFNUM];
  void* rx_desc;
};

/** Hardware ethernet driver */
struct eth_hw_handler_t {
  uint16_t vendor, device;
  netif_init_fn init;
  netif_linkoutput_fn output;
  netif_linkinput_fn input;
  uint64_t link_speed;
  char name[2];
};

#define ethif_log(pciId, lvl, fmt, ...) { \
    uint8_t bus, dev, func; \
    pci_split_id(pciId, &bus, &dev, &func); \
    /* "Dev/0xx:xx:xxx\0" */  \
    char sub[16] = {0}; \
    sub[0] = 'D'; sub[1] = 'e'; sub[2] = 'v'; \
    snprintf(sub+4, 9, "%02x:%02x:%d", bus, dev, func); \
    syslogf((syslog_ctx){"NET", sub, lvl}, fmt, __VA_ARGS__); \
}

#endif
