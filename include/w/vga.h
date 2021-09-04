#ifndef __MOD_VGA_H
#define __MOD_VGA_H
#include "types.h"

W_FN(vga, info, w_res, (uint32_t* width, uint32_t* height), {ST_PTR, ST_PTR})
/** Draw on screen. buf is an array of uint32_t 0BGR pixels. buf_size is sizeof(*buf) */
W_FN(vga, put, w_res,
	(uint32_t x, uint32_t y, const void* buf, w_size buf_size, uint32_t width, uint32_t height),
	{ST_I32, ST_I32, ST_ARR, ST_LEN, ST_I32, ST_I32})

#endif
