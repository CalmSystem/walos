#include <efi/efi-tools.h>
#include <efi/protocol/efi-gop.h>
#include <elf.h>
#include <uzlib/uzlib.h>
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
		llogs(WL_CRIT, "Kernel: Bad format");
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
	llogs(WL_CRIT, "Failed to load ACPI");
	return NULL;
}

static inline void* load_initrd() {
	EFI_FILE_PROTOCOL *entry_file = open_file(NULL, WSTR(ENTRY_PATH));
	if (!entry_file) goto err;

	uint64_t file_size = get_file_size(entry_file);
	unsigned char* file_data = NULL;
	system_table->BootServices->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, EFI_TO_PAGES(file_size), (uint64_t*)&file_data);
	const bool failed = !file_data || entry_file->Read(entry_file, &file_size, file_data);
	entry_file->Close(entry_file);
	if (failed) goto err;

	// get decompressed length
	uint64_t out_size = file_data[file_size-1];
	out_size = 256*out_size + file_data[file_size-2];
	out_size = 256*out_size + file_data[file_size-3];
	out_size = 256*out_size + file_data[file_size-4];

	unsigned char* out_data = NULL;
	system_table->BootServices->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, EFI_TO_PAGES(out_size), (uint64_t*)&out_data);
	if (!out_data) goto err;

	uzlib_init();

	struct uzlib_uncomp d;
	uzlib_uncompress_init(&d, NULL, 0);

	d.source = file_data;
	d.source_limit = file_data + file_size - 4;
	d.source_read_cb = NULL;
	if (uzlib_gzip_parse_header(&d) != TINF_OK) goto err;

	d.dest_start = d.dest = out_data;
	int res;
	for (uint64_t rem_size = out_size+1; rem_size;) {
		const unsigned int OUT_CHUNK_SIZE = 1;
		unsigned int chunk_len = rem_size < OUT_CHUNK_SIZE ? rem_size : OUT_CHUNK_SIZE;
		d.dest_limit = d.dest + chunk_len;
		res = uzlib_uncompress_chksum(&d);
		rem_size -= chunk_len;
		if (res != TINF_OK) {
			break;
		}
	}
	system_table->BootServices->FreePages((uint64_t)file_data, EFI_TO_PAGES(file_size));
	if (res != TINF_DONE) goto err;

	return out_data;

err:
	llogs(WL_CRIT, "Failed to load initrd");
	return NULL;
}

enum EFI_MEMORY_TYPE {
	EFI_RESERVED_MEMORY = 0x00000000,
	EFI_LOADER_CODE = 0x00000001,
	EFI_LOADER_DATA = 0x00000002,
	EFI_BOOTSERVICES_CODE = 0x00000003,
	EFI_BOOTSERVICES_DATA = 0x00000004,
	EFI_RUNTIMESERVICES_CODE = 0x00000005,
	EFI_RUNTIMESERVICES_DATA = 0x00000006,
	EFI_CONVENTIONAL_MEMORY = 0x00000007,
	EFI_UNUSABLE_MEMORY = 0x00000008,
	EFI_ACPI_RECLAIM_MEMORY = 0x00000009,
	EFI_ACPI_NVS_MEMORY = 0x0000000a,
	EFI_MAPPED_IO_MEMORY = 0x0000000b,
	EFI_MAPPED_IO_PORTSPACE_MEMORY = 0x0000000c,
	EFI_PALCODE_MEMORY = 0x0000000d,
	EFI_PERSISTENT_MEMORY = 0x0000000e
};
static inline uintn_t load_mmap(struct memory_map *const mmap) {
	uintn_t map_key = 0;
	void *map = NULL;
	uintn_t desc_size, map_size;
	uint32_t desc_version;
	EFI_STATUS err;
	while ((err = system_table->BootServices->GetMemoryMap(&map_size, (EFI_MEMORY_DESCRIPTOR*)map, &map_key, &desc_size, &desc_version)) & EFI_ERR) {
		if (map) system_table->BootServices->FreePool(map);
		system_table->BootServices->AllocatePool(EfiLoaderData, map_size, &map);
	}
	mmap->count = map_size / desc_size;
	mmap->ptr = map;
	for (size_t i = 0; i < mmap->count; i++) {
		EFI_MEMORY_DESCRIPTOR desc = *(EFI_MEMORY_DESCRIPTOR*)((char*)map + desc_size * i);
		mmap->ptr[i].base = (void*)desc.PhysicalStart;
		mmap->ptr[i].num_pages = desc.NumberOfPages;
		mmap->ptr[i].type = desc.Type == EFI_CONVENTIONAL_MEMORY ? MEMORY_MAP_CONVENSIONAL :
			desc.Type >= EFI_LOADER_CODE && desc.Type <= EFI_BOOTSERVICES_DATA ?
			MEMORY_MAP_USABLE : MEMORY_MAP_RESERVED;
	}
	return map_key;
}

EFI_STATUS efi_main(EFI_HANDLE ih, EFI_SYSTEM_TABLE *st) {
	system_table = st;
	image_handle = ih;

	st->ConOut->ClearScreen(st->ConOut);
	llogs(WL_NOTICE, "EFI OS Loader");

	Elf64_Addr kernel_entry = load_kernel(WSTR(K_PATH));
	if (!kernel_entry) return EFI_ERR;

	struct loader_info info = {0};

	info.acpi_rsdp = load_acpi();
	if (!info.acpi_rsdp) return EFI_ERR;

	info.initrd = load_initrd();
	if (!info.initrd) return EFI_ERR;

	struct linear_frame_buffer lfb = {0};
	if (load_gop(&lfb) == EFI_SUCCESS) info.lfb = &lfb;

	uintn_t map_key = load_mmap(&info.mmap);

	__attribute__((sysv_abi)) void (*kernel_start)(struct loader_info*) = (__attribute__((sysv_abi)) void (*)(struct loader_info*))kernel_entry;
	llogs(WL_INFO, "No more logs on screen. See serial");

	st->BootServices->ExitBootServices(ih, map_key);
	kernel_start(&info);

	while (1);
}
