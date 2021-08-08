#ifndef __TIME_H
#define __TIME_H

/* Identifier for system-wide realtime clock.  */
# define CLOCK_REALTIME			0
/* Monotonic system-wide clock.  */
# define CLOCK_MONOTONIC		1
/* High-resolution timer from the CPU.  */
# define CLOCK_PROCESS_CPUTIME_ID	2
/* Thread-specific CPU-time clock.  */
# define CLOCK_THREAD_CPUTIME_ID	3

typedef int clockid_t;
struct timespec {
	long	tv_sec;			/* seconds */
	long	tv_nsec;		/* nanoseconds */
};

int clock_getres(clockid_t clockid, struct timespec *res);

int clock_gettime(clockid_t clockid, struct timespec *tp);
int clock_settime(clockid_t clockid, const struct timespec *tp);

typedef long clock_t;
clock_t clock(void);

#endif
