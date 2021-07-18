#include "interrupt.h"
#include "stdint.h"
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

struct idtr_t {
    uint16_t size;
    uintptr_t offset;
} __attribute__((packed));
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

void set_interrupt(uint8_t i, uint8_t attrs, uint64_t addr) {
    idt[i].ptr_low = addr;
    idt[i].ptr_middle = addr >> 16;
    idt[i].ptr_high = addr >> 32;

    idt[i].attributes = attrs;

    idt[i].gdt_selector = 0x38;
    idt[i].ist = 0;
    idt[i].reserved = 0;
}

#define QUARTZ 0x1234DD
#define SCHEDFREQ 50
#define CLOCKFREQ 200

static inline void outb(unsigned char value, unsigned short port) {
    __asm__ __volatile__("outb %0, %1" : : "a" (value), "Nd" (port));
}
static inline unsigned char inb(unsigned short port) {
    unsigned char rega;
    __asm__ __volatile__("inb %1,%0" : "=a" (rega) : "Nd" (port));
    return rega;
}

static void load_pic() {
    /* Initialize the master. */
    outb(0x11, 0x20);
    outb(0x20, 0x21);
    outb(0x4, 0x21);
    outb(0x1, 0x21);
    /* Initialize the slave. */
    outb(0x11, 0xa0);
    outb(0x28, 0xa1);
    outb(0x2, 0xa1);
    outb(0x1, 0xa1);
    /* Ack any bogus intrs by setting the End Of Interrupt bit. */
    outb(0x20, 0x20);
    outb(0x20, 0xa0);
    /* Disable all IRQs */
    outb(0xff, 0x21);
    outb(0xff, 0xa1);
}
static void load_pit() {
    const int interval = QUARTZ / CLOCKFREQ;
    outb(0x34, 0x43);
    outb(interval % 256, 0x40);
    outb(interval / 256, 0x40);
}
unsigned long pit_count = 0;
void tic_PIT() {
    outb(0x20, 0x20);
    pit_count++;
    const unsigned long seconds = pit_count / CLOCKFREQ;
    char time_str[9]; // space for "HH:MM:SS\0"
    sprintf(time_str, "%02ld:%02ld:%02ld", seconds / (60 * 60), (seconds / 60) % 60, seconds % 60);
    PUTBYTES_STATE pb_state = {
        .bg = RGB_BLACK,
        .fg = PACK_RGB(0, 255, 0)
    };
    uint16_t chrx;
    graphics_size(&pb_state.x, NULL, &chrx, NULL);
    pb_state.x = pb_state.x / chrx - 8;
    putbytes_at(time_str, 8, &pb_state);
}
extern void IT_PIT_handler();

void enable_irq(uint8_t irq, bool on) {
    uint32_t dataport = 0x21;
    if (irq >= 8) {
        dataport = 0xA1;
        irq -= 8;
    }
    uint8_t masks = inb(dataport);
    // Set bit irq to !on
    masks = (masks & ~(1UL << irq)) | ((!on) << irq);
    outb(masks, dataport);
}

void load_interrupts() {

    __asm__ __volatile__("cli":::"memory");

    for (uint32_t i = 0; i < sizeof(idt)/sizeof(idt[0]); i++) {
        idt[i].attributes = 0xE;
    }
    for (uint8_t i = 0; i < sizeof(exception_handlers)/sizeof(exception_handlers[0]); i++) {
        set_interrupt(i, INTGATE, exception_handlers[i]);
    }

    load_pic();

    set_interrupt(32, INTGATE, (uint64_t)IT_PIT_handler);
    load_pit();
    enable_irq(0, true);

    __asm__ volatile ("lidt (%0)" : : "r" (&idt_descriptor));

}
