#include <w/sys.h>
#include <embed.wasm.h>

uint8_t _binary_embed_wasm[];

static const w_ciovec done = {"Return to caller", 17};
void _start() {
	sys_exec("embed", 6, _binary_embed_wasm, sizeof(_binary_embed_wasm));
	sys_log(WL_INFO, &done, 1);
}
