#ifndef __PSF_H
#define __PSF_H

#define PSF1_MAGIC0 0x36
#define PSF1_MAGIC1 0x04

typedef struct {
	unsigned char magic[2];
	unsigned char mode;
	unsigned char charsize;
} PSF1_HEADER;

typedef struct {
	PSF1_HEADER* header;
	void* glyph_buffer;
} PSF1_FONT;

#endif
