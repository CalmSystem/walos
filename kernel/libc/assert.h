#ifndef __ASSERT_H
#define __ASSERT_H

extern void panic(const char *fmt, ...) __attribute__((noreturn, format (printf, 1, 2)));

#define BUG() do { panic(__FILE__":%u: BUG !\n", __LINE__); } while (0)

#ifdef NDEBUG

#define assert(expr) ((void)0)

#else

#define assert(expr) \
	((void)((expr) ? 0 : \
		(panic(__FILE__":%u: failed assertion `"#expr"'\n", \
			__LINE__), 0)))

#endif

#endif
