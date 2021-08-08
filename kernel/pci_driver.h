#ifndef __PCI_DRIVER_H
#define __PCI_DRIVER_H

#include "pci.h"

enum pci_reg {
    // PCI Configuration Registers
    PCI_CONFIG_VENDOR_ID = 0x00,
    PCI_CONFIG_DEVICE_ID = 0x02,
    PCI_CONFIG_COMMAND = 0x04,
    PCI_CONFIG_STATUS = 0x06,
    PCI_CONFIG_REVISION_ID = 0x08,
    PCI_CONFIG_PROG_INTF = 0x09,
    PCI_CONFIG_SUBCLASS = 0x0a,
    PCI_CONFIG_BASECLASS = 0x0b,
    PCI_CONFIG_CACHELINE_SIZE = 0x0c,
    PCI_CONFIG_LATENCY = 0x0d,
    PCI_CONFIG_HEADER_TYPE = 0x0e,
    PCI_CONFIG_BIST = 0x0f,

    // Type 0x00 (Generic) Configuration Registers
    PCI_CONFIG_BAR0 = 0x10,
    PCI_CONFIG_BAR1 = 0x14,
    PCI_CONFIG_BAR2 = 0x18,
    PCI_CONFIG_BAR3 = 0x1c,
    PCI_CONFIG_BAR4 = 0x20,
    PCI_CONFIG_BAR5 = 0x24,
    PCI_CONFIG_CARDBUS_CIS = 0x28,
    PCI_CONFIG_SUBSYSTEM_VENDOR_ID = 0x2c,
    PCI_CONFIG_SUBSYSTEM_DEVICE_ID = 0x2e,
    PCI_CONFIG_EXPANSION_ROM = 0x30,
    PCI_CONFIG_CAPABILITIES = 0x34,
    PCI_CONFIG_INTERRUPT_LINE = 0x3c,
    PCI_CONFIG_INTERRUPT_PIN = 0x3d,
    PCI_CONFIG_MIN_GRANT = 0x3e,
    PCI_CONFIG_MAX_LATENCY = 0x3f,

    // Type 0x01 (PCI-to-PCI bridge) Configuration Registers
    PCI_CONFIG_PRIMARY_BUS = 0x18,
    PCI_CONFIG_SECONDARY_BUS = 0x19,
    PCI_CONFIG_SUBORDINATE_BUS = 0x1A,
    PCI_CONFIG_SECONDARY_LATENCY = 0x1B
};

#define PCI_MAKE_ID(bus, dev, func) ((bus) << 16) | ((dev) << 11) | ((func) << 8)

uint8_t pci_read8(pci_id id, enum pci_reg reg);
uint16_t pci_read16(pci_id id, enum pci_reg reg);
uint32_t pci_read32(pci_id id, enum pci_reg reg);
void pci_write8(pci_id id, enum pci_reg reg, uint8_t data);
void pci_write16(pci_id id, enum pci_reg reg, uint16_t data);
void pci_write32(pci_id id, enum pci_reg reg, uint32_t data);

enum pci_bar_flag {
    PCI_BAR_IO                     = 0x01,
    PCI_BAR_LOWMEM                 = 0x02,
    PCI_BAR_64                     = 0x04,
    PCI_BAR_PREFETCH               = 0x08
};

struct pci_bar {
    union {
        void *address;
        uint16_t port;
    } u;
    uint64_t size;
    uint32_t flags;
};

void pci_get_bar(struct pci_bar *bar, pci_id id, uint32_t index);

enum pci_vendor {
    PCI_VENDOR_INTEL = 0x8086, 
    PCI_VENDOR_NONE = 0xffff
};

