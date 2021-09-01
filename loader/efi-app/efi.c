#include <efi/efi-tools.h>
#include <kernel/os.h>
#include "../shared/vga.c.h"

#define SRV_PATH "srv"

static inline void push_watchdog_timer() {
	system_table->BootServices->SetWatchdogTimer(60, 0, 13, L"OS timed out");
}
static void wait() {
	push_watchdog_timer(); /* Expand for a minute */
	//TODO: wait for event
	system_table->BootServices->Stall(1000); /* Sleep for now */
}

static void loader_srv_list(size_t to_skip, void (*cb)(void *arg, const struct loader_srv_file_t *files, size_t nfiles), void *arg) {

	EFI_FILE_PROTOCOL *srv_dir = open_file(NULL, WSTR(SRV_PATH));
	if (!srv_dir) {
		cb(arg, NULL, 0);
		return;
	}

	uint8_t buf[2*sizeof(struct EFI_FILE_INFO)];
	while (1) {
		for (uintn_t i = 0; i < sizeof(buf); i++) buf[i] = 0;
		uintn_t size = sizeof(buf);
		if (srv_dir->Read(srv_dir, &size, buf) & EFI_ERR || size == 0) {
			/* No more files */
			cb(arg, NULL, 0);
			break;
		}
		if (size > sizeof(buf)) {
			llogs(WL_CRIT, "Service name too long");
			cb(arg, NULL, 0);
			break;
		}
		struct EFI_FILE_INFO* info = (void*)buf;

		if (info->Attribute & EFI_FILE_DIRECTORY) continue;

		uint64_t name_size;
		int name_fit = strlen_utf8(info->FileName, (sizeof(buf) - sizeof(*info)) / sizeof(char16_t), &name_size);
		if (name_fit < 0 && !name_size) continue;

		if (to_skip > 0) {
			to_skip--;
			continue;
		}

		// FIXME: assume name fit
		sto_utf8(info->FileName, (sizeof(buf) - sizeof(*info)) / sizeof(char16_t), (char*)info->FileName);
		((char*)info->FileName)[name_size] = '\0';

		struct loader_srv_file_t srv = {(cstr)info->FileName, info->FileSize, NULL};
		cb(arg, &srv, 1);
		break;
	}
	srv_dir->Close(srv_dir);
}
static void loader_srv_read(cstr name8, void *ptr, size_t len, size_t offset, void (*cb)(void *arg, size_t read), void *arg) {

	uint64_t name_size;
	if (!ptr || strlen_utf16(name8, UINT8_MAX, &name_size) < 0 || !name_size) {
		cb(arg, 0);
		return;
	}

	EFI_FILE_PROTOCOL *srv_dir = open_file(NULL, WSTR(SRV_PATH));
	if (!srv_dir) {
		cb(arg, 0);
		return;
	}

	char16_t name[name_size + 1];
	sto_utf16(name8, UINT8_MAX, name);
	name[name_size] = L'\0';
	EFI_FILE_PROTOCOL *file = open_file(srv_dir, name);
	srv_dir->Close(srv_dir);
	if (!file) {
		cb(arg, 0);
		return;
	}

	uint64_t size = get_file_size(file);
	if (size > offset) {
		if (len > size - offset) len = size - offset;
		file->Read(file, &len, ptr);
	} else 
		len = 0;

	file->Close(file);
	cb(arg, len);
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

static cstr hw_key_read(const void** argv, void** retv, struct k_runtime_ctx* ctx) {
	EFI_INPUT_KEY key;
	while (system_table->ConIn->ReadKeyStroke(system_table->ConIn, &key) != EFI_SUCCESS)
		system_table->BootServices->WaitForEvent(1, system_table->ConIn->WaitForKey, 0);

	*(char*)retv[0] = key.ScanCode == '\b' ? 127 : key.UnicodeChar;
	return NULL;
}

EFI_STATUS efi_main(EFI_HANDLE ih, EFI_SYSTEM_TABLE *st) {
	system_table = st;
	image_handle = ih;

	st->ConOut->ClearScreen(st->ConOut);
	llogs(WL_NOTICE, "EFI APP Loader");
	push_watchdog_timer();

	size_t i_feature = 1;
	struct linear_frame_buffer lfb = {0};
	if (load_gop(&lfb) == EFI_SUCCESS) {
		vga_setup(&lfb);
		i_feature += lengthof(vga_features);
	}

	const size_t featurecnt = i_feature;
	k_signed_call features[featurecnt];
	i_feature = 0;
	features[i_feature++] = (k_signed_call){hw_key_read, {"hw", "key_read", 1, 0, NULL}};
	for (size_t i = 0; lfb.base_addr && i < lengthof(vga_features); i++) {
		features[i_feature++] = vga_features[i];
	}

	struct os_ctx_t os_ctx = {
		{
			.log=putsn,
			.wait=wait,
			.take_pages=loader_alloc,
			.release_pages=loader_free,
			.srv_list=loader_srv_list,
			.srv_read=loader_srv_read
		}, {
			features, featurecnt
		}
	};
	os_entry(&os_ctx);

	llogs(WL_INFO, "OS Stopped");
#if LOADER_SLOW_STOP
	llogs(WL_DEBUG, "Waiting 10s...");
	system_table->BootServices->Stall(10000000);
#endif
	return system_table->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
}
