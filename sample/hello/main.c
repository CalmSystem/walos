#include <stdout.h>

const struct w_ciovec hello_vs[] = {
	{"Hello", 5},
	{" world", 6}
};
void _start() {
	stdout_write(hello_vs, sizeof(hello_vs)/sizeof(hello_vs[0]));
	stdout_putc('!');
	stdout_putc('\n');
}
