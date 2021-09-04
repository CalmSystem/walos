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

static inline void load_initrd(struct fake_initrd* init) {
	EFI_FILE_PROTOCOL *srv_dir = open_file(NULL, WSTR(SRV_PATH));
	if (!srv_dir) goto err;

	const uint64_t list_size = get_file_size(srv_dir) / sizeof(struct EFI_FILE_INFO) * sizeof(struct loader_srv_file_t);
	system_table->BootServices->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, EFI_TO_PAGES(list_size), (uint64_t*)&init->list);
	if (!init->list) {
		srv_dir->Close(srv_dir);
		goto err;
	}

	uint8_t buf[2*sizeof(struct EFI_FILE_INFO)];
	while (1) {
		{
			for (uintn_t i = 0; i < sizeof(buf); i++) buf[i] = 0;
			uintn_t size = sizeof(buf);
			if (srv_dir->Read(srv_dir, &size, buf) & EFI_ERR || size == 0) break;
			if (size > sizeof(buf)) {
				llogs(WL_CRIT, "Service name too long");
				init->count = 0;
				return;
			}
		}
		struct EFI_FILE_INFO* info = (void*)buf;

		if (info->Attribute & EFI_FILE_DIRECTORY) continue;

		uint64_t name_size;
		if (strlen_utf8(info->FileName, (sizeof(buf) - sizeof(*info)) / sizeof(char16_t), &name_size) < 0 || !name_size) continue;

		struct loader_srv_file_t *entry = init->list + init->count;
		sto_utf8(info->FileName, (sizeof(buf) - sizeof(*info)) / sizeof(char16_t), (char*)entry->name);
		((char*)entry->name)[name_size] = '\0';
		entry->size = info->FileSize;
		EFI_FILE_PROTOCOL *file = NULL;
		srv_dir->Open(srv_dir, &file, info->FileName, EFI_FILE_MODE_READ, 0);
		char *data = NULL;
		system_table->BootServices->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, EFI_TO_PAGES(info->FileSize + 1), (EFI_PHYSICAL_ADDRESS *)&data);
		file->Read(file, &info->FileSize, data);
		data[info->FileSize] = '\0';
		entry->data = data;
		file->Close(file);
		init->count++;
	}
	srv_dir->Close(srv_dir);

	return;

err:
	println(WSTR("Failed to open services directory"));
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

	load_initrd(&info.initrd);
	if (!info.initrd.count) return EFI_ERR;

	struct linear_frame_buffer lfb = {0};
	if (load_gop(&lfb) == EFI_SUCCESS) info.lfb = &lfb;

	uintn_t map_key = 0;
	{
		EFI_MEMORY_DESCRIPTOR *map = NULL;
		uint32_t desc_version;
		EFI_STATUS err;
		while ((err = st->BootServices->GetMemoryMap(&info.mmap.size, map, &map_key, &info.mmap.desc_size, &desc_version)) & EFI_ERR) {
			if (map) st->BootServices->FreePool(map);
			st->BootServices->AllocatePool(EfiLoaderData, info.mmap.size, (void **)&map);
		}
		info.mmap.ptr = map;
	}

	__attribute__((sysv_abi)) void (*kernel_start)(struct loader_info*) = (__attribute__((sysv_abi)) void (*)(struct loader_info*))kernel_entry;
	llogs(WL_INFO, "No more logs on screen. See serial");

	st->BootServices->ExitBootServices(ih, map_key);
	kernel_start(&info);

	while (1);
}
