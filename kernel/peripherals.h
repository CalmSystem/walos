#ifndef __PERIPHERALS_H
#define __PERIPHERALS_H

#include "stdint.h"
uint8_t cmos_read(uint16_t reg);

struct tm8_t* rtc_get(struct tm8_t* time);

uint32_t true_rand(void);

#endif
