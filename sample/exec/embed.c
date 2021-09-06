#include <w/sys.h>

static const w_ciovec hello = {"Hello from sys:exec", 20};
void _start() {
	sys_log(WL_NOTICE, &hello, 1);
}
