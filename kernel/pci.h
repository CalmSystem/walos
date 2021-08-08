#ifndef __PCI_H
#define __PCI_H

#include "stdint.h"
#include "stdbool.h"

typedef uint32_t pci_id;
static inline void pci_split_id(pci_id id, uint8_t *bus, uint8_t *dev, uint8_t *func) {
    if (bus) *bus = (id >> 16) & 0xFF;
    if (dev) *dev = (id >> 11) & 0b11111;
    if (func) *func = (id >> 8) & 0b11;
}

static inline uint16_t pci_pack_class(uint8_t base, uint8_t sub) {
    return (((uint16_t)base << 8) | sub);
}
const char *pci_class_name(uint16_t fullclass, uint8_t progIntf);

struct pci_device_info {
    pci_id pciId;
    uint16_t vendorId;
    uint16_t deviceId;
    uint8_t baseclass;
    uint8_t subclass;
    uint8_t progIf;
};
struct pci_driver {
    void (*handle)(const struct pci_device_info* info);
    /** Devices triggering handle */
    union pci_selector {
        /** IF is_device */
        struct pci_selector_device {
            uint16_t vendorId;
            uint16_t deviceId;
        } device;
        /** If !is_device */
        struct pci_selector_type {
            uint8_t baseclass;
            uint8_t subclass;
            /** 0xFF match any */
            uint8_t progIf;
        } type;
    } selector;
    /** Is selector using deviceIds */
    bool is_device;
};
typedef void (*pci_list_f)(const struct pci_device_info*);

/** Interrate pci devices, calling list and pci_driver.handle if target match */
void pci_scan(struct pci_driver *drivers, uint32_t ndrivers, pci_list_f list);

#endif
