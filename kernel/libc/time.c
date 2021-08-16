#include "time.h"
#include "sys/time8.h"
#include "errno.h"
#include "stdio.h"
#include <limits.h>
#include "../interrupt.h"
#include "../peripherals.h"

#define NANO_FREQ 1000000000

#define TM_YEAR_OFFSET 1900ll
time_t __year_to_secs(long long year, int *is_leap) {
	if (year-2ULL <= 136) {
		int y = year;
		int leaps = (y-68)>>2;
		if (!((y-68)&3)) {
			leaps--;
			if (is_leap) *is_leap = 1;
		} else if (is_leap) *is_leap = 0;
		return 31536000*(y-70) + 86400*leaps;
	}

	int cycles, centuries, leaps, rem;

	if (!is_leap) is_leap = &(int){0};
	cycles = (year-100) / 400;
	rem = (year-100) % 400;
	if (rem < 0) {
		cycles--;
		rem += 400;
	}
	if (!rem) {
		*is_leap = 1;
		centuries = 0;
		leaps = 0;
	} else {
		if (rem >= 200) {
			if (rem >= 300) centuries = 3, rem -= 300;
			else centuries = 2, rem -= 200;
		} else {
			if (rem >= 100) centuries = 1, rem -= 100;
			else centuries = 0;
		}
		if (!rem) {
			*is_leap = 0;
			leaps = 0;
		} else {
			leaps = rem / 4U;
			rem %= 4U;
			*is_leap = !rem;
		}
	}

	leaps += 97*cycles + 24*centuries - *is_leap;

	return (year-100) * 31536000LL + leaps * 86400LL + 946684800 + 86400;
}
static inline int __month_to_secs(int month, int is_leap) {
	static const int secs_through_month[] = {
		0, 31*86400, 59*86400, 90*86400,
		120*86400, 151*86400, 181*86400, 212*86400,
		243*86400, 273*86400, 304*86400, 334*86400 };
	int t = secs_through_month[month];
	if (is_leap && month >= 2) t+=86400;
	return t;
}

time_t mktime8(const struct tm8_t* tm) {
	int is_leap;
	long long year = tm->year + (long)tm->century * 100 - TM_YEAR_OFFSET;
	int month = tm->month - 1;
	if (month >= 12 || month < 0) {
		int adj = month / 12;
		month %= 12;
		if (month < 0) {
			adj--;
			month += 12;
		}
		year += adj;
	}
	time_t t = __year_to_secs(year, &is_leap);
	t += __month_to_secs(month, is_leap);
	t += 86400LL * (tm->day - 1);
	t += 3600LL * tm->hour;
	t += 60LL * tm->minute;
	t += tm->second;
	return t;
}
/* 2000-03-01 (mod 400 year, immediately after feb29 */
#define LEAPOCH (946684800LL + 86400*(31+29))

#define DAYS_PER_400Y (365*400 + 97)
#define DAYS_PER_100Y (365*100 + 24)
#define DAYS_PER_4Y   (365*4   + 1)

struct tm8_t* gmtime8(time_t t, struct tm8_t* tm) {
	long long days, secs, years;
	int remdays, remsecs, remyears;
	int qc_cycles, c_cycles, q_cycles;
	int months;
	int leap;
	static const char days_in_month[] = {31,30,31,30,31,31,30,31,30,31,31,29};

	/* Reject time_t values whose year would overflow int */
	if (t < INT_MIN * 31622400LL || t > INT_MAX * 31622400LL)
		return 0;

	secs = t - LEAPOCH;
	days = secs / 86400;
	remsecs = secs % 86400;
	if (remsecs < 0) {
		remsecs += 86400;
		days--;
	}

	/* No week day in current format
	wday = (3+days)%7;
	if (wday < 0) wday += 7;
	*/

	qc_cycles = days / DAYS_PER_400Y;
	remdays = days % DAYS_PER_400Y;
	if (remdays < 0) {
		remdays += DAYS_PER_400Y;
		qc_cycles--;
	}

	c_cycles = remdays / DAYS_PER_100Y;
	if (c_cycles == 4) c_cycles--;
	remdays -= c_cycles * DAYS_PER_100Y;

	q_cycles = remdays / DAYS_PER_4Y;
	if (q_cycles == 25) q_cycles--;
	remdays -= q_cycles * DAYS_PER_4Y;

	remyears = remdays / 365;
	if (remyears == 4) remyears--;
	remdays -= remyears * 365;

	leap = !remyears && (q_cycles || !c_cycles);
	/* No year day in current format
	yday = remdays + 31 + 28 + leap;
	if (yday >= 365+leap) yday -= 365+leap;
	*/

	years = remyears + 4*q_cycles + 100*c_cycles + 400LL*qc_cycles;

	for (months=0; days_in_month[months] <= remdays; months++)
		remdays -= days_in_month[months];

	if (months >= 10) {
		months -= 12;
		years++;
	}

	if (years+100 > INT_MAX || years+100 < INT_MIN)
		return 0;

	tm->century = years / 100 + 20;
	tm->year = years % 100;
	tm->month = months + 3;
	tm->day = remdays + 1;

	tm->hour = remsecs / 3600;
	tm->minute = remsecs / 60 % 60;
	tm->second = remsecs % 60;

	return tm;
}

char* asctime8(const struct tm8_t* tm) {
	const size_t buf_size = 20;
	static char buf[buf_size+1];
	if (snprintf(buf, buf_size, "%0.4d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d",
		(int)tm->century * 100 + tm->year, tm->month, tm->day,
		tm->hour, tm->minute, tm->second
	) > buf_size)
		return 0;

	return buf;
}

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
		struct tm8_t tm;
		if (!rtc_get(&tm))
			return -1;
		if (tp) {
			tp->tv_nsec = 0;
			tp->tv_sec = mktime8(&tm);
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
