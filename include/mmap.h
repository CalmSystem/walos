#ifndef __MMAP_H
#define __MMAP_H

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

#endif
