#include "pci_driver.h"
#include "asm.h"

enum pci_type {
    PCI_TYPE_MULTIFUNC             = 0x80,
    PCI_TYPE_GENERIC               = 0x00,
    PCI_TYPE_PCI_BRIDGE            = 0x01,
    PCI_TYPE_CARDBUS_BRIDGE        = 0x02
};

#define PCI_CONFIG_ADDR                 0xcf8
#define PCI_CONFIG_DATA                 0xcfC
uint8_t pci_read8(pci_id id, enum pci_reg reg) {
    uint32_t addr = 0x80000000 | id | (reg & 0xfc);
    io_write32(PCI_CONFIG_ADDR, addr);
    return io_read8(PCI_CONFIG_DATA + (reg & 0x03));
}
uint16_t pci_read16(pci_id id, enum pci_reg reg) {
    uint32_t addr = 0x80000000 | id | (reg & 0xfc);
    io_write32(PCI_CONFIG_ADDR, addr);
    return io_read16(PCI_CONFIG_DATA + (reg & 0x02));
}
uint32_t pci_read32(pci_id id, enum pci_reg reg) {
    uint32_t addr = 0x80000000 | id | (reg & 0xfc);
    io_write32(PCI_CONFIG_ADDR, addr);
    return io_read32(PCI_CONFIG_DATA);
}

void pci_write8(pci_id id, enum pci_reg reg, uint8_t data) {
    uint32_t address = 0x80000000 | id | (reg & 0xfc);
    io_write32(PCI_CONFIG_ADDR, address);
    io_write8(PCI_CONFIG_DATA + (reg & 0x03), data);
}
void pci_write16(pci_id id, enum pci_reg reg, uint16_t data) {
    uint32_t address = 0x80000000 | id | (reg & 0xfc);
    io_write32(PCI_CONFIG_ADDR, address);
    io_write16(PCI_CONFIG_DATA + (reg & 0x02), data);
}
void pci_write32(pci_id id, enum pci_reg reg, uint32_t data) {
    uint32_t address = 0x80000000 | id | (reg & 0xfc);
    io_write32(PCI_CONFIG_ADDR, address);
    io_write32(PCI_CONFIG_DATA, data);
}

static void pci_read_bar(pci_id id, uint32_t index, uint32_t *address, uint32_t *mask) {
    enum pci_reg reg = PCI_CONFIG_BAR0 + index * sizeof(uint32_t);

    // Get address
    *address = pci_read32(id, reg);

    // Find out size of the bar
    pci_write32(id, reg, 0xffffffff);
    *mask = pci_read32(id, reg);

    // Restore adddress
    pci_write32(id, reg, *address);
}

void pci_get_bar(struct pci_bar *bar, pci_id id, uint32_t index) {
    // Read pci bar register
    uint32_t addressLow;
    uint32_t maskLow;
    pci_read_bar(id, index, &addressLow, &maskLow);

    if (addressLow & PCI_BAR_64)
    {
        // 64-bit mmio
        uint32_t addressHigh;
        uint32_t maskHigh;
        pci_read_bar(id, index + 1, &addressHigh, &maskHigh);

        bar->u.address = (void *)(((uintptr_t)addressHigh << 32) | (addressLow & ~0xf));
        bar->size = ~(((uint64_t)maskHigh << 32) | (maskLow & ~0xf)) + 1;
        bar->flags = addressLow & 0xf;
    }
    else if (addressLow & PCI_BAR_IO)
    {
        // i/o register
        bar->u.port = (uint16_t)(addressLow & ~0x3);
        bar->size = (uint16_t)(~(maskLow & ~0x3) + 1);
        bar->flags = addressLow & 0x3;
    }
    else
    {
        // 32-bit mmio
        bar->u.address = (void *)(uintptr_t)(addressLow & ~0xf);
        bar->size = ~(maskLow & ~0xf) + 1;
        bar->flags = addressLow & 0xf;
    }
}

