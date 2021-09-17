#include <efi/efi-tools.h>
#include <efi/protocol/efi-snp.h>
#include <kernel/os.h>
#include <kernel/engine.h>

static inline int memcpy(void* aptr, const void* bptr, size_t n){
	char* a = aptr;
	const char *b = bptr;
	for (size_t i = 0; i < n; i++) a[i] = b[i];
	return 0;
}
#include "../shared/vga.c.h"

#define SRV_PATH "entry"

static inline void push_watchdog_timer(void) {
	system_table->BootServices->SetWatchdogTimer(60, 0, 13, L"OS timed out");
}
static void wait(void) {
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

static struct {
	uintn_t cnt;
	EFI_HANDLE* hdls;
	EFI_SIMPLE_NETWORK_PROTOCOL** protos;
} s_netifs = {0};
static inline void netif_load_hdls(void) {
	if (UNLIKELY(!s_netifs.hdls)) {
		EFI_GUID snpGuid = EFI_SIMPLE_NETWORK_PROTOCOL_GUID;
		if (system_table->BootServices->LocateHandleBuffer(ByProtocol, &snpGuid, NULL, &s_netifs.cnt, &s_netifs.hdls) || !s_netifs.cnt) {
			s_netifs.cnt = 0;
			s_netifs.hdls = (void*)INTPTR_MAX;
			return;
		}

		system_table->BootServices->AllocatePool(EfiRuntimeServicesData, sizeof(s_netifs.protos[0]) * s_netifs.cnt, (void**)&s_netifs.protos);
		for (uintn_t i = 0; i < s_netifs.cnt; i++) s_netifs.protos[i] = NULL;
	}
}
static inline EFI_SIMPLE_NETWORK_PROTOCOL* netif_get(uintn_t n) {
	netif_load_hdls();
	if (UNLIKELY(n >= s_netifs.cnt)) return NULL;
	if (UNLIKELY(!s_netifs.protos[n])) {
		EFI_GUID snpGuid = EFI_SIMPLE_NETWORK_PROTOCOL_GUID;
		if (system_table->BootServices->HandleProtocol(s_netifs.hdls[n], &snpGuid, (void**)(s_netifs.protos + n)))
			s_netifs.protos[n] = (void*)INTPTR_MAX;
	}
	if (UNLIKELY(s_netifs.protos[n] == (void*)INTPTR_MAX)) return NULL;
	return s_netifs.protos[n];
}

static K_SIGNED_HDL(hw_netif_cnt) {
	netif_load_hdls();
	K__RET(w_size, 0) = s_netifs.cnt;
	return NULL;
}
static const w_fn_sign_val hw_netif_info_sign[] = {ST_LEN, ST_PTR, ST_ARR, ST_LEN, ST_ARR, ST_LEN, ST_PTR};
static K_SIGNED_HDL(hw_netif_info) {
	EFI_SIMPLE_NETWORK_PROTOCOL* netif = netif_get(K__GET(w_size, 0));
	if (!netif ||
		(K__GET(w_size, 3) && K__GET(w_size, 3) < netif->Mode->HwAddressSize) ||
		(K__GET(w_size, 5) && K__GET(w_size, 5) < netif->Mode->HwAddressSize)
	) K__RES(W_EINVAL);

	netif->GetStatus(netif, NULL, NULL);
	*(uint32_t*)_args[1] = (netif->Mode->MediaPresentSupported && !netif->Mode->MediaPresent ? (1<<2) : 0) & netif->Mode->State;
	if (K__GET(w_size, 3)) memcpy((void*)_args[2], &netif->Mode->CurrentAddress, netif->Mode->HwAddressSize);
	if (K__GET(w_size, 5)) memcpy((void*)_args[4], &netif->Mode->BroadcastAddress, netif->Mode->HwAddressSize);
	*(uint32_t*)_args[6] = netif->Mode->MaxPacketSize - netif->Mode->MediaHeaderSize;

	K__RES(W_SUCCESS);
}
static K_SIGNED_HDL(hw_netif_stop) {
	EFI_SIMPLE_NETWORK_PROTOCOL* netif = netif_get(K__GET(w_size, 0));
	if (!netif) K__RES(W_EINVAL);

	netif->Stop(netif);

	K__RES(W_SUCCESS);
}
static inline bool netif_set_ready(EFI_SIMPLE_NETWORK_PROTOCOL* netif) {
	if (UNLIKELY(netif->Mode->State == EfiSimpleNetworkStopped) && netif->Start(netif)) return false;
	if (UNLIKELY(netif->Mode->State == EfiSimpleNetworkStarted) && netif->Initialize(netif, 0, 0)) {
		netif->Stop(netif);
		return false;
	}

	return true;
}
/* Assume packet size < PAGE_SIZE / 2 */
#define PK_SZ (PAGE_SIZE / 2)
static const w_fn_sign_val hw_netif_transmit_sign[] = {ST_LEN, ST_VAL, ST_ARR, ST_LEN, ST_ARR, ST_LEN, ST_CIO, ST_LEN};
static K_SIGNED_HDL(hw_netif_transmit) {
	static void* s_bufs = NULL; /* Linked list of free packets */
	EFI_SIMPLE_NETWORK_PROTOCOL* netif = netif_get(K__GET(w_size, 0));
	if (UNLIKELY(!netif ||
		(K__GET(w_size, 3) && K__GET(w_size, 3) < netif->Mode->HwAddressSize) ||
		K__GET(w_size, 5) < netif->Mode->HwAddressSize
	)) K__RES(W_EINVAL);
	if (UNLIKELY(!netif_set_ready(netif))) K__RES(W_EFAIL);

	void* buf = NULL;
	{
		while (!netif->GetStatus(netif, NULL, &buf) && buf) {
			*(void**)buf = s_bufs; /* Free sent packet */
			s_bufs = buf;
			buf = NULL;
		}
		if (LIKELY(s_bufs)) {
			buf = s_bufs;
			s_bufs = *(void**)s_bufs;
		} else {
			if (system_table->BootServices->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, 1, (uintn_t*)&buf))
				K__RES(W_EFAIL);
			s_bufs = (uint8_t*)buf + PK_SZ;
			*(void**)s_bufs = NULL;
		}
	}

	uint64_t buf_sz = netif->Mode->MediaHeaderSize;
	{
		uint8_t* pb = (uint8_t*)buf + buf_sz;
		const k_iovec* iovs = _args[6];
		const w_size iovcnt = K__GET(w_size, 7);
		for (w_size i = 0; i < iovcnt; i++) {
			buf_sz += iovs[i].len;
			if (UNLIKELY(buf_sz > PK_SZ)) {
				*(void**)buf = s_bufs;
				s_bufs = buf;
				K__RES(W_ERANGE);
			}
			memcpy(pb, iovs[i].base, iovs[i].len);
			pb += iovs[i].len;
		}
	}

	EFI_STATUS res = netif->Transmit(netif, netif->Mode->MediaHeaderSize,
		buf_sz, buf, K__GET(w_size, 3) ? (EFI_MAC_ADDRESS*)_args[2] : NULL, (EFI_MAC_ADDRESS*)_args[4], (uint16_t*)_args[1]);

	if (UNLIKELY(res)) {
		*(void**)buf = s_bufs;
		s_bufs = buf;
	}
	K__RES(LIKELY(!res) ? W_SUCCESS : (res == EFI_NOT_READY ? W_ENOTREADY : W_EFAIL));
}
static const w_fn_sign_val hw_netif_receive_sign[] = {ST_LEN, ST_PTR, ST_ARR, ST_LEN, ST_ARR, ST_LEN, ST_BIO, ST_LEN, ST_PTR};
static K_SIGNED_HDL(hw_netif_receive) {
	EFI_SIMPLE_NETWORK_PROTOCOL* netif = netif_get(K__GET(w_size, 0));
	if (UNLIKELY(!netif ||
		(K__GET(w_size, 3) && K__GET(w_size, 3) < netif->Mode->HwAddressSize) ||
		(K__GET(w_size, 5) && K__GET(w_size, 5) < netif->Mode->HwAddressSize)
	)) K__RES(W_EINVAL);
	if (UNLIKELY(!netif_set_ready(netif))) K__RES(W_EFAIL);

	static uint8_t s_buf[PK_SZ] = {0};

	uintn_t header_sz, buf_sz = PK_SZ;
	EFI_STATUS res = netif->Receive(netif,&header_sz, &buf_sz, s_buf,
		K__GET(w_size, 3) ? (EFI_MAC_ADDRESS*)_args[2] : NULL,
		K__GET(w_size, 5) ? (EFI_MAC_ADDRESS*)_args[4] : NULL,
		(uint16_t*)_args[1]);

	if (res) K__RES(res == EFI_NOT_READY ? W_ENOTREADY : W_EFAIL);

	{
		uint8_t* pb = s_buf + header_sz;
		buf_sz -= header_sz;
		*(w_size*)_args[8] = buf_sz;
		const k_iovec* iovs = _args[6];
		const w_size iovcnt = K__GET(w_size, 7);
		for (w_size i = 0; i < iovcnt && buf_sz; i++) {
			w_size len = iovs[i].len;
			if (len > buf_sz) len = buf_sz;
			memcpy(iovs[i].base, pb, buf_sz);
			pb += len;
			buf_sz -= len;
		}
	}

	K__RES(W_SUCCESS);
}
/*static K_SIGNED_HDL(hw_netif_wait) {
	EFI_SIMPLE_NETWORK_PROTOCOL* netif = netif_get(K__GET(w_size, 0));
	if (UNLIKELY(!netif)) K__RES(W_EINVAL);
	if (UNLIKELY(!netif_set_ready(netif))) K__RES(W_EFAIL);

	//TODO: scheduler
	EFI_STATUS res = system_table->BootServices->WaitForEvent(1, &netif->WaitForPacket, NULL);

	K__RES(LIKELY(!res) ? W_SUCCESS : W_EFAIL);
}*/

static k_signed_call_table hw_feats = {
	NULL, 6, {
		{hw_key_read, NULL, {"hw", "key_read", 1, 0, NULL}},
		{hw_netif_cnt, NULL, {"hw", "netif_cnt", 1, 0, NULL}},
		{hw_netif_info, NULL, {"hw", "netif_info", 1, 7, hw_netif_info_sign}},
		{hw_netif_stop, NULL, {"hw", "netif_stop", 1, 1, NULL}},
		{hw_netif_transmit, NULL, {"hw", "netif_transmit", 1, 8, hw_netif_transmit_sign}},
		{hw_netif_receive, NULL, {"hw", "netif_receive", 1, 9, hw_netif_receive_sign}}
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
