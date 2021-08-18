#include <efi/efi-tools.h>
#include <kernel/os.h>

static int loader_log(struct w_argvec in, struct w_argvec out) {
	if (in.len != 2 || out.len) return -1;
	putsn(in.ptr[0], *(const uintw_t *)in.ptr[1]);
	return 0;
}
static inline void push_watchdog_timer() {
	system_table->BootServices->SetWatchdogTimer(60, 0, 13, L"OS timed out");
}
static int loader_wait(struct w_argvec in, struct w_argvec out) {
	if (in.len || out.len) return -1;
	push_watchdog_timer(); /* Expand for a minute */
	//TODO: wait for event
	system_table->BootServices->Stall(1000); /* Sleep for now */
	return 0;
}

EFI_STATUS efi_main(EFI_HANDLE ih, EFI_SYSTEM_TABLE *st) {
	system_table = st;
	image_handle = ih;

	println(WSTR("EFI APP Loader"));
	push_watchdog_timer();

	module_fn mfn[] = {
		DECL_MOD_FN_NATIVE("loader", "log", loader_log, 1),
		DECL_MOD_FN_NATIVE("loader", "wait", loader_wait, 1)
	};
	struct os_ctx_t os_ctx = {mfn, sizeof(mfn)/sizeof(mfn[0])};
	os_entry(&os_ctx);

	println(WSTR("OS Stopped"));
#if LOADER_SLOW_STOP
	println(WSTR("Waiting 10s..."));
	system_table->BootServices->Stall(10000000);
#endif
	return system_table->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
}