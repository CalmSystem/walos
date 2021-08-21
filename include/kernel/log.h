#ifndef __KERNEL_LOG_H
#define __KERNEL_LOG_H
#include "types.h"

enum klog_level {
	KL_EMERG, /* Emergency - System is unusable */
	KL_ALERT, /* Alert - Action must be taken immediately */
	KL_CRIT, /* Critical - System is unstable */
	KL_ERR, /* Error - Something went wrong */
	KL_WARN, /* Warning - Danger may cause problems */
	KL_NOTICE, /* Notice - Normal but significant conditions */
	KL_INFO, /* Informational - Informational messages */
	KL_DEBUG /* Debug - Debug-level messages */
};
void klogs(enum klog_level, cstr ctx, cstr str);
void klogf(enum klog_level, cstr ctx, cstr fmt, ...) __attribute__((format(printf, 3, 4)));

#define __STR__(x) #x
#define __STR(x) __STR__(x)

#if DEBUG
#undef K_CTX
#define K_CTX __FILE__ ":" __STR(__LINE__)
#else
#ifndef K_CTX
#define K_CTX "kernel"
#endif
#endif
#define logs(level, str) klogs(level, K_CTX, str)
#define logf(level, fmt, ...) klogf(level, K_CTX, fmt, __VA_ARGS__)

#endif
