#include "lfb.h"

static struct linear_frame_buffer s_lfb = {0};
static inline void vga_setup(struct linear_frame_buffer* lfb) {
	memcpy(&s_lfb, lfb, sizeof(s_lfb));
}

static const w_fn_sign_val vga_info_sign[] = {ST_PTR, ST_PTR};
static K_SIGNED_HDL(vga_info) {
	if (!s_lfb.base_addr) return "No graphical output";

	*(uint32_t*)_args[0] = s_lfb.width;
	*(uint32_t*)_args[1] = s_lfb.height;

	K__RES(0);
}
static const w_fn_sign_val vga_put_sign[] = {ST_I32, ST_I32, ST_ARR, ST_LEN, ST_I32, ST_I32};
static K_SIGNED_HDL(vga_put) {
	if (!s_lfb.base_addr) return "No graphical output";

	const uint32_t start_x = K__GET(uint32_t, 0);
	const uint32_t start_y = K__GET(uint32_t, 1);
	const uint32_t *const pxs = _args[2];
	const uint32_t npx = (K__GET(uint32_t, 3)) / sizeof(uint32_t);
	const uint32_t size_x = K__GET(uint32_t, 4);
	const uint32_t size_y = K__GET(uint32_t, 5);

	uint32_t ipx = 0;
	for (uint32_t y = start_y; y < start_y + size_y && y < s_lfb.height; y++) {
		uint32_t* const line_addr = (uint32_t*)s_lfb.base_addr + s_lfb.scan_line_size * y;
		for (size_t x = start_x; x < start_x + size_x; x++) {
			if (UNLIKELY(x > s_lfb.width)) {
				ipx = (ipx + size_x - x) % npx;
				break;
			}
			if (LIKELY(pxs[ipx] < (1 << 24))) {
				line_addr[x] = s_lfb.use_bgr ? pxs[ipx] : /* BGR to RGB */
					((pxs[ipx] & 0x00FF00) | ((pxs[ipx] >> 16) & 0xFF) | ((pxs[ipx] & 0xFF) << 16));
			}
			ipx = (ipx + 1) % npx;
		}
	}

	K__RES(0);
}

static k_signed_call_table vga_feats = {
	NULL, 2, {
		{vga_info, NULL, {"vga", "info", 1, 2, vga_info_sign}},
		{vga_put, NULL, {"vga", "put", 1, 6, vga_put_sign}}
	}
};
static k_signed_call_table no_vga_feats = { NULL, 0 };
