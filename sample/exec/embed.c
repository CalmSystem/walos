#include <w/main>

W_MAIN() {
	static const w_ciovec hello[] = W_IOBUF("Hello from sys:exec");
	sys_log(WL_NOTICE, hello, 1);
}
