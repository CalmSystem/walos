#include "asm.h"

static inline cstr x86_io_write8(const void **args, void **rets, struct k_runtime_ctx* ctx) {
	io_write8(*(const uint16_t*)args[0], *(const uint8_t*)args[1]);
	return NULL;
}
static inline cstr x86_io_read8(const void **args, void **rets, struct k_runtime_ctx* ctx) {
	*(uint8_t*)rets[0] = io_read8(*(const uint16_t*)args[0]);
	return NULL;
}
static inline cstr x86_io_write16(const void **args, void **rets, struct k_runtime_ctx* ctx) {
	io_write16(*(const uint16_t*)args[0], *(const uint16_t*)args[1]);
	return NULL;
}
static inline cstr x86_io_read16(const void **args, void **rets, struct k_runtime_ctx* ctx) {
	*(uint16_t*)rets[0] = io_read16(*(const uint16_t*)args[0]);
	return NULL;
}
static inline cstr x86_io_write32(const void **args, void **rets, struct k_runtime_ctx* ctx) {
	io_write32(*(const uint16_t*)args[0], *(const uint32_t*)args[1]);
	return NULL;
}
static inline cstr x86_io_read32(const void **args, void **rets, struct k_runtime_ctx* ctx) {
	*(uint32_t*)rets[0] = io_read32(*(const uint16_t*)args[0]);
	return NULL;
}
static const k_signed_call x86_features[] = {
	{x86_io_write8, {"x86", "io_write8", 0, 2, NULL}},
	{x86_io_read8, {"x86", "io_read8", 1, 1, NULL}},
	{x86_io_write16, {"x86", "io_write16", 0, 2, NULL}},
	{x86_io_read16, {"x86", "io_read16", 1, 1, NULL}},
	{x86_io_write32, {"x86", "io_write32", 0, 2, NULL}},
	{x86_io_read32, {"x86", "io_read32", 1, 1, NULL}}
};
