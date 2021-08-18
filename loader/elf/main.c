#include <kernel/os.h>
#include <stdio.h>
#include <unistd.h>

static int loader_log(struct w_argvec in, struct w_argvec out) {
	if (in.len != 2 || out.len) return -1;
	cstr str = in.ptr[0];
	const uintw_t len = *(const uintw_t *)in.ptr[1];
	for (uintw_t i = 0; i < len && *str; i++)
		putchar(*(str++));

	return 0;
}
static int loader_wait(struct w_argvec in, struct w_argvec out) {
	if (in.len || out.len) return -1;
	//TODO: wait for event
	sleep(1); /* Sleep for now */
	return 0;
}

int main(int argc, char const *argv[]) {
	puts("ELF loader");

	module_fn mfn[] = {
		DECL_MOD_FN_NATIVE("loader", "log", loader_log, 1),
		DECL_MOD_FN_NATIVE("loader", "wait", loader_wait, 1)
	};
	struct os_ctx_t os_ctx = {mfn, sizeof(mfn) / sizeof(mfn[0])};
	os_entry(&os_ctx);
	puts("OS Stopped");
	return 0;
}
