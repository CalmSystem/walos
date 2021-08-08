#ifndef __ACPI_H
#define __ACPI_H

#include "stdint.h"
#include "stdbool.h"

bool acpi_load_root(void* p);

struct apic_local {
    uint8_t acpiProcessorId;
    uint8_t apicId;
    uint32_t flags;
} __attribute__ ((packed));
struct apic_io {
    uint8_t ioApicId;
    uint8_t reserved;
    uint32_t ioApicAddress;
    uint32_t globalSystemInterruptBase;
} __attribute__ ((packed));
struct apic_interrupt_override {
    uint8_t bus;
    uint8_t source;
    uint32_t interrupt;
    uint16_t flags;
} __attribute__ ((packed));
enum apic_type {
    APIC_TYPE_LOCAL                 = 0,
    APIC_TYPE_IO                    = 1,
    APIC_TYPE_INTERRUPT_OVERRIDE    = 2,
    APIC_TYPE_PHYSICAL              = 9
};
bool apic_read(enum apic_type type, void** entries, uint32_t* nentries);

#endif
