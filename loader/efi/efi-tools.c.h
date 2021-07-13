#ifndef __EFI_TOOLS_C_H
#define __EFI_TOOLS_C_H

#include "efi.h"
#include "protocol/efi-lip.h"
#include "protocol/efi-sfsp.h"
#include "mmap.h"
#include <stddef.h>

EFI_SYSTEM_TABLE* system_table;
EFI_HANDLE image_handle;

#define CAT(A, B)   A##B
#define WSTR(A)  CAT(L, A)

static inline int tochar16(const char* s, char16_t *res) {
    const int c = 0;
    *res = '\0';
    char first = s[c];
    if ((first & 0x80) == 0) {
        *res = (char16_t)s[c];
        return 1;
    }
    else if ((first & 0xe0) == 0xc0) {
        *res |= first & 0x1f;
        *res <<= 6;
        *res |= s[c+1] & 0x3f;
        return 2;
    }
    else if ((first & 0xf0) == 0xe0) {
        *res |= first & 0x0f;
        *res <<= 6;
        *res |= s[c+1] & 0x3f;
        *res <<= 6;
        *res |= s[c+2] & 0x3f;
        return 3;
    }
    else if ((first & 0xf8) == 0xf0) {
        *res |= first & 0x07;
        *res <<= 6;
        *res |= s[c+1] & 0x3f;
        *res <<= 6;
        *res |= s[c+2] & 0x3f;
        *res <<= 6;
        *res |= s[c+3] & 0x3f;
        return 4;
    }
    else {
        return -1;
    }
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

static void printd(uint64_t dec) {
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
static void printx(uint64_t hex, uint8_t width) {
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

void printmmap(EFI_MEMORY_MAP* m) {
    uint8_t                    *mm = (uint8_t*)m->ptr;
    EFI_MEMORY_DESCRIPTOR   *mem_map;
    uint64_t                i;
    uint64_t                total_mapped = 0;

    uint64_t _mem_map_num_entries = m->size / m->desc_size;
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
        mm += m->desc_size;
    }
    print(L"Total memory mapped ... ");
    printd(total_mapped);
    print(L"\n\r");
}

#define EFI_TO_PAGES(size) ((size + 0x1000 - 1) / 0x1000)

static EFI_FILE_PROTOCOL* open_file(EFI_FILE_PROTOCOL* directory, char16_t* path) {
    static EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* sfsp = NULL;

    if (!directory) {
        if (!sfsp) {
            EFI_GUID lipGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
            EFI_LOADED_IMAGE_PROTOCOL* lip;
            EFI_GUID sfspGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
            system_table->BootServices->HandleProtocol(image_handle, &lipGuid, (void**)&lip);
            system_table->BootServices->HandleProtocol(lip->DeviceHandle, &sfspGuid, (void**)&sfsp);
        }
        sfsp->OpenVolume(sfsp, &directory);
    }

    EFI_FILE_PROTOCOL* file;
    EFI_STATUS status = directory->Open(directory, &file, path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
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

#endif
