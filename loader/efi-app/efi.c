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
			print(WSTR("Service name too long: "));
			printd(size);
			print(WSTR(""));
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

EFI_STATUS efi_main(EFI_HANDLE ih, EFI_SYSTEM_TABLE *st) {
	system_table = st;
	image_handle = ih;

	println(WSTR("EFI APP Loader"));
	push_watchdog_timer();

	size_t featurecnt = 0;
	struct linear_frame_buffer lfb = {0};
	if (load_gop(&lfb) == EFI_SUCCESS) {
		vga_setup(&lfb);
		featurecnt += 2;
	}

	k_signed_call features[featurecnt];
	size_t i_feature = 0;
	if (lfb.base_addr) {
		features[i_feature++] = vga_info_call;
		features[i_feature++] = vga_put_call;
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

	println(WSTR("OS Stopped"));
#if LOADER_SLOW_STOP
	println(WSTR("Waiting 10s..."));
	system_table->BootServices->Stall(10000000);
#endif
	return system_table->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
}
