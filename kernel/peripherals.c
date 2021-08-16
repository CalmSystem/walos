#include "peripherals.h"
#include "asm.h"
#include "sys/time8.h"

#define CMOS_CMD_PORT 0x70
#define CMOS_DTA_PORT 0x71
uint8_t cmos_read(uint16_t reg) {
	preempt_lock_t lock;
	preempt_disable(&lock);
	io_write8(CMOS_CMD_PORT, reg | 0x80);
	uint8_t value = io_read8(CMOS_DTA_PORT);
	preempt_release(lock);
	return value;
}

static inline int rtc_update_in_progress() {
	return cmos_read(0x0A) & 0x80;
}

struct tm8_t* rtc_get(struct tm8_t* time) {
	if (!time) return 0;

	struct tm8_t last;
	uint8_t registerB;

	// Note: This uses the "read registers until you get the same values twice in a row" technique
	//       to avoid getting dodgy/inconsistent values due to RTC updates

	uint8_t cnt = 0;
	do {
		if (cnt > 199) return 0;
		last = *time;

		while (rtc_update_in_progress())
			; // Make sure an update isn't in progress
		time->second = cmos_read(0x00);
		time->minute = cmos_read(0x02);
		time->hour = cmos_read(0x04);
		time->day = cmos_read(0x07);
		time->month = cmos_read(0x08);
		time->year = cmos_read(0x09);
		time->century = cmos_read(0x32);

		registerB = cmos_read(0x0B);
		cnt++;
	} while ((last.second != time->second) || (last.minute != time->minute) || (last.hour != time->hour) ||
			 (last.day != time->day) || (last.month != time->month) || (last.year != time->year) ||
			 (last.century != time->century));

	// Convert BCD to binary values if necessary
	if (!(registerB & 0x04)) {
		time->second = (time->second & 0x0F) + ((time->second / 16) * 10);
		time->minute = (time->minute & 0x0F) + ((time->minute / 16) * 10);
		time->hour = ((time->hour & 0x0F) + (((time->hour & 0x70) / 16) * 10)) | (time->hour & 0x80);
		time->day = (time->day & 0x0F) + ((time->day / 16) * 10);
		time->month = (time->month & 0x0F) + ((time->month / 16) * 10);
		time->year = (time->year & 0x0F) + ((time->year / 16) * 10);
		time->century = (time->century & 0x0F) + ((time->century / 16) * 10);
	}

	// Convert 12 hour clock to 24 hour clock if necessary
	if (!(registerB & 0x02) && (time->hour & 0x80)) {
		time->hour = ((time->hour & 0x7F) + 12) % 24;
	}

	return time;
}

static uint32_t z1, z2, z3, z4;
static int rand_ready = 0;
#define INIT_RAND(seed) if (!rand_ready) { \
	rand_ready = 1; \
	z1 = (seed) + 2; z2 = z1 + 6; \
	z3 = z2 + 6; z4 = z3 + 112; \
}

int rand(void) {
	INIT_RAND(true_rand());
	uint32_t b;
	b  = ((z1 << 6) ^ z1) >> 13;
	z1 = ((z1 & 4294967294U) << 18) ^ b;
	b  = ((z2 << 2) ^ z2) >> 27;
	z2 = ((z2 & 4294967288U) << 2) ^ b;
	b  = ((z3 << 13) ^ z3) >> 21;
	z3 = ((z3 & 4294967280U) << 7) ^ b;
	b  = ((z4 << 3) ^ z4) >> 12;
	z4 = ((z4 & 4294967168U) << 13) ^ b;
	return (z1 ^ z2 ^ z3 ^ z4);
}

uint32_t true_rand() {
	uint32_t out;
	if (cpuid_has_feat_c(CPU_FEAT_C_RDRAND)) {
		int ok;
		__asm__ __volatile__("rdrand %0; sbb %1,%1" : "=a" (out), "=b" (ok));
		if (ok) return out;
	}

	// Fallback
	struct tm8_t t;
	INIT_RAND(mktime8(rtc_get(&t)));
	*(int*)&out = rand();
	return out;
}
