#ifndef __PERIPHERALS_H
#define __PERIPHERALS_H

#include "stdint.h"
struct u8_datetime_t {
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned char year;
    unsigned char century;
};
void rtc_read(struct u8_datetime_t* time);
unsigned long time_u8_to_secs(const struct u8_datetime_t* time);

uint32_t true_rand(void);

#endif
