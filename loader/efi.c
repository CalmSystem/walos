#include "protocol/efi-gop.h"
#include "efi-tools.c.h"
#include "loader.h"
#include "elf.h"
#include <stddef.h>

static inline int memcmp(const void* aptr, const void* bptr, size_t n){
    const unsigned char* a = aptr, *b = bptr;
    for (size_t i = 0; i < n; i++){
        if (a[i] < b[i]) return -1;
        else if (a[i] > b[i]) return 1;
    }
    return 0;
}

static inline EFI_STATUS load_kernel(char16_t* path, Elf64_Addr* entry) {
    EFI_FILE_PROTOCOL* file = open_file(NULL, path);
    if (!file) return 1;

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
        return 1;
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
    *entry = header.e_entry;

    system_table->BootServices->FreePool(phdrs);
    file->Close(file);
    return EFI_SUCCESS;
}

static inline PSF1_FONT* load_psf1_font(char16_t* path) {
    EFI_FILE_PROTOCOL* file = open_file(NULL, path);
    if (!file) return NULL;

    PSF1_HEADER* font_header;
    system_table->BootServices->AllocatePool(EfiRuntimeServicesData, sizeof(PSF1_HEADER), (void**)&font_header);
    uintn_t size = sizeof(PSF1_HEADER);
    file->Read(file, &size, font_header);

    if (font_header->magic[0] != PSF1_MAGIC0 || font_header->magic[1] != PSF1_MAGIC1){
        return NULL;
    }

    uintn_t glyph_buffer_size = font_header->charsize * 256;
    if (font_header->mode == 1) { //512 glyph mode
        glyph_buffer_size = font_header->charsize * 512;
    }

    void* glyphBuffer;
    {
        file->SetPosition(file, sizeof(PSF1_HEADER));
        system_table->BootServices->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData,
            EFI_TO_PAGES(glyph_buffer_size), (EFI_PHYSICAL_ADDRESS *)&glyphBuffer);
        file->Read(file, &glyph_buffer_size, glyphBuffer);
    }

    file->Close(file);
    PSF1_FONT *ret;
    system_table->BootServices->AllocatePool(EfiRuntimeServicesData, sizeof(PSF1_FONT), (void**)&ret);
    ret->header = font_header;
    ret->glyph_buffer = glyphBuffer;
    return ret;
}

static PROGRAM* load_service_program(char16_t* path, PROGRAM** pgm) {
    EFI_FILE_PROTOCOL* file = open_file(NULL, path);
    if (!file) {
        println(L"Missing service file");
        return NULL;
    }

    uint8_t* code;
    uint64_t file_size = get_file_size(file);
    system_table->BootServices->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, EFI_TO_PAGES(file_size), (EFI_PHYSICAL_ADDRESS *)&code);
    file->Read(file, &file_size, code);

    (*pgm)->code = code;
    (*pgm)->code_size = file_size;

    file->Close(file);
    return (*pgm)++;
}

static inline void *memset(void *dst, int c, size_t n) {
    char *q = dst;

#if defined(__i386__)
    size_t nl = n >> 2;
    __asm__ __volatile__ ("cld ; rep ; stosl ; movl %3,%0 ; rep ; stosb"
              : "+c" (nl), "+D" (q)
              : "a" ((unsigned char)c * 0x01010101U), "r" (n & 3));
#elif defined(__x86_64__)
    size_t nq = n >> 3;
    __asm__ __volatile__ ("cld ; rep ; stosq ; movl %3,%%ecx ; rep ; stosb"
              :"+c" (nq), "+D" (q)
              : "a" ((unsigned char)c * 0x0101010101010101U),
            "r" ((uint32_t) n & 7));
#else
    while (n--) {
        *q++ = c;
    }
#endif

    return dst;
}

