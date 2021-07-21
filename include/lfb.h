#ifndef __LFB_H
#define __LFB_H

#define GOP_PIXEL_FORMAT 1

/** 32bit VGA like framebuffer */
typedef struct {
	void* base_addr;
	uint32_t width;
	uint32_t height;
	uint32_t scan_line_size;
} LINEAR_FRAMEBUFFER;

#endif
