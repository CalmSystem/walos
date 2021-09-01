#include <kernel/log_fmt.h>

static inline void llogs(enum w_log_level lvl, cstr str) {
	klog_prefix(llog_out, lvl, "loader");
	int len = strlen(str);
	llog_out(str, len);
	klog_suffix(llog_out, str[len] != '\n');
}
