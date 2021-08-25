#include "lib.h"
#include <kernel/log.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include "libc/doprnt.h"

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

static inline void log(cstr str, unsigned len) {
	loader_get_handle()->log(str, len);
}

static inline void prefix(enum klog_level lvl, cstr ctx) {
	log(LVL_S[lvl], strlen(LVL_S[0]));
	log(" [", 2);
	log(ctx, strlen(ctx));
	log("] ", 2);
}
static inline void suffix(bool ln) {
#if ANSI_COLOR
	log("\x1B[0m\n", 4 + ln);
#else
	if (ln) log("\n", 1);
#endif
}

void klogs(enum klog_level lvl, cstr ctx, cstr str) {
	prefix(lvl, ctx);
	int len = strlen(str);
	log(str, len);
	suffix(str[len] != '\n');
}

static void savechar(char *arg, int c) {
	static char buf[32];
	static size_t buf_size = 0;
	if (buf_size && (!c || buf_size >= sizeof(buf))) {
		log(buf, buf_size);
		buf_size = 0;
	}
	if (c) buf[buf_size++] = c;
}
void klogf(enum klog_level lvl, cstr ctx, cstr fmt, ...) {
	va_list args;

	prefix(lvl, ctx);
	va_start(args, fmt);
	_doprnt(fmt, args, 0, (void (*)())savechar, NULL);
	va_end(args);
	savechar(NULL, 0);
	suffix(false);

	return;
}
