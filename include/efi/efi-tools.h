#ifndef __EFI_TOOLS_C_H
#define __EFI_TOOLS_C_H

#include "efi.h"
#include "protocol/efi-lip.h"
#include "protocol/efi-sfsp.h"
#include <stddef.h>

static EFI_SYSTEM_TABLE* system_table;
static EFI_HANDLE image_handle;

#define CAT(A, B)   A##B
#define WSTR(A)  CAT(L, A)

/** Convert first character of UTF-8(s) to UTF-16(res).
 *  Return number a UTF-8 token used or -1. */
static inline int to_utf16(const char* s, char16_t *res) {
	const int c = 0;
	if (res) *res = '\0';
	char first = s[c];
	if ((first & 0x80) == 0) {
		if (res)
			*res = (char16_t)s[c];
		return 1;
	}
	else if ((first & 0xe0) == 0xc0) {
		if (res)  {
			*res |= first & 0x1f;
			*res <<= 6;
			*res |= s[c+1] & 0x3f;
		}
		return 2;
	}
	else if ((first & 0xf0) == 0xe0) {
		if (res) {
			*res |= first & 0x0f;
			*res <<= 6;
			*res |= s[c+1] & 0x3f;
			*res <<= 6;
			*res |= s[c+2] & 0x3f;
		}
		return 3;
	}
	else if ((first & 0xf8) == 0xf0) {
		if (res) {
			*res |= first & 0x07;
			*res <<= 6;
			*res |= s[c+1] & 0x3f;
			*res <<= 6;
			*res |= s[c+2] & 0x3f;
			*res <<= 6;
			*res |= s[c+3] & 0x3f;
		}
		return 4;
	}
	else {
		return -1;
	}
}
/** Get length of s converted to UTF-16(char16_t).
 *  Return -1: invalid string, 0: ok, 1: fit in place */
static inline int strlen_utf16(const char* s, uint64_t max_len, uint64_t *ret) {
	int fit = 1;
	uint64_t i = 0, out_len = 0;
	while (i < max_len && s[i]) {
		int n = to_utf16(s + i, NULL);
		if (n < 0) return -1;
		out_len++;
		i += n;
		if (fit && out_len*2 >= i) fit = 0;
	}
	if (ret) *ret = out_len;
	return fit;
}
/** Convert valid string s to UTF-16. */
static inline void sto_utf16(const char* s, uint64_t max_len, char16_t* out) {
	uint64_t i = 0;
	while (i < max_len && s[i])
		i += to_utf16(s + i, out++);
}
/** Convert first character of UTF-16(c) to UTF-8(res).
 *  Res must at least fit 4 tokens or accurately predict token count.
 *  Return number a UTF-8 token produced or -1. */
static inline int to_utf8(char16_t c, char *res) {
	unsigned v = c;
	if (res) *res = '\0';
	if (v < 0x80) { /* ASCII */
		if (res)
			*res = c;
		return 1;
	}
	else if (v < 0x800) {
		if (res) {
			res[0] = (v >> 6) | 0xc0;
			res[1] = (v & 0x3f) | 0x80;
		}
		return 2;
	}
	else if (v < 0xd800 || v-0xe000 < 0x2000) {
		if (res) {
			res[0] = (v >> 12) | 0xe0;
			res[1] = ((v >> 6) & 0x3f) | 0x80;
			res[2] = (v & 0x3f) | 0x80;
		}
		return 3;
	}
	else if (v-0x10000 < 0x100000) {
		if (res) {
			res[0] = (v >> 18) | 0xf0;
			res[1] = ((v >> 12) & 0x3f) | 0x80;
			res[2] = ((v >> 6) & 0x3f) | 0x80;
			res[3] = (v & 0x3f) | 0x80;
		}
		return 4;
	}
	return -1;
}
/** Get length of UTF-16 s converted to UTF-8(char).
 *  Return -1: invalid string, 0: ok, 1: fit in place */
static inline int strlen_utf8(const char16_t* s, uint64_t max_len, uint64_t *ret) {
	int fit = 1;
	uint64_t i = 0, out_len = 0;
	while (i < max_len && s[i]) {
		int n = to_utf8(s[i], NULL);
		if (n < 0) return -1;
		out_len += n;
		i++;
		if (fit && out_len > i*2) fit = 0;
	}
	if (ret) *ret = out_len;
	return fit;
}
/** Convert valid string UTF-16 s to UTF-8. */
static inline void sto_utf8(const char16_t* s, uint64_t max_len, char* out) {
	for (uint64_t i = 0; i < max_len && s[i]; i++)
		out += to_utf8(s[i], out);
}

#define PRINT_BUFFER_MAX 128
static char16_t _hex_table[] = {
	L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7',
	L'8', L'9', L'a', L'b', L'c', L'd', L'e', L'f'
};

