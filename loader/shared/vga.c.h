#include "lfb.h"

static struct linear_frame_buffer s_lfb = {0};
static inline void vga_setup(struct linear_frame_buffer* lfb) { s_lfb = *lfb; }

static const enum w_fn_sign_type vga_info_sign[] = {ST_OVAL, ST_OVAL};
static cstr vga_info(const void** argv, void** retv, struct k_runtime_ctx* ctx) {
	if (!s_lfb.base_addr) return "No graphical output";

	*(uint32_t*)argv[0] = s_lfb.width;
	*(uint32_t*)argv[1] = s_lfb.height;
	*(int32_t*)retv[0] = 0;
	return NULL;
}
static const enum w_fn_sign_type vga_put_sign[] = {ST_I32, ST_I32, ST_VEC, ST_CLEN, ST_I32, ST_I32};
static cstr vga_put(const void** argv, void** retv, struct k_runtime_ctx* ctx) {
	if (!s_lfb.base_addr) return "No graphical output";

	const uint32_t start_x = *(const uint32_t*)argv[0];
	const uint32_t start_y = *(const uint32_t*)argv[1];
	const uint32_t *const pxs = argv[2];
	const uint32_t npx = (*(const uint32_t*)argv[3]) / sizeof(uint32_t);
	const uint32_t size_x = *(const uint32_t*)argv[4];
	const uint32_t size_y = *(const uint32_t*)argv[5];

	uint32_t ipx = 0;
	for (uint32_t y = start_y; y < start_y + size_y && y < s_lfb.height; y++) {
		uint32_t* const line_addr = (uint32_t*)s_lfb.base_addr + s_lfb.scan_line_size * y;
		for (size_t x = start_x; x < start_x + size_x; x++) {
			if (__builtin_expect(x > s_lfb.width, 0)) {
				ipx = (ipx + size_x - x) % npx;
				break;
			}
			if (__builtin_expect(pxs[ipx] < (1 << 24), 1)) {
				line_addr[x] = s_lfb.use_bgr ? pxs[ipx] : /* BGR to RGB */
					((pxs[ipx] & 0x00FF00) | ((pxs[ipx] >> 16) & 0xFF) | ((pxs[ipx] & 0xFF) << 16));
			}
			ipx = (ipx + 1) % npx;
		}
	}

	*(int32_t*)retv[0] = 0;
	return NULL;
}

static const k_signed_call vga_info_call = {vga_info, {"vga", "info", 1, 2, vga_info_sign}};
static const k_signed_call vga_put_call = {vga_put, {"vga", "put", 1, 6, vga_put_sign}};
