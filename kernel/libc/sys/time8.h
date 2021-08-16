#ifndef __SYS_TIME8_H
#define __SYS_TIME8_H

#include "../time.h"
struct tm8_t {
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned char year;
    unsigned char century;
};

time_t mktime8(const struct tm8_t*);
struct tm8_t* gmtime8(time_t, struct tm8_t*);
char* asctime8(const struct tm8_t*);

#endif
