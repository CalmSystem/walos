#include <w/stdio.h>
#include <w/log.h>

const struct w_ciovec hello_io_vs[] = {
	{"Hello ", 6}, {"world", 5}
};
const struct w_ciovec hello_log_vs[] = {
	{"Hello ", 6}, {"log", 3}
};
void _start() {
	stdio_write(hello_io_vs, lengthof(hello_io_vs));
	stdio_putc('!');
	stdio_putc('\n');
	sys_log(WL_INFO, hello_log_vs, lengthof(hello_log_vs));
}
