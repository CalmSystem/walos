#ifndef LFB_H
#define LFB_H
#include <stdint.h>
#include <stdbool.h>

#ifndef LFB_MAX_HEIGHT
#define LFB_MAX_HEIGHT 0 // 1080
#endif
#ifndef LFB_MIN_RATIO
#define LFB_MIN_RATIO (16/9.f)
#endif
#ifndef LFB_MAX_RATIO
#define LFB_MAX_RATIO (16/9.f)
#endif

/** 32bit VGA like framebuffer */
struct linear_frame_buffer {
	void* base_addr;
	uint32_t width;
	uint32_t height;
	uint32_t scan_line_size;
	bool use_bgr; /* else pixel format is rgb */
};

#endif
