#include <efi/efi-tools.h>
#include <efi/protocol/efi-gop.h>
#include <efi/elf.h>
#include "entry.h"

static inline int memcmp(const void* aptr, const void* bptr, size_t n){
	const unsigned char* a = aptr, *b = bptr;
	for (size_t i = 0; i < n; i++){
		if (a[i] < b[i]) return -1;
		else if (a[i] > b[i]) return 1;
	}
	return 0;
}

static inline Elf64_Addr load_kernel(char16_t* path) {
	EFI_FILE_PROTOCOL* file = open_file(NULL, path);
	if (!file) return 0;

	Elf64_Ehdr header;
	{
		uintn_t size = sizeof(header);
		file->Read(file, &size, &header);
	}

	if (
		memcmp(&header.e_ident[EI_MAG0], ELFMAG, SELFMAG) != 0 ||
		header.e_ident[EI_CLASS] != ELFCLASS64 ||
		header.e_ident[EI_DATA] != ELFDATA2LSB ||
		header.e_type != ET_EXEC ||
		header.e_machine != EM_X86_64 ||
		header.e_version != EV_CURRENT
	) {
		println(L"Kernel: Bad format");
		return 0;
	}

	Elf64_Phdr* phdrs;
	{
		file->SetPosition(file, header.e_phoff);
		uintn_t size = header.e_phnum * header.e_phentsize;
		system_table->BootServices->AllocatePool(EfiRuntimeServicesData, size, (void**)&phdrs);
		file->Read(file, &size, phdrs);
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
				Elf64_Addr segment = phdr->p_paddr;
				system_table->BootServices->AllocatePages(AllocateAddress, EfiRuntimeServicesData, EFI_TO_PAGES(phdr->p_memsz), &segment);

				file->SetPosition(file, phdr->p_offset);
				uintn_t size = phdr->p_filesz;
				file->Read(file, &size, (void*)segment);
				break;
			}
		}
	}
	system_table->BootServices->FreePool(phdrs);
	file->Close(file);

	return header.e_entry;
}

static inline EFI_STATUS load_gop(struct linear_frame_buffer* out) {
	EFI_STATUS status;
	EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;

	status = system_table->BootServices->LocateProtocol(&gopGuid, 0, (void**)&gop);
	if(EFI_ERR & status) {
		println(L"GOP: Not available");
		return status;
	}

	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
	uintn_t i, SizeOfInfo, numModes, nativeMode;

	status = gop->QueryMode(gop, gop->Mode ? gop->Mode->Mode : 0, &SizeOfInfo, &info);
	// this is needed to get the current video mode
	if (status == EFI_NOT_STARTED)
		status = gop->SetMode(gop, 0);
	if(EFI_ERR & status) {
		println(L"GOP: Can not query");
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
			println(L"GOP: No compatible mode");
			return status;
		}
	}

	out->base_addr = (void*)gop->Mode->FrameBufferBase;
	out->width = gop->Mode->Info->HorizontalResolution;
	out->height = gop->Mode->Info->VerticalResolution;
	out->scan_line_size = gop->Mode->Info->PixelsPerScanLine;
	return EFI_SUCCESS;
}

static inline void* load_acpi() {
	EFI_GUID v2 = EFI_ACPI_20_TABLE_GUID;
	for (uintn_t i = 0; i < system_table->NumberOfTableEntries; i++) {
		if (memcmp(&system_table->ConfigurationTable[i].VendorGuid, &v2, sizeof(v2)) == 0)
			return system_table->ConfigurationTable[i].VendorTable;
	}
	EFI_GUID v1 = EFI_ACPI_10_TABLE_GUID;
	for (uintn_t i = 0; i < system_table->NumberOfTableEntries; i++) {
		if (memcmp(&system_table->ConfigurationTable[i].VendorGuid, &v1, sizeof(v1)) == 0)
			return system_table->ConfigurationTable[i].VendorTable;
	}
	println(WSTR("Failed to load ACPI"));
	return NULL;
}

EFI_STATUS efi_main(EFI_HANDLE ih, EFI_SYSTEM_TABLE *st) {
	system_table = st;
	image_handle = ih;

	println(WSTR("EFI OS Loader"));

	Elf64_Addr kernel_entry = load_kernel(WSTR(K_PATH));
	if (!kernel_entry) return EFI_SUCCESS;

	struct loader_info info = {0};

	info.acpi_rsdp = load_acpi();
	if (!info.acpi_rsdp) return EFI_SUCCESS;

	//TODO: load services

	struct linear_frame_buffer gop = {0};
	if ((load_gop(&gop) & EFI_ERR) == 0) info.lfb = &gop;

	uintn_t map_key;
	{
		EFI_MEMORY_DESCRIPTOR *map = NULL;
		uint32_t desc_version;
		st->BootServices->GetMemoryMap(&info.mmap.size, map, &map_key, &info.mmap.desc_size, &desc_version);
		st->BootServices->AllocatePool(EfiLoaderData, info.mmap.size + 2, (void **)&map);
		st->BootServices->GetMemoryMap(&info.mmap.size, map, &map_key, &info.mmap.desc_size, &desc_version);
		info.mmap.ptr = map;
	}

	__attribute__((sysv_abi)) void (*kernel_start)(struct loader_info*) = (__attribute__((sysv_abi)) void (*)(struct loader_info*))kernel_entry;
	println(WSTR("No more logs on screen. See serial"));

	st->BootServices->ExitBootServices(ih, map_key);
	kernel_start(&info);

	while (1);
}
