#ifndef ENTRY_H
#define ENTRY_H
#include <kernel/os.h>
#include <stdint.h>

#define LFB_MAX_HEIGHT 0 // 1080
#define LFB_RATIO_WIDTH(height) (height*16/9)
#define GOP_PIXEL_FORMAT 1

/** 32bit VGA like framebuffer */
struct linear_frame_buffer {
	void* base_addr;
	uint32_t width;
	uint32_t height;
	uint32_t scan_line_size;
};

#ifndef __EFI_H
/** UEFI memory block descriptor */
typedef struct {
	uint32_t type;
	void* physAddr;
	void* virtAddr;
	uint64_t numPages;
	uint64_t attribs;
} EFI_MEMORY_DESCRIPTOR;
#endif

/** UEFI memory pages list */
struct efi_memory_map {
	void* ptr;
	uint64_t size;
	uint64_t desc_size;
};

/** Pseudo init ramdisk. Just a list of preloaded files */
struct fake_initrd {
	struct loader_srv_file_t* list;
	uint64_t count;
};

#define K_PATH "boot\\kernel.elf"
#define SRV_PATH "srv"

/** Informations from loader to kernel. Partial Multiboot2 infos */
struct loader_info {
	struct efi_memory_map mmap;
	struct fake_initrd initrd;
	void* acpi_rsdp;
	struct linear_frame_buffer* lfb;
};

void _start(struct loader_info *info);

#endif