static inline void print(char16_t* s) {
	system_table->ConOut->OutputString(system_table->ConOut, s);
}
static inline void println(char16_t* s) {
	print(s);
	print(L"\r\n");
}

static inline void printd(uint64_t dec) {
	int16_t i = PRINT_BUFFER_MAX - 1;
	char16_t buffer[PRINT_BUFFER_MAX];

	buffer[i--] = 0x0000;

	do {
		buffer[i--] = L'0' + dec % 10;
		dec /= 10;
	} while (dec > 0 && i >= 0);
	i++;

	print(buffer + i);
}
static inline void printx(uint64_t hex, uint8_t width) {
	int16_t i = width + 1;
	char16_t buffer[PRINT_BUFFER_MAX];

	if (i >= PRINT_BUFFER_MAX) {
		return;
	}

	buffer[i--] = 0x0000;
	buffer[i--] = L'h';

	while (width && i >= 0) {
		buffer[i--] = _hex_table[hex & 0x0f];
		hex >>= 4;
		width--;
	}

	print(buffer);
}

static inline void putsn(const char* str, size_t len) {
	size_t done = 0;
	char16_t buf[32];
	while (len > done) {
		/* memset */
		for (size_t i = 0; i < sizeof(buf)/sizeof(buf[0]); i++)
			buf[i] = 0;

		for (size_t i = 0; len > done && i < sizeof(buf)/sizeof(buf[0])-1; i++) {
			if (*(str + done) == '\n') {
				if (i < sizeof(buf)/sizeof(buf[0])-2) {
					buf[i++] = '\r';
				} else break;
			}

			const int csize = to_utf16(str + done, &buf[i]);
			if (csize > 0) {
				done += csize;
			} else {
				done = len;
			}
		}
		print(buf);
	}
}

static char16_t *_tab_str = L"    ";
static char16_t *_header_str = L"physical address     virtual address      pages                type";
static char16_t *_mem_attribute[] = {
	L"reserved",
	L"loader code",
	L"loader data",
	L"boot services code",
	L"boot services data",
	L"runtime services code",
	L"runtime services data",
	L"conventional memory",
	L"unusable memory",
	L"acpi reclaim memory",
	L"acpi memory nvs",
	L"memory mapped io",
	L"memory mapped io port space",
	L"pal code",
	L"persistent memory"
};
static char16_t *_mem_attribute_unrecognized = L"unrecognized";

static inline void print_mmap(void* ptr, uint64_t size, uint64_t desc_size) {
	uint8_t                    *mm = (uint8_t*)ptr;
	EFI_MEMORY_DESCRIPTOR   *mem_map;
	uint64_t                i;
	uint64_t                total_mapped = 0;

	uint64_t _mem_map_num_entries = size / desc_size;
	println(_header_str);
	for (i = 0; i < _mem_map_num_entries; i++) {
		mem_map = (EFI_MEMORY_DESCRIPTOR *)mm;
		printx(mem_map->PhysicalStart, 16);
		print(_tab_str);
		printx(mem_map->NumberOfPages, 16);
		print(_tab_str);
		if (mem_map->Type >= EfiMaxMemoryType) {
			println(_mem_attribute_unrecognized);
		} else {
			println(_mem_attribute[mem_map->Type]);
		}
		total_mapped += mem_map->NumberOfPages * 4096;
		mm += desc_size;
	}
	print(L"Total memory mapped ... ");
	printd(total_mapped);
	print(L"\n\r");
}

#define EFI_TO_PAGES(size) ((size + 0x1000 - 1) / 0x1000)

__attribute__((__unused__)) static EFI_FILE_PROTOCOL *open_file(EFI_FILE_PROTOCOL *parent, char16_t *path) {
	static EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* sfsp = NULL;

	EFI_FILE_PROTOCOL* directory, *file;
	if (!parent) {
		if (!sfsp) {
			EFI_GUID lipGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
			EFI_LOADED_IMAGE_PROTOCOL* lip;
			EFI_GUID sfspGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
			system_table->BootServices->HandleProtocol(image_handle, &lipGuid, (void**)&lip);
			system_table->BootServices->HandleProtocol(lip->DeviceHandle, &sfspGuid, (void**)&sfsp);
		}
		sfsp->OpenVolume(sfsp, &directory);
	} else {
		directory = parent;
	}

	EFI_STATUS status = directory->Open(directory, &file, path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
	if (!parent) directory->Close(directory);
	if (status & EFI_ERR) {
		print(L"Cannot open file '");
		print(path);
		println(L"'");
		return NULL;
	}

	return file;
}

static inline uint64_t get_file_size(EFI_FILE_PROTOCOL* file) {
	uint64_t start, size;
	file->GetPosition(file, &start);
	file->SetPosition(file, UINT64_MAX);
	file->GetPosition(file, &size);
	file->SetPosition(file, start);
	return size;
}

/** Ignore stack checks (unsafe) */
void __chkstk() {}
/** Hack float support */
int32_t _fltused = 0;

#endif
