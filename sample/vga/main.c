#include <w/vga.h>
#include <wasm.tga.h>

uint8_t _binary_wasm_tga[];

typedef struct {
  uint8_t magic1;             // must be zero
  uint8_t colormap;           // must be zero
  uint8_t encoding;           // must be 2
  uint16_t cmaporig, cmaplen; // must be zero
  uint8_t cmapent;            // must be zero
  uint16_t x;                 // must be zero
  uint16_t y;                 // must be zero
  uint16_t w;                 // image's width
  uint16_t h;                 // image's height
  uint8_t bpp;                // must be 32
  uint8_t pixeltype;          // must be 40
} __attribute__((packed)) tga_header_t;

void _start(void) {
	const tga_header_t* h = (void*)_binary_wasm_tga;
	if (h-> magic1 != 0 || h-> colormap != 0 || h-> encoding != 2 || h-> cmaporig != 0 || h-> cmaplen != 0 ||
		h-> cmapent != 0 || h-> x != 0 || h-> bpp != 32 || h-> pixeltype != 40) return;

	uint32_t* ptr = (void*)_binary_wasm_tga + sizeof(tga_header_t);
	// Replace alpha with zeros inplace
	for (uint32_t i = 0; i < (uint32_t)h->h * h->w; i++) {
		ptr[i] &= 0xFFFFFF;
	}

	uint32_t width, height;
	vga_info(&width, &height); // Center on screen
	vga_put((width - h->w) / 2, (height - h->h) / 2, ptr, (w_size)h->h * h->w * sizeof(uint32_t), h->w, h->h);

	while(1);
}
