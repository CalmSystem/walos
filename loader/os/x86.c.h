#include "asm.h"

static inline K_SIGNED_HDL(x86_io_write8) {
	io_write8(K__GET(uint16_t, 0), K__GET(uint8_t, 1));
	return NULL;
}
static inline K_SIGNED_HDL(x86_io_read8) {
	K__RET(uint8_t, 0) = io_read8(K__GET(uint16_t, 0));
	return NULL;
}
static inline K_SIGNED_HDL(x86_io_write16) {
	io_write16(K__GET(uint16_t, 0), K__GET(uint16_t, 1));
	return NULL;
}
static inline K_SIGNED_HDL(x86_io_read16) {
	K__RET(uint16_t, 0) = io_read16(K__GET(uint16_t, 0));
	return NULL;
}
static inline K_SIGNED_HDL(x86_io_write32) {
	io_write32(K__GET(uint16_t, 0), K__GET(uint32_t, 1));
	return NULL;
}
static inline K_SIGNED_HDL(x86_io_read32) {
	K__RET(uint32_t, 0) = io_read32(K__GET(uint16_t, 0));
	return NULL;
}
static k_signed_call_table x86_feats = {
	NULL, 6, {
		{x86_io_write8, NULL, {"x86", "io_write8", 0, 2, NULL}},
		{x86_io_read8, NULL, {"x86", "io_read8", 1, 1, NULL}},
		{x86_io_write16, NULL, {"x86", "io_write16", 0, 2, NULL}},
		{x86_io_read16, NULL, {"x86", "io_read16", 1, 1, NULL}},
		{x86_io_write32, NULL, {"x86", "io_write32", 0, 2, NULL}},
		{x86_io_read32, NULL, {"x86", "io_read32", 1, 1, NULL}}
	}
};
