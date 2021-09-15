#include "entry.h"
#include <stivale.h>
#include <x86_64.h>

void stivale_start(struct stivale_struct *stivale_struct);
extern char _heap_start;

__attribute__((section(".stivalehdr"), used))
static struct stivale_header stivale_hdr = {
    .stack = (uint64_t)((&_heap_start - 1 / PAGE_SIZE * PAGE_SIZE)),
    .flags = 1, /* Use framebuffer */
    .framebuffer_width  = LFB_MAX_HEIGHT * LFB_MIN_RATIO,
    .framebuffer_height = LFB_MAX_HEIGHT,
    .framebuffer_bpp    = 32,
    .entry_point = (uint64_t)&stivale_start /* will create loader_info and call _start */
};

extern void loader_log(cstr str, size_t len);
static inline void llog_out(cstr str, unsigned len) { loader_log(str, len); }
#include <llog.h>

void stivale_start(struct stivale_struct *stivale) {
	llogs(WL_NOTICE, "Stivale OS Loader");

	if (!stivale->module_count) {
		llogs(WL_CRIT, "Failed to load initrd");
		return;
	}

	// NOTE: hope llogs does not use simd...
	x86_64_enable_feats();

	struct loader_info info;
	info.acpi_rsdp = (void*)stivale->rsdp;
	info.lfb = NULL;
	struct linear_frame_buffer lfb;
	if (stivale->framebuffer_addr) {
		lfb.base_addr = stivale->framebuffer_addr;
		lfb.height = stivale->framebuffer_height;
		lfb.width = stivale->framebuffer_width;
		lfb.scan_line_size = lfb.width;
		lfb.use_bgr = stivale->fb_red_mask_shift;
		info.lfb = &lfb;
	} else llogs(WL_CRIT, "Failed to load VGA");

	struct memory_map_entry mmap[stivale->memory_map_entries];
	for (size_t i = 0; i < stivale->memory_map_entries; i++) {
		struct stivale_mmap_entry* entry = (void*)(stivale->memory_map_addr + i * sizeof(*entry));
		mmap[i].type = entry->type == STIVALE_MMAP_USABLE /* MAYBE: || 10 */ ?
			MEMORY_MAP_CONVENSIONAL : MEMORY_MAP_RESERVED;
		mmap[i].base = (void*)entry->base;
		mmap[i].num_pages = entry->length / PAGE_SIZE;
	}
	info.mmap.ptr = mmap;
	info.mmap.count = stivale->memory_map_entries;

	info.initrd = (void*)((struct stivale_module*)stivale->modules)->begin;

	_start(&info);
}
