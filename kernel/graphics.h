#ifndef __GRAPHICS_H
#define __GRAPHICS_H

#include "stdint.h"
#include "psf.h"
#include "lfb.h"

typedef struct {
    uint32_t x, y, fg, bg;
} PUTBYTES_STATE;
#define PACK_RGB(r, g, b) ((uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16))
#define RGB_BLACK 0
#define RGB_NONE (1<<24)
#define RGB_WHITE (RGB_NONE-1)

void graphics_use_dummy();
void graphics_use_lfb(LINEAR_FRAMEBUFFER*, PSF1_FONT*);

void putbytes(const char *s, int len);
void putbytes_at(const char *s, int len, PUTBYTES_STATE* st);

#endif
