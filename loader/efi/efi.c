#include <efi/efi-tools.h>
#include <efi/protocol/efi-snp.h>
#include <kernel/os.h>
static inline int memcpy(void* aptr, const void* bptr, size_t n){
	char* a = aptr;
	const char *b = bptr;
	for (size_t i = 0; i < n; i++) a[i] = b[i];
	return 0;
}
#include "../shared/vga.c.h"

#define SRV_PATH "entry"

static inline void push_watchdog_timer() {
	system_table->BootServices->SetWatchdogTimer(60, 0, 13, L"OS timed out");
}
static void wait() {
	push_watchdog_timer(); /* Expand for a minute */
	//TODO: wait for event
	system_table->BootServices->Stall(1000); /* Sleep for now */
}

static size_t loader_srv_list(struct loader_srv_file_t* files, size_t nfiles, size_t offset) {
	EFI_FILE_PROTOCOL *srv_dir = open_file(NULL, WSTR(SRV_PATH));
	if (!srv_dir) return 0;

	size_t read = 0;
	uint8_t buf[sizeof(struct EFI_FILE_INFO)*2];
	while (read < offset + nfiles) {
		{
			for (uintn_t i = 0; i < sizeof(buf); i++) buf[i] = 0;
			uintn_t size = sizeof(buf);
			if (srv_dir->Read(srv_dir, &size, buf) & EFI_ERR || size == 0) {
				/* No more files */
				break;
			}
			if (size > sizeof(buf)) {
				llogs(WL_CRIT, "Service name too long");
				break;
			}
		}
		struct EFI_FILE_INFO* info = (void*)buf;

		if (info->Attribute & EFI_FILE_DIRECTORY) continue;

		uint64_t name_size;
		int name_fit = strlen_utf8(info->FileName, (sizeof(buf) - sizeof(*info)) / sizeof(char16_t), &name_size);
		if (name_fit < 0 && !name_size) continue;

		if (offset) {
			offset--;
			continue;
		}

		struct loader_srv_file_t* file = &files[read++];
		// NOTE: assume name fit
		sto_utf8(info->FileName, (sizeof(buf) - sizeof(*info)) / sizeof(char16_t), (char*)file->name);
		((char*)file->name)[name_size] = '\0';
		file->data = NULL;
		file->size = info->FileSize;
	}
	srv_dir->Close(srv_dir);
	return read;
}
static size_t loader_srv_read(cstr name8, uint8_t *ptr, size_t len, size_t offset) {

	uint64_t name_size;
	if (!ptr || strlen_utf16(name8, UINT8_MAX, &name_size) < 0 || !name_size)
		return 0;

	EFI_FILE_PROTOCOL *srv_dir = open_file(NULL, WSTR(SRV_PATH));
	if (!srv_dir) return 0;

	char16_t name[name_size + 1];
	sto_utf16(name8, UINT8_MAX, name);
	name[name_size] = L'\0';
	EFI_FILE_PROTOCOL *file = open_file(srv_dir, name);
	srv_dir->Close(srv_dir);
	if (!file) return 0;

	uint64_t size = get_file_size(file);
	if (size > offset) {
		if (len > size - offset) len = size - offset;
		file->Read(file, &len, ptr);
	} else
		len = 0;

	file->Close(file);
	return len;
}

static page_t* loader_alloc(size_t n_pages) {
	EFI_PHYSICAL_ADDRESS pages = 0;
	if (system_table->BootServices->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, n_pages, &pages) & EFI_ERR)
		pages = 0;

	return (page_t*)pages;
}
static void loader_free(page_t* first, size_t n_pages) {
	system_table->BootServices->FreePages((uintn_t)first, n_pages);
}

static K_SIGNED_HDL(hw_key_read) {
	EFI_INPUT_KEY key;
	while (system_table->ConIn->ReadKeyStroke(system_table->ConIn, &key) != EFI_SUCCESS)
		system_table->BootServices->WaitForEvent(1, system_table->ConIn->WaitForKey, 0);

	K__RET(char, 0) = key.ScanCode == '\b' ? 127 : key.UnicodeChar;
	return NULL;
}

static k_signed_call_table hw_feats = {
	NULL, 1, {
		{hw_key_read, NULL, {"hw", "key_read", 1, 0, NULL}}
	}
};
EFI_STATUS efi_main(EFI_HANDLE ih, EFI_SYSTEM_TABLE *st) {
	system_table = st;
	image_handle = ih;

	st->ConOut->ClearScreen(st->ConOut);
	llogs(WL_NOTICE, "EFI APP Loader");
	push_watchdog_timer();

	struct linear_frame_buffer lfb = {0};
	if (load_gop(&lfb) == EFI_SUCCESS) vga_setup(&lfb);

	const struct loader_ctx_t ctx = {
		{
			.log=putsn,
			.wait=wait,
			.take_pages=loader_alloc,
			.release_pages=loader_free,
			.srv_list=loader_srv_list,
			.srv_read=loader_srv_read
		},
		.hw_feats=&hw_feats,
		.usr_feats=lfb.base_addr ? &vga_feats : &no_vga_feats
	};
	os_entry(&ctx);

	llogs(WL_INFO, "OS Stopped");
#if LOADER_SLOW_STOP
	llogs(WL_DEBUG, "Waiting 10s...");
	system_table->BootServices->Stall(10000000);
#endif
	return system_table->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
}
