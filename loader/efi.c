#include "protocol/efi-lip.h"
#include "protocol/efi-gop.h"
#include "protocol/efi-sfsp.h"
#include "bootinfo.h"
#include "elf.h"
#include <stddef.h>

EFI_SYSTEM_TABLE* system_table;
static inline void printk(char16_t* s) {
    system_table->ConOut->OutputString(system_table->ConOut, s);
}

EFI_FILE_PROTOCOL* load_file(EFI_FILE_PROTOCOL* directory, char16_t* path, EFI_HANDLE ih) {
	EFI_STATUS status;
    EFI_FILE_PROTOCOL* file;
    EFI_GUID lipGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
	EFI_LOADED_IMAGE_PROTOCOL* lip;
    EFI_GUID sfspGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* sfsp;

	system_table->BootServices->HandleProtocol(ih, &lipGuid, (void**)&lip);
    system_table->BootServices->HandleProtocol(lip->DeviceHandle, &sfspGuid, (void**)&sfsp);

	if (!directory) sfsp->OpenVolume(sfsp, &directory);

	status = directory->Open(directory, &file, path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
	if (status & EFI_ERR) {
        printk(L"FP: Can not open file\n\r");
        return NULL;
    }

	return file;
}

int memcmp(const void* aptr, const void* bptr, size_t n){
	const unsigned char* a = aptr, *b = bptr;
	for (size_t i = 0; i < n; i++){
		if (a[i] < b[i]) return -1;
		else if (a[i] > b[i]) return 1;
	}
	return 0;
}

EFI_STATUS load_kernel(EFI_HANDLE ih, Elf64_Addr* entry) {
    EFI_FILE_PROTOCOL* kernel = load_file(NULL, L"kernel.elf", ih);
	if (!kernel) return 1;
	
    Elf64_Ehdr header;
	{
		uintn_t f_info_size;
		EFI_FILE_INFO* f_info;
        EFI_GUID finfGuid = { 0x9576e92, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} };
		kernel->GetInfo(kernel, &finfGuid, &f_info_size, NULL);
		system_table->BootServices->AllocatePool(EfiLoaderData, f_info_size, (void**)&f_info);
		kernel->GetInfo(kernel, &finfGuid, &f_info_size, (void**)&f_info);

		uintn_t size = sizeof(header);
		kernel->Read(kernel, &size, &header);
	}

	if (
		memcmp(&header.e_ident[EI_MAG0], ELFMAG, SELFMAG) != 0 ||
		header.e_ident[EI_CLASS] != ELFCLASS64 ||
		header.e_ident[EI_DATA] != ELFDATA2LSB ||
		header.e_type != ET_EXEC ||
		header.e_machine != EM_X86_64 ||
		header.e_version != EV_CURRENT
	) {
		printk(L"Kernel: Bad format\r\n");
        return 1;
	}
	
	Elf64_Phdr* phdrs;
	{
		kernel->SetPosition(kernel, header.e_phoff);
		uintn_t size = header.e_phnum * header.e_phentsize;
		system_table->BootServices->AllocatePool(EfiLoaderData, size, (void**)&phdrs);
		kernel->Read(kernel, &size, phdrs);
	}

	for (
		Elf64_Phdr* phdr = phdrs;
		(char*)phdr < (char*)phdrs + header.e_phnum * header.e_phentsize;
		phdr = (Elf64_Phdr*)((char*)phdr + header.e_phentsize)
	)
	{
		switch (phdr->p_type){
			case PT_LOAD:
			{
				int pages = (phdr->p_memsz + 0x1000 - 1) / 0x1000;
				Elf64_Addr segment = phdr->p_paddr;
				system_table->BootServices->AllocatePages(AllocateAddress, EfiLoaderData, pages, &segment);

				kernel->SetPosition(kernel, phdr->p_offset);
				uintn_t size = phdr->p_filesz;
				kernel->Read(kernel, &size, (void*)segment);
				break;
			}
		}
	}
    *entry = header.e_entry;
    return EFI_SUCCESS;
}

