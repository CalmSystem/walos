#include <stdout.h>

const struct w_ciovec hello_vs[] = {
	{"Hello", 5},
	{" world\n", 7}
};
void _start() {
	stdout_write(hello_vs, sizeof(hello_vs)/sizeof(hello_vs[0]));
}
