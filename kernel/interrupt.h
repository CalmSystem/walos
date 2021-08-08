#ifndef __INTERRUPT_H
#define __INTERRUPT_H

#include "stdint.h"

void interrupts_setup();

/** PIT ticks per second */
#define PIT_FREQ 200
/** PIT ticks since kernel start */
unsigned long pit_get_count(void);

/** Mark IRQ as received */
void irq_accept(uint8_t irq);

#endif
