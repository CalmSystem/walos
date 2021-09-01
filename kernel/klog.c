#include "lib.h"
#include <kernel/log_fmt.h>
#include <stddef.h>
#include <stdarg.h>
#include "libc/doprnt.h"

static inline void log(cstr str, unsigned len) {
	loader_get_handle()->log(str, len);
}

void klogs(enum w_log_level lvl, cstr ctx, cstr str) {
	klog_prefix(log, lvl, ctx);
	int len = strlen(str);
	log(str, len);
	klog_suffix(log, str[len] != '\n');
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
void klogf(enum w_log_level lvl, cstr ctx, cstr fmt, ...) {
	va_list args;

	klog_prefix(log, lvl, ctx);
	va_start(args, fmt);
	_doprnt(fmt, args, 0, (void (*)())savechar, NULL);
	va_end(args);
	savechar(NULL, 0);
	klog_suffix(log, false);

	return;
}
