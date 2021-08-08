#include "acpi.h"
#include "stddef.h"
#include "string.h"

static inline bool sum_check(uint8_t *p, size_t length) {
    uint8_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += p[i];
    }
    return sum == 0;
}

struct sdt_header {
  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  char OEMID[6];
  char OEMTableID[8];
  uint32_t OEMRevision;
  uint32_t creatorID;
  uint32_t creator_revision;
};
/** RSDT or XSDT */
struct root_sdt {
  struct sdt_header* it;
  /** Is XSDT */
  bool xtended;
};

#define SDT_FIND(uintx_t) { \
    uintx_t* p = (uintx_t*)(r.it + 1); \
    uintx_t* end = (uintx_t*)((uint8_t*)r.it + r.it->length); \
    while (p < end) { \
        struct sdt_header* h = (struct sdt_header*)(uint64_t)*p; \
        if (!strncmp(h->signature, signature, 4) && sum_check((uint8_t*)h, h->length)) \
            return h; \
        p++; \
    } \
    return NULL; \
}
static void* sdt_find(struct root_sdt r, const char signature[4]) {
    if (r.xtended) SDT_FIND(uint64_t)
    else SDT_FIND(uint32_t);
}

#define RSDP_SIGN "RSD PTR "
struct rsdp_descriptor {
 char Signature[8];
 uint8_t Checksum;
 char OEMID[6];
 uint8_t Revision;
 uint32_t RsdtAddress;
} __attribute__ ((packed));
struct rsdp_descriptor2 {
 struct rsdp_descriptor top;
 uint32_t length;
 uint64_t xsdt_address;
 uint8_t extended_checksum;
 uint8_t reserved[3];
} __attribute__ ((packed));

static struct root_sdt g_acpi_root = {0};
bool acpi_load_root(void* p) {
    if (g_acpi_root.it) return true;

    struct rsdp_descriptor* rsdp = (struct rsdp_descriptor*)p;

    if (!rsdp || !sum_check((uint8_t*)rsdp, sizeof(*rsdp))) return false;

    if (rsdp->Revision == 2) {
        struct rsdp_descriptor2* rsdp2 = (struct rsdp_descriptor2*)rsdp;
        if (!sum_check((uint8_t*)rsdp2, sizeof(*rsdp2))) return false;

        if (rsdp2->xsdt_address) {
            g_acpi_root.xtended = true;
            g_acpi_root.it = (struct sdt_header*)rsdp2->xsdt_address;
            return true;
        }
    }

    if (rsdp->Revision != 0 && rsdp->Revision != 2) return false;

    g_acpi_root.xtended = false;
    g_acpi_root.it = (struct sdt_header*)(uint64_t)rsdp->RsdtAddress;
    return true;
}

struct generic_address {
    uint8_t AddressSpace;
    uint8_t BitWidth;
    uint8_t BitOffset;
    uint8_t AccessSize;
    uint64_t Address;
};

#define FADT_SIGN "FACP"
struct fadt {
    struct   sdt_header h;
    uint32_t FirmwareCtrl;
    uint32_t Dsdt;

    // field used in ACPI 1.0; no longer in use, for compatibility only
    uint8_t  Reserved;

    uint8_t  PreferredPowerManagementProfile;
    uint16_t SCI_Interrupt;
    uint32_t SMI_CommandPort;
    uint8_t  AcpiEnable;
    uint8_t  AcpiDisable;
    uint8_t  S4BIOS_REQ;
    uint8_t  PSTATE_Control;
    uint32_t PM1aEventBlock;
    uint32_t PM1bEventBlock;
    uint32_t PM1aControlBlock;
    uint32_t PM1bControlBlock;
    uint32_t PM2ControlBlock;
    uint32_t PMTimerBlock;
    uint32_t GPE0Block;
    uint32_t GPE1Block;
    uint8_t  PM1EventLength;
    uint8_t  PM1ControlLength;
    uint8_t  PM2ControlLength;
    uint8_t  PMTimerLength;
    uint8_t  GPE0Length;
    uint8_t  GPE1Length;
    uint8_t  GPE1Base;
    uint8_t  CStateControl;
    uint16_t WorstC2Latency;
    uint16_t WorstC3Latency;
    uint16_t FlushSize;
    uint16_t FlushStride;
    uint8_t  DutyOffset;
    uint8_t  DutyWidth;
    uint8_t  DayAlarm;
    uint8_t  MonthAlarm;
    uint8_t  Century;

    // reserved in ACPI 1.0; used since ACPI 2.0+
    uint16_t BootArchitectureFlags;

    uint8_t  Reserved2;
    uint32_t Flags;

    // 12 byte structure; see below for details
    struct generic_address ResetReg;

    uint8_t  ResetValue;
    uint8_t  Reserved3[3];

    // 64bit pointers - Available on ACPI 2.0+
    uint64_t                X_FirmwareControl;
    uint64_t                X_Dsdt;

    struct generic_address X_PM1aEventBlock;
    struct generic_address X_PM1bEventBlock;
    struct generic_address X_PM1aControlBlock;
    struct generic_address X_PM1bControlBlock;
    struct generic_address X_PM2ControlBlock;
    struct generic_address X_PMTimerBlock;
    struct generic_address X_GPE0Block;
    struct generic_address X_GPE1Block;
};

#define MADT_SIGN "APIC"
struct madt_header {
    struct sdt_header h;
    uint32_t localApicAddr;
    uint32_t flags;
};
struct apic_entry {
    uint8_t type;
    uint8_t length;
};

bool apic_read(enum apic_type type, void** entries, uint32_t* nentries) {
    if (!g_acpi_root.it) return false;
    struct madt_header* madt = (struct madt_header*)sdt_find(g_acpi_root, MADT_SIGN);
    if (!madt) return false;

    uint8_t* p = (uint8_t*)(madt + 1);
    uint8_t* end = (uint8_t*)madt + madt->h.length;
    uint32_t cur_entry = 0;
    while (p < end) {
        struct apic_entry *header = (struct apic_entry*)p;
        if (header->type == type) {
            if (entries && cur_entry < *nentries)
                entries[cur_entry] = p+1;

            cur_entry++;
        }
        p += header->length;
    }
    *nentries = cur_entry;
    return true;
}
