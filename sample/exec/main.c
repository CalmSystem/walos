#include <w/main>
#include <w/proc.h>
#include <embed.wasm.h>

uint8_t _binary_embed_wasm[];

W_MAIN() {
	proc_exec("embed", 6, _binary_embed_wasm, sizeof(_binary_embed_wasm));
	static const w_ciovec done[] = W_IOBUF("Return to caller");
	sys_log(WL_INFO, done, 1);
}
