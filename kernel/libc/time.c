#include "time.h"
#include "errno.h"
#include <limits.h>
#include "../interrupt.h"
#include "../peripherals.h"

#define NANO_FREQ 1000000000

int clock_getres(clockid_t clk, struct timespec *ts) {
    if (clk < CLOCK_REALTIME || clk > CLOCK_THREAD_CPUTIME_ID)
        return -EINVAL;
    if (ts) {
        ts->tv_nsec = clk != CLOCK_REALTIME;
        ts->tv_sec = clk == CLOCK_REALTIME;
    }
    return 0;
}

int clock_gettime(clockid_t clockid, struct timespec *tp) {
    switch (clockid)
    {
    case CLOCK_REALTIME: {
        struct u8_datetime_t time;
        rtc_read(&time);
        if (tp) {
            tp->tv_nsec = 0;
            tp->tv_sec = time_u8_to_secs(&time);
        }
        return 0;
    }

    case CLOCK_MONOTONIC: {
        unsigned long pit = pit_get_count();
        if (tp) {
            tp->tv_sec = pit / PIT_FREQ;
            tp->tv_nsec = (pit % PIT_FREQ) * NANO_FREQ / PIT_FREQ;
        }
        return 0;
    }

    case CLOCK_PROCESS_CPUTIME_ID:
    case CLOCK_THREAD_CPUTIME_ID:
        //TODO: process timer
        return clock_gettime(CLOCK_MONOTONIC, tp);

    default:
        return -EINVAL;
    }
}
int clock_settime(clockid_t clockid, const struct timespec *tp) {
    (void)clockid;
    (void)tp;
    //MAYBE: allow overriding realtime (rtc_write)
    return -EINVAL;
}

clock_t clock() {
	struct timespec ts;

	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts))
		return -1;

	if (ts.tv_sec > LONG_MAX/1000000
	 || ts.tv_nsec/1000 > LONG_MAX-1000000*ts.tv_sec)
		return -1;

	return ts.tv_sec*1000000 + ts.tv_nsec/1000;
}