struct pci_scan_t {
    struct pci_driver *drivers;
    uint32_t ndrivers;
    pci_list_f list;
};
static void pci_scan_function(struct pci_scan_t *scan, uint8_t bus, uint8_t dev, uint8_t func);
static inline void pci_scan_device(struct pci_scan_t *scan, uint8_t bus, uint8_t dev) {
    uint16_t vendorID = pci_read16(PCI_MAKE_ID(bus, dev, 0), PCI_CONFIG_VENDOR_ID);
    if (vendorID == PCI_VENDOR_NONE) return;

    pci_scan_function(scan, bus, dev, 0);
    uint8_t headerType = pci_read8(PCI_MAKE_ID(bus, dev, 0), PCI_CONFIG_HEADER_TYPE);
    uint8_t funcCount = headerType & PCI_TYPE_MULTIFUNC ? 8 : 1;
    for (uint8_t func = 1; func < funcCount; func++) {
        vendorID = pci_read16(PCI_MAKE_ID(bus, dev, func), PCI_CONFIG_VENDOR_ID);
        if (vendorID == PCI_VENDOR_NONE) continue;

        pci_scan_function(scan, bus, dev, func);
    }
}
static void pci_scan_bus(struct pci_scan_t *scan, uint8_t bus) {
    for (uint8_t dev = 0; dev < 32; dev++) {
        pci_scan_device(scan, bus, dev);
    }
}
static void pci_scan_function(struct pci_scan_t *scan, uint8_t bus, uint8_t dev, uint8_t func) {
    struct pci_device_info info;
    info.pciId = PCI_MAKE_ID(bus, dev, func);
    info.vendorId = pci_read16(info.pciId, PCI_CONFIG_VENDOR_ID);
    info.deviceId = pci_read16(info.pciId, PCI_CONFIG_DEVICE_ID);
    info.progIf = pci_read8(info.pciId, PCI_CONFIG_PROG_INTF);
    info.subclass = pci_read8(info.pciId, PCI_CONFIG_SUBCLASS);
    info.baseclass = pci_read8(info.pciId, PCI_CONFIG_BASECLASS);
    if (scan->list) scan->list(&info);

    for (uint32_t i = 0; i < scan->ndrivers; i++) {
        if (scan->drivers[i].is_device) {
            struct pci_selector_device s = scan->drivers[i].selector.device;
            if (s.vendorId != info.vendorId || s.deviceId != info.deviceId) continue;
        } else {
            struct pci_selector_type s = scan->drivers[i].selector.type;
            if (s.baseclass != info.baseclass || s.subclass != info.subclass ||
                (s.progIf != info.progIf && s.progIf != PCI_SERIAL_ANY)) continue;
        }
        scan->drivers[i].handle(&info);
    }

    if (pci_pack_class(info.baseclass, info.subclass) == PCI_BRIDGE_PCI) {
        pci_scan_bus(scan, pci_read8(info.pciId, PCI_CONFIG_SECONDARY_BUS));
    }
}
void pci_scan(struct pci_driver *drivers, uint32_t ndrivers, pci_list_f list) {
    struct pci_scan_t scan = {drivers, ndrivers, list};
    uint8_t headerType = pci_read8(PCI_MAKE_ID(0, 0, 0), PCI_CONFIG_HEADER_TYPE);
    uint8_t funcCount = headerType & PCI_TYPE_MULTIFUNC ? 8 : 1;
    for (uint8_t func = 0; func < funcCount; func++) {
        uint16_t vendorId = pci_read16(PCI_MAKE_ID(0, 0, func), PCI_CONFIG_VENDOR_ID);
        if (vendorId == PCI_VENDOR_NONE) break;

        pci_scan_bus(&scan, func);
    }
}

void pci_enable(pci_id id) {
    uint16_t cmd = pci_read16(id, PCI_CONFIG_COMMAND);
    // IO, MEM, MASTER
    pci_write16(id, PCI_CONFIG_COMMAND, cmd | 0b111);
}