PSF1_FONT* load_psf1_font(char16_t* path, EFI_HANDLE ih) {
	EFI_FILE_PROTOCOL* font = load_file(NULL, path, ih);
	if (!font) return NULL;

	PSF1_HEADER* font_header;
	system_table->BootServices->AllocatePool(EfiLoaderData, sizeof(PSF1_HEADER), (void**)&font_header);
	uintn_t size = sizeof(PSF1_HEADER);
	font->Read(font, &size, font_header);

	if (font_header->magic[0] != PSF1_MAGIC0 || font_header->magic[1] != PSF1_MAGIC1){
		return NULL;
	}

	uintn_t glyph_buffer_size = font_header->charsize * 256;
	if (font_header->mode == 1) { //512 glyph mode
		glyph_buffer_size = font_header->charsize * 512;
	}

	void* glyphBuffer;
	{
		font->SetPosition(font, sizeof(PSF1_HEADER));
		system_table->BootServices->AllocatePool(EfiLoaderData, glyph_buffer_size, (void**)&glyphBuffer);
		font->Read(font, &glyph_buffer_size, glyphBuffer);
	}

	PSF1_FONT* ret;
	system_table->BootServices->AllocatePool(EfiLoaderData, sizeof(PSF1_FONT), (void**)&ret);
	ret->header = font_header;
	ret->glyph_buffer = glyphBuffer;
	return ret;
}

EFI_STATUS load_gop(LINEAR_FRAMEBUFFER* out) {
	EFI_STATUS status;
    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    
    status = system_table->BootServices->LocateProtocol(&gopGuid, 0, (void**)&gop);
    if(EFI_ERR & status) {
        printk(L"GOP: Not available\n\r");
        return status;
    }

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
    uintn_t i, SizeOfInfo, numModes, nativeMode;
 
    status = gop->QueryMode(gop, gop->Mode ? gop->Mode->Mode : 0, &SizeOfInfo, &info);
    // this is needed to get the current video mode
    if (status == EFI_NOT_STARTED)
        status = gop->SetMode(gop, 0);
    if(EFI_ERR & status) {
        printk(L"GOP: Can not query\n\r");
        return status;
    }
        
    nativeMode = gop->Mode->Mode;
    numModes = gop->Mode->MaxMode;

    if (LFB_MAX_HEIGHT) {
        uint32_t preferMode = numModes+1, maxHeight = 0;
        for (i = 0; LFB_MAX_HEIGHT && i < numModes; i++) {
            status = gop->QueryMode(gop, i, &SizeOfInfo, &info);
            if (info->PixelFormat == GOP_PIXEL_FORMAT &&
                info->VerticalResolution <= LFB_MAX_HEIGHT &&
                info->VerticalResolution > maxHeight &&
                info->HorizontalResolution == LFB_RATIO_WIDTH(info->VerticalResolution))
            {
                preferMode = i;
                maxHeight = info->VerticalResolution;
            }
        }
        status = gop->SetMode(gop, preferMode);
        if(EFI_ERR & status) {
            printk(L"GOP: No compatible mode\n\r");
            return status;
        }
    }

    out->base_addr = (void*)gop->Mode->FrameBufferBase;
    out->width = gop->Mode->Info->HorizontalResolution;
    out->height = gop->Mode->Info->VerticalResolution;
    out->scan_line_size = gop->Mode->Info->PixelsPerScanLine;
    return EFI_SUCCESS;
}

EFI_STATUS efi_main(EFI_HANDLE ih, EFI_SYSTEM_TABLE *st) {
    system_table = st;
    
    printk(L"Walos EFI loader\n\r");

    EFI_STATUS status;
    BOOT_INFO bootinfo = {0};
    Elf64_Addr kernel_entry;

    status = load_kernel(ih, &kernel_entry);
    if (status & EFI_ERR) return EFI_SUCCESS;

    printk(L"Kernel: Loaded\n\r");
    
    bootinfo.font = load_psf1_font(PSF1_PATH, ih);
    if (bootinfo.font) printk(L"Font: Loaded\n\r");

    LINEAR_FRAMEBUFFER gop;
    status = load_gop(&gop);
    if (!(status & EFI_ERR)) {
        bootinfo.lfb = &gop;
        printk(L"GOP: Loaded\n\r");
    }

    EFI_MEMORY_MAP mmap = {0};
    uintn_t map_key;
    {
        EFI_MEMORY_DESCRIPTOR* map = NULL;
        uint32_t desc_version;
        st->BootServices->GetMemoryMap(&mmap.size, map, &map_key, &mmap.desc_size, &desc_version);
        st->BootServices->AllocatePool(EfiLoaderData, mmap.size, (void**)&map);
        st->BootServices->GetMemoryMap(&mmap.size, map, &map_key, &mmap.desc_size, &desc_version);
        mmap.ptr = map;
    }
    bootinfo.mmap = &mmap;
    printk(L"Mmap: Loaded\n\r");

	__attribute__((sysv_abi)) void (*kernel_start)(BOOT_INFO*) = (__attribute__((sysv_abi)) void (*)(BOOT_INFO*))kernel_entry;

    printk(L"Exit to kernel\n\r");

    st->BootServices->ExitBootServices(ih, map_key);
	kernel_start(&bootinfo);

    while(1);
}
