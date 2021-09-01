#ifndef __KERNEL_LOG_FMT_H
#define __KERNEL_LOG_FMT_H
#include "log.h"

static const char *const LVL_S[] = {
#if ANSI_COLOR
	"\x1B[1;41mEMERG ",
	"\x1B[1;31mALERT ",
	"\x1B[0;91mCRIT  ",
	"\x1B[0;31mERR   ",
	"\x1B[0;33mWARN  ",
	"\x1B[0;01mNOTICE",
	"\x1B[0;97mINFO  ",
	"\x1B[0;90mDEBUG "
#else
	"EMERG ",
	"ALERT ",
	"CRIT  ",
	"ERR   ",
	"WARN  ",
	"NOTICE",
	"INFO  ",
	"DEBUG "
#endif
};

typedef void(*klog_write)(cstr, unsigned);
size_t strlen(const char*);
static inline void klog_prefix(klog_write log, enum w_log_level lvl, cstr ctx) {
	log(LVL_S[lvl], strlen(LVL_S[0]));
	log(" [", 2);
	log(ctx, strlen(ctx));
	log("] ", 2);
}
static inline void klog_suffix(klog_write log, bool ln) {
#if ANSI_COLOR
	log("\x1B[0m\n", 4 + ln);
#else
	if (ln) log("\n", 1);
#endif
}

#endif