const char *pci_class_name(uint16_t fullclass, uint8_t progIf) {
    switch (fullclass)
    {
    case PCI_VGA_COMPATIBLE:            return "VGA-Compatible Device";
    case PCI_STORAGE_SCSI:              return "SCSI Storage Controller";
    case PCI_STORAGE_IDE:               return "IDE Interface";
    case PCI_STORAGE_FLOPPY:            return "Floppy Disk Controller";
    case PCI_STORAGE_IPI:               return "IPI Bus Controller";
    case PCI_STORAGE_RAID:              return "RAID Bus Controller";
    case PCI_STORAGE_ATA:               return "ATA Controller";
    case PCI_STORAGE_SATA:              return "SATA Controller";
    case PCI_STORAGE_OTHER:             return "Mass Storage Controller";
    case PCI_NETWORK_ETHERNET:          return "Ethernet Controller";
    case PCI_NETWORK_TOKEN_RING:        return "Token Ring Controller";
    case PCI_NETWORK_FDDI:              return "FDDI Controller";
    case PCI_NETWORK_ATM:               return "ATM Controller";
    case PCI_NETWORK_ISDN:              return "ISDN Controller";
    case PCI_NETWORK_WORLDFIP:          return "WorldFip Controller";
    case PCI_NETWORK_PICGMG:            return "PICMG Controller";
    case PCI_NETWORK_OTHER:             return "Network Controller";
    case PCI_DISPLAY_VGA:               return "VGA-Compatible Controller";
    case PCI_DISPLAY_XGA:               return "XGA-Compatible Controller";
    case PCI_DISPLAY_3D:                return "3D Controller";
    case PCI_DISPLAY_OTHER:             return "Display Controller";
    case PCI_MULTIMEDIA_VIDEO:          return "Multimedia Video Controller";
    case PCI_MULTIMEDIA_AUDIO:          return "Multimedia Audio Controller";
    case PCI_MULTIMEDIA_PHONE:          return "Computer Telephony Device";
    case PCI_MULTIMEDIA_AUDIO_DEVICE:   return "Audio Device";
    case PCI_MULTIMEDIA_OTHER:          return "Multimedia Controller";
    case PCI_MEMORY_RAM:                return "RAM Memory";
    case PCI_MEMORY_FLASH:              return "Flash Memory";
    case PCI_MEMORY_OTHER:              return "Memory Controller";
    case PCI_BRIDGE_HOST:               return "Host Bridge";
    case PCI_BRIDGE_ISA:                return "ISA Bridge";
    case PCI_BRIDGE_EISA:               return "EISA Bridge";
    case PCI_BRIDGE_MCA:                return "MicroChannel Bridge";
    case PCI_BRIDGE_PCI:                return "PCI Bridge";
    case PCI_BRIDGE_PCMCIA:             return "PCMCIA Bridge";
    case PCI_BRIDGE_NUBUS:              return "NuBus Bridge";
    case PCI_BRIDGE_CARDBUS:            return "CardBus Bridge";
    case PCI_BRIDGE_RACEWAY:            return "RACEway Bridge";
    case PCI_BRIDGE_OTHER:              return "Bridge Device";
    case PCI_COMM_SERIAL:               return "Serial Controller";
    case PCI_COMM_PARALLEL:             return "Parallel Controller";
    case PCI_COMM_MULTIPORT:            return "Multiport Serial Controller";
    case PCI_COMM_MODEM:                return "Modem";
    case PCI_COMM_OTHER:                return "Communication Controller";
    case PCI_SYSTEM_PIC:                return "PIC";
    case PCI_SYSTEM_DMA:                return "DMA Controller";
    case PCI_SYSTEM_TIMER:              return "Timer";
    case PCI_SYSTEM_RTC:                return "RTC";
    case PCI_SYSTEM_PCI_HOTPLUG:        return "PCI Hot-Plug Controller";
    case PCI_SYSTEM_SD:                 return "SD Host Controller";
    case PCI_SYSTEM_OTHER:              return "System Peripheral";
    case PCI_INPUT_KEYBOARD:            return "Keyboard Controller";
    case PCI_INPUT_PEN:                 return "Pen Controller";
    case PCI_INPUT_MOUSE:               return "Mouse Controller";
    case PCI_INPUT_SCANNER:             return "Scanner Controller";
    case PCI_INPUT_GAMEPORT:            return "Gameport Controller";
    case PCI_INPUT_OTHER:               return "Input Controller";
    case PCI_DOCKING_GENERIC:           return "Generic Docking Station";
    case PCI_DOCKING_OTHER:             return "Docking Station";
    case PCI_PROCESSOR_386:             return "386";
    case PCI_PROCESSOR_486:             return "486";
    case PCI_PROCESSOR_PENTIUM:         return "Pentium";
    case PCI_PROCESSOR_ALPHA:           return "Alpha";
    case PCI_PROCESSOR_MIPS:            return "MIPS";
    case PCI_PROCESSOR_CO:              return "CO-Processor";
    case PCI_SERIAL_FIREWIRE:           return "FireWire (IEEE 1394)";
    case PCI_SERIAL_SSA:                return "SSA";
    case PCI_SERIAL_USB:
        switch (progIf)
        {
        case PCI_SERIAL_USB_UHCI:       return "USB (UHCI)";
        case PCI_SERIAL_USB_OHCI:       return "USB (OHCI)";
        case PCI_SERIAL_USB_EHCI:       return "USB2";
        case PCI_SERIAL_USB_XHCI:       return "USB3";
        case PCI_SERIAL_USB_OTHER:      return "USB Controller";
        default:                        return "Unknown USB Class";
        }
        break;
    case PCI_SERIAL_FIBER:              return "Fiber Channel";
    case PCI_SERIAL_SMBUS:              return "SMBus";
    case PCI_WIRELESS_IRDA:             return "iRDA Compatible Controller";
    case PCI_WIRLESSS_IR:               return "Consumer IR Controller";
    case PCI_WIRLESSS_RF:               return "RF Controller";
    case PCI_WIRLESSS_BLUETOOTH:        return "Bluetooth";
    case PCI_WIRLESSS_BROADBAND:        return "Broadband";
    case PCI_WIRLESSS_ETHERNET_A:       return "802.1a Controller";
    case PCI_WIRLESSS_ETHERNET_B:       return "802.1b Controller";
    case PCI_WIRELESS_OTHER:            return "Wireless Controller";
    case PCI_INTELLIGENT_I2O:           return "I2O Controller";
    case PCI_SATELLITE_TV:              return "Satellite TV Controller";
    case PCI_SATELLITE_AUDIO:           return "Satellite Audio Controller";
    case PCI_SATELLITE_VOICE:           return "Satellite Voice Controller";
    case PCI_SATELLITE_DATA:            return "Satellite Data Controller";
    case PCI_CRYPT_NETWORK:             return "Network and Computing Encryption Device";
    case PCI_CRYPT_ENTERTAINMENT:       return "Entertainment Encryption Device";
    case PCI_CRYPT_OTHER:               return "Encryption Device";
    case PCI_SP_DPIO:                   return "DPIO Modules";
    case PCI_SP_OTHER:                  return "Signal Processing Controller";
    default:                            return "Unknown PCI Class";
    }
}
