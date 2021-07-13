#ifndef __BOOTINFO_H
#define __BOOTINFO_H

#include "lfb.h"
#include "psf.h"
#include "mmap.h"
#include "services.h"

#define LFB_MAX_HEIGHT 0 // 1080
#define LFB_RATIO_WIDTH(height) (height*16/9)

#define K_PATH "kernel.elf"
#define FONT_PATH "shr\\zap-vga16.psf"
#define SRV_PATH "cfg\\services.csv"

typedef struct {
    LINEAR_FRAMEBUFFER* lfb;
    PSF1_FONT* font;
    EFI_MEMORY_MAP* mmap;
	SERVICE_TABLE services;
} BOOT_INFO;

#endif