static inline SERVICE_TABLE load_services(char16_t* idx_path) {
    SERVICE_TABLE tb = {0};
    EFI_FILE_PROTOCOL* file = open_file(NULL, idx_path);
    if (!file) return tb;

    char *idx;
    {
        uint64_t file_size = get_file_size(file);
        system_table->BootServices->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, EFI_TO_PAGES(file_size), (EFI_PHYSICAL_ADDRESS *)&idx);
        file->Read(file, &file_size, idx);
    }
    file->Close(file);

    uintn_t n_services = 0;
    uintn_t n_rights = 0;
    uintn_t longest_path = 0;
    {
        uint32_t commas = 0;
        uintn_t current_path = 0;
        for (const char* p = idx; *p; p++) {
            if (*p == ',') {
                if (commas == 1 && current_path > longest_path)
                    longest_path = current_path;

                commas++;
            } else if (*p == '\n') {
                if (commas) {
                    n_services++;
                    n_rights += commas-1;
                    commas = 0;
                    current_path = 0;
                }
            } else if (commas == 1) {
                char16_t trash;
                int res = tochar16(p, &trash);
                if (res > 0) {
                    p += (res-1);
                    longest_path++;
                }
            }
        }
    }

    char16_t *path;
    system_table->BootServices->AllocatePool(EfiLoaderData, longest_path*sizeof(char16_t), (void **)&path);

    uintn_t srv_size = (n_services+1) * sizeof(SERVICE);
    uintn_t pgm_size = n_services * sizeof(PROGRAM);
    uintn_t out_size = srv_size + pgm_size + n_rights * sizeof(PROGRAM_RIGHT);
    system_table->BootServices->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, EFI_TO_PAGES(out_size), (EFI_PHYSICAL_ADDRESS *)&tb.ptr);
    memset(tb.ptr, 0, EFI_TO_PAGES(out_size) * 0x1000);

    SERVICE *srv = tb.ptr;
    tb.free_services = (EFI_TO_PAGES(out_size) * 0x1000 - out_size) / sizeof(SERVICE);
    PROGRAM *pgm = (PROGRAM *)(srv + n_services + 1 + tb.free_services);
    PROGRAM_RIGHT *rgt = (PROGRAM_RIGHT *)(pgm + n_services);

    uint32_t commas = 0;
    char16_t *current_path = path;
    srv->name = idx;
    for (char* p = idx; *p; p++) {
        if (*p == ',') {
            *p = '\0';
            if (commas == 1) {
                *current_path = L'\0';
                srv->program = load_service_program(path, &pgm);
            }
            if (commas >= 1 && srv->program) {
                if (!srv->program->rights) {
                    srv->program->rights = rgt;
                    srv->program->rights_size = 1;
                } else {
                    rgt++;
                    srv->program->rights_size++;
                }
                rgt->service = p+1;
            }
            commas++;
        } else if (*p == '\n') {
            *p = '\0';
            if (commas == 1) {
                *current_path = L'\0';
                srv->program = load_service_program(path, &pgm);
            }
            if (commas > 1) rgt++;
            commas = 0;
            current_path = path;
            if (srv->program) srv++;
            srv->name = p+1;
        } else if (commas == 1) {
            int res = tochar16(p, current_path);
            if (res > 0) {
                if (*current_path == '/' && *(current_path-1) != '\\')
                    *current_path = '\\';

                p += (res-1);
                current_path++;
            }
        } else if (*p == ':' && commas > 1 && srv->program && !rgt->path) {
            *p = '\0';
            rgt->path = (uint8_t*)p+1;
        }
    }

    system_table->BootServices->FreePool(path);
    return tb;
}

static inline EFI_STATUS load_gop(LINEAR_FRAMEBUFFER* out) {
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

EFI_STATUS efi_main(EFI_HANDLE ih, EFI_SYSTEM_TABLE *st) {
    system_table = st;
    image_handle = ih;

    println(L"Walos EFI loader");

    BOOT_INFO bootinfo = {0};
    Elf64_Addr kernel_entry;
    EFI_STATUS status;

    status = load_kernel(WSTR(K_PATH), &kernel_entry);
    if (status & EFI_ERR) return EFI_SUCCESS;

    println(L"Kernel: Loaded");

    bootinfo.font = load_psf1_font(WSTR(FONT_PATH));
    if (bootinfo.font) println(L"Font: Loaded");

    bootinfo.services = load_services(WSTR(SRV_PATH));
    if (bootinfo.services.ptr) println(L"Services: Loaded");

    LINEAR_FRAMEBUFFER gop;
    status = load_gop(&gop);
    if (!(status & EFI_ERR)) {
        bootinfo.lfb = &gop;
        println(L"GOP: Loaded");
    }

    EFI_MEMORY_MAP mmap = {0};
    uintn_t map_key;
    {
        EFI_MEMORY_DESCRIPTOR* map = NULL;
        uint32_t desc_version;
        st->BootServices->GetMemoryMap(&mmap.size, map, &map_key, &mmap.desc_size, &desc_version);
        st->BootServices->AllocatePool(EfiRuntimeServicesData, mmap.size, (void**)&map);
        st->BootServices->GetMemoryMap(&mmap.size, map, &map_key, &mmap.desc_size, &desc_version);
        mmap.ptr = map;
    }
    bootinfo.mmap = &mmap;
    println(L"Mmap: Loaded");

    __attribute__((sysv_abi)) void (*kernel_start)(BOOT_INFO*) = (__attribute__((sysv_abi)) void (*)(BOOT_INFO*))kernel_entry;

    println(L"Exit to kernel");

    st->BootServices->ExitBootServices(ih, map_key);
    kernel_start(&bootinfo);

    while(1);
}
