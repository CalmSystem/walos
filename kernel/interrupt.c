#include "interrupt.h"
#include "asm.h"
#include "stdbool.h"
#include "stdio.h"

/** interrupt table entry */
struct idt_entry_t {
    uint16_t ptr_low;
    uint16_t gdt_selector;
    uint8_t  ist;
    uint8_t  attributes;
    uint16_t ptr_middle;
    uint32_t ptr_high;
    uint32_t reserved;
} __attribute__((packed));
struct idt_entry_t idt[256] = {0};

struct idtr_t idt_descriptor = {
    .size = sizeof(idt),
    .offset = (uint64_t)&idt[0],
};

#define INTGATE 0x8E

/** Handlers definided in handlers.S */
extern uint64_t exception_handlers[32];

#include "graphics.h"
static void write_hex(int length, unsigned long value, PUTBYTES_STATE* pb_state) {
    static char map[16];
    if (!map[0]) {
        int i;
        for (i=0; i<10; i++) map[i] = '0' + i;
        for (i=0; i<6; i++) map[i + 10] = 'A' + i;
    }
    pb_state->x += length-1;
    uint32_t dest = pb_state->x;
    while (length-- > 0) {
        putbytes_at(&map[value & 15], 1, pb_state);
        pb_state->x -= 2;
        value >>= 4;
    }
    pb_state->x = dest+1;
}
void trap_handler(uint64_t trapno, uint64_t error_code) {
    PUTBYTES_STATE pb_state = {
        .bg = RGB_BLACK,
        .fg = PACK_RGB(255, 0, 0)
    };
    putbytes_at("----------------------------\n", 29, &pb_state);
    putbytes_at("Kernel trap#", 13, &pb_state);
    write_hex(2, trapno, &pb_state);
    putbytes_at(" code#", 6, &pb_state);
    write_hex(8, error_code, &pb_state);
    putbytes_at("\n----------------------------\n\n", 31, &pb_state);
}

void interrupt_set_handler(uint8_t i, uint8_t attrs, uint64_t addr) {
    idt[i].ptr_low = addr;
    idt[i].ptr_middle = addr >> 16;
    idt[i].ptr_high = addr >> 32;

    idt[i].attributes = attrs;

    idt[i].gdt_selector = 0x38;
    idt[i].ist = 0;
    idt[i].reserved = 0;
}

#define IRQ_HIGH 8
#define QUARTZ 0x1234DD

/* IO base address for master PIC */
#define PIC1_CMND 0x20
#define PIC1_DATA 0x21
/* IO base address for slave PIC */
#define PIC2_CMND 0xA0
#define PIC2_DATA 0xA1

#define PIC_EOI 0x20 /* End-of-interrupt command code */

static void pic_setup() {
    /* Initialize the master. */
    io_write8(PIC1_CMND, 0x11);
    io_write8(PIC1_DATA, 0x20);
    io_write8(PIC1_DATA, 0x4);
    io_write8(PIC1_DATA, 0x1);
    /* Initialize the slave. */
    io_write8(PIC2_CMND, 0x11);
    io_write8(PIC2_DATA, 0x28);
    io_write8(PIC2_DATA, 0x2);
    io_write8(PIC2_DATA, 0x1);
    /* Ack any bogus intrs by setting the End Of Interrupt bit. */
    irq_accept(IRQ_HIGH);
    /* Disable all Is*/
    io_write8(PIC1_DATA, 0xff);
    io_write8(PIC2_DATA, 0xff);
}

static void pit_setup() {
    const int interval = QUARTZ / PIT_FREQ;
    io_write8(0x43, 0x34);
    io_write8(0x40, interval % 256);
    io_write8(0x40, interval / 256);
}
unsigned long pit_count = 0;
void tic_PIT() {
    irq_accept(0);
    pit_count++;
    const unsigned long seconds = pit_count / PIT_FREQ;
    char time_str[9]; // space for "HH:MM:SS\0"
    sprintf(time_str, "%02ld:%02ld:%02ld", seconds / (60 * 60), (seconds / 60) % 60, seconds % 60);
    PUTBYTES_STATE pb_state = {
        .bg = RGB_BLACK,
        .fg = PACK_RGB(0, 255, 0)
    };
    uint16_t chrx;
    graphics_get_size(&pb_state.x, NULL, &chrx, NULL);
    pb_state.x = pb_state.x / chrx - 8;
    putbytes_at(time_str, 8, &pb_state);
}
extern void IT_PIT_handler();
unsigned long pit_get_count() { return pit_count; }

void irq_enable(uint8_t irq, bool on) {
    uint32_t dataport = PIC1_DATA;
    if (irq >= IRQ_HIGH) {
        dataport = PIC2_DATA;
        irq -= IRQ_HIGH;
    }
    uint8_t masks = io_read8(dataport);
    // Set bit irq to !on
    masks = (masks & ~(1UL << irq)) | ((!on) << irq);
    io_write8(dataport, masks);
}
/* End-of-interrupt command code */
#define PIC_EOI 0x20
void irq_accept(uint8_t irq) {
    if (irq >= IRQ_HIGH)
        io_write8(PIC2_CMND, PIC_EOI);

    io_write8(PIC1_CMND, PIC_EOI);
}

void interrupts_setup() {

    cli();

    for (uint32_t i = 0; i < sizeof(idt)/sizeof(idt[0]); i++) {
        idt[i].attributes = 0xE;
    }
    for (uint8_t i = 0; i < sizeof(exception_handlers)/sizeof(exception_handlers[0]); i++) {
        interrupt_set_handler(i, INTGATE, exception_handlers[i]);
    }

    pic_setup();

    interrupt_set_handler(32, INTGATE, (uint64_t)IT_PIT_handler);
    pit_setup();
    irq_enable(0, true);

    idt_load(&idt_descriptor);

}
