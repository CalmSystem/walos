#ifndef __BOOTINFO_H
#define __BOOTINFO_H

#define LFB_MAX_HEIGHT 0 // 1080
#define LFB_RATIO_WIDTH(height) (height*16/9)
#define GOP_PIXEL_FORMAT 1

typedef struct {
	void* base_addr;
	unsigned int width;
	unsigned int height;
	unsigned int scan_line_size;
} LINEAR_FRAMEBUFFER;

#define PSF1_MAGIC0 0x36
#define PSF1_MAGIC1 0x04
#define PSF1_PATH L"share\\zap-vga16.psf"

typedef struct {
	unsigned char magic[2];
	unsigned char mode;
	unsigned char charsize;
} PSF1_HEADER;

typedef struct {
	PSF1_HEADER* header;
	void* glyph_buffer;
} PSF1_FONT;

#ifndef __EFI_H
typedef struct {
    uint32_t type;
    void* physAddr;
    void* virtAddr; 
    uint64_t numPages;
    uint64_t attribs;
} EFI_MEMORY_DESCRIPTOR;
#endif

typedef struct {
    void* ptr;
	uintn_t size;
	uintn_t desc_size;
} EFI_MEMORY_MAP;

typedef struct {
    EFI_MEMORY_MAP* mmap;
    LINEAR_FRAMEBUFFER* lfb;
    PSF1_FONT* font;
} BOOT_INFO;

#endif