enum pci_base_class {
    PCI_CLASS_LEGACY            = 0x00,
    PCI_CLASS_STORAGE           = 0x01,
    PCI_CLASS_NETWORK           = 0x02,
    PCI_CLASS_DISPLAY           = 0x03,
    PCI_CLASS_MULTIMEDIA        = 0x04,
    PCI_CLASS_MEMORY            = 0x05,
    PCI_CLASS_BRIDGE_DEVICE     = 0x06,
    PCI_CLASS_COMMUNICATION     = 0x07,
    PCI_CLASS_PERIHPERALS       = 0x08,
    PCI_CLASS_INPUT_DEVICES     = 0x09,
    PCI_CLASS_DOCKING_STATION   = 0x0a,
    PCI_CLASS_PROCESSOR         = 0x0b,
    PCI_CLASS_SERIAL_BUS        = 0x0c,
    PCI_CLASS_WIRELESS          = 0x0d,
    PCI_CLASS_INTELLIGENT_IO    = 0x0e,
    PCI_CLASS_SATELLITE         = 0x0f,
    PCI_CLASS_CRYPT             = 0x10,
    PCI_CLASS_SIGNAL_PROCESSING = 0x11,
    PCI_CLASS_UNDEFINED         = 0xff
};
enum pci_full_class {
// Undefined Class
    PCI_UNCLASSIFIED                = 0x0000,
    PCI_VGA_COMPATIBLE              = 0x0001,

// Mass Storage Controller
    PCI_STORAGE_SCSI                = 0x0100,
    PCI_STORAGE_IDE                 = 0x0101,
    PCI_STORAGE_FLOPPY              = 0x0102,
    PCI_STORAGE_IPI                 = 0x0103,
    PCI_STORAGE_RAID                = 0x0104,
    PCI_STORAGE_ATA                 = 0x0105,
    PCI_STORAGE_SATA                = 0x0106,
    PCI_STORAGE_OTHER               = 0x0180,

// Network Controller
    PCI_NETWORK_ETHERNET            = 0x0200,
    PCI_NETWORK_TOKEN_RING          = 0x0201,
    PCI_NETWORK_FDDI                = 0x0202,
    PCI_NETWORK_ATM                 = 0x0203,
    PCI_NETWORK_ISDN                = 0x0204,
    PCI_NETWORK_WORLDFIP            = 0x0205,
    PCI_NETWORK_PICGMG              = 0x0206,
    PCI_NETWORK_OTHER               = 0x0280,

// Display Controller
    PCI_DISPLAY_VGA                 = 0x0300,
    PCI_DISPLAY_XGA                 = 0x0301,
    PCI_DISPLAY_3D                  = 0x0302,
    PCI_DISPLAY_OTHER               = 0x0380,

// Multimedia Controller
    PCI_MULTIMEDIA_VIDEO            = 0x0400,
    PCI_MULTIMEDIA_AUDIO            = 0x0401,
    PCI_MULTIMEDIA_PHONE            = 0x0402,
    PCI_MULTIMEDIA_AUDIO_DEVICE     = 0x0403,
    PCI_MULTIMEDIA_OTHER            = 0x0480,

// Memory Controller
    PCI_MEMORY_RAM                  = 0x0500,
    PCI_MEMORY_FLASH                = 0x0501,
    PCI_MEMORY_OTHER                = 0x0580,

// Bridge Device
    PCI_BRIDGE_HOST                 = 0x0600,
    PCI_BRIDGE_ISA                  = 0x0601,
    PCI_BRIDGE_EISA                 = 0x0602,
    PCI_BRIDGE_MCA                  = 0x0603,
    PCI_BRIDGE_PCI                  = 0x0604,
    PCI_BRIDGE_PCMCIA               = 0x0605,
    PCI_BRIDGE_NUBUS                = 0x0606,
    PCI_BRIDGE_CARDBUS              = 0x0607,
    PCI_BRIDGE_RACEWAY              = 0x0608,
    PCI_BRIDGE_OTHER                = 0x0680,

// Simple Communication Controller
    PCI_COMM_SERIAL                 = 0x0700,
    PCI_COMM_PARALLEL               = 0x0701,
    PCI_COMM_MULTIPORT              = 0x0702,
    PCI_COMM_MODEM                  = 0x0703,
    PCI_COMM_GPIB                   = 0x0704,
    PCI_COMM_SMARTCARD              = 0x0705,
    PCI_COMM_OTHER                  = 0x0780,

// Base System Peripherals
    PCI_SYSTEM_PIC                  = 0x0800,
    PCI_SYSTEM_DMA                  = 0x0801,
    PCI_SYSTEM_TIMER                = 0x0802,
    PCI_SYSTEM_RTC                  = 0x0803,
    PCI_SYSTEM_PCI_HOTPLUG          = 0x0804,
    PCI_SYSTEM_SD                   = 0x0805,
    PCI_SYSTEM_OTHER                = 0x0880,

// Input Devices
    PCI_INPUT_KEYBOARD              = 0x0900,
    PCI_INPUT_PEN                   = 0x0901,
    PCI_INPUT_MOUSE                 = 0x0902,
    PCI_INPUT_SCANNER               = 0x0903,
    PCI_INPUT_GAMEPORT              = 0x0904,
    PCI_INPUT_OTHER                 = 0x0980,

// Docking Stations
    PCI_DOCKING_GENERIC             = 0x0a00,
    PCI_DOCKING_OTHER               = 0x0a80,

// Processors
    PCI_PROCESSOR_386               = 0x0b00,
    PCI_PROCESSOR_486               = 0x0b01,
    PCI_PROCESSOR_PENTIUM           = 0x0b02,
    PCI_PROCESSOR_ALPHA             = 0x0b10,
    PCI_PROCESSOR_POWERPC           = 0x0b20,
    PCI_PROCESSOR_MIPS              = 0x0b30,
    PCI_PROCESSOR_CO                = 0x0b40,

// Serial Bus Controllers
    PCI_SERIAL_FIREWIRE             = 0x0c00,
    PCI_SERIAL_ACCESS               = 0x0c01,
    PCI_SERIAL_SSA                  = 0x0c02,
    PCI_SERIAL_USB                  = 0x0c03,
    PCI_SERIAL_FIBER                = 0x0c04,
    PCI_SERIAL_SMBUS                = 0x0c05,

// Wireless Controllers
    PCI_WIRELESS_IRDA               = 0x0d00,
    PCI_WIRLESSS_IR                 = 0x0d01,
    PCI_WIRLESSS_RF                 = 0x0d10,
    PCI_WIRLESSS_BLUETOOTH          = 0x0d11,
    PCI_WIRLESSS_BROADBAND          = 0x0d12,
    PCI_WIRLESSS_ETHERNET_A         = 0x0d20,
    PCI_WIRLESSS_ETHERNET_B         = 0x0d21,
    PCI_WIRELESS_OTHER              = 0x0d80,

// Intelligent I/O Controllers
    PCI_INTELLIGENT_I2O             = 0x0e00,

// Satellite Communication Controllers
    PCI_SATELLITE_TV                = 0x0f00,
    PCI_SATELLITE_AUDIO             = 0x0f01,
    PCI_SATELLITE_VOICE             = 0x0f03,
    PCI_SATELLITE_DATA              = 0x0f04,

// Encryption/Decryption Controllers
    PCI_CRYPT_NETWORK               = 0x1000,
    PCI_CRYPT_ENTERTAINMENT         = 0x1001,
    PCI_CRYPT_OTHER                 = 0x1080,

// Data Acquisition and Signal Processing Controllers
    PCI_SP_DPIO                     = 0x1100,
    PCI_SP_OTHER                    = 0x1180
};
enum pci_usb_prog_if {
    PCI_SERIAL_USB_UHCI             = 0x00,
    PCI_SERIAL_USB_OHCI             = 0x10,
    PCI_SERIAL_USB_EHCI             = 0x20,
    PCI_SERIAL_USB_XHCI             = 0x30,
    PCI_SERIAL_USB_OTHER            = 0x80
};
#define PCI_SERIAL_ANY              0xFF
static inline void pci_split_class(enum pci_full_class full, uint8_t* base, uint8_t* sub) {
    if (base) *base = ((uint16_t)full >> 8) & 0xFF;
    if (sub) *sub = (uint16_t)full & 0xFF;
}

void pci_enable(pci_id id);

#endif
