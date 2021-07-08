#include "console.h"
#include "protocol/efi-gop.h"
#include "font_vga.h"
#include "string.h"

void (*_putbytes)(const char *s, int len);
void console_putbytes(const char *s, int len) {
    if (_putbytes) _putbytes(s, len);
}

int tochar16(const char* s, char16_t *res) {
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

EFI_SYSTEM_TABLE *efi_system_table;

void efi_putbytes(const char *s, int len) {
    int done = 0;
    // Convert blocks of char to wchar
    while(done < len) {
        char16_t buffer[16] = {0};
        for(int n=0; n < 15 && done < len; n++) {
            // Convert \n to \r\n
            if (s[done] == '\n') {
                if (n < 14) {
                    buffer[n] = L'\r';
                    n++;
                    buffer[n] = L'\n';
                    done++;
                }
                continue;
            }
            int res = tochar16(s+done, &buffer[n]);
            if (!res) done = len;
            done += res;
        }
        efi_system_table->ConOut->OutputString(efi_system_table->ConOut, buffer);
    }
}

void console_use_efi(EFI_SYSTEM_TABLE *st) {
    efi_system_table = st;
    _putbytes = efi_putbytes;
}

uint32_t* gop_buffer;
EFI_GRAPHICS_OUTPUT_MODE_INFORMATION gop_info;
struct gop_state_t gop_state;
unsigned char *vga_font;

inline uint32_t gop_pixel_pack(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t p = gop_info.PixelFormat ? b : r;
    p |= (g << 8);
    p |= ((gop_info.PixelFormat ? r : b) << 16);
    return p;
}
inline uint32_t* gop_pixel_addr(uint32_t x, uint32_t y) {
    return gop_buffer + gop_info.PixelsPerScanLine * y + x;
}

#define FONT_X 8
#define FONT_Y 8
#define COLOR_BLACK 0
#define COLOR_SIZE (1 << 24)
#define COLOR_WHITE (COLOR_SIZE-1)

void gop_draw_vga_char(uint32_t dx, uint32_t dy, char c, uint32_t fg, uint32_t bg) {
    for (uint32_t y = 0; y < FONT_Y; y++) {
        for (uint32_t x = 0; x < FONT_X; x++) {
            uint32_t *px = gop_pixel_addr(dx+x, dy+y);
            if (font_vga[c*FONT_Y+y] & (1 << x)) {
                *px = fg;
            } else if (bg < COLOR_SIZE) {
                *px = bg;
            }
        }
    }
}

void gop_flush(uint32_t from, uint32_t count, uint32_t bg) {
    uint32_t* cur = gop_buffer + from;
    while(count--) *cur++ = bg;
}
void gop_clean() { memset(gop_buffer, 0, gop_info.HorizontalResolution*gop_info.VerticalResolution*sizeof(*gop_buffer)); }
void gop_scroll(uint32_t size) {
    const uint32_t bottom = gop_info.VerticalResolution*gop_info.HorizontalResolution - size;
    memmove(gop_buffer, gop_buffer + size, bottom*sizeof(*gop_buffer));
    memset(gop_buffer + bottom, 0, size*sizeof(*gop_buffer));
}

void gop_putstr(const char *s, int len, struct gop_state_t* state) {
    for(int i = 0; i < len && s[i]; i++) {
        switch (s[i])
        {
        case 8: //back
            if (state->x > 0) state->x--;
            break;

        case 9: //tab
            state->x = ((state->x / 8) + 1) * 8;
            if (state->x >= gop_info.HorizontalResolution/FONT_X) state->x = (gop_info.HorizontalResolution/FONT_X)-1;
            break;

        case 10: //line break
            state->x = 0;
            state->y++;
            break;

        case 12: //flush
            state->x = 0;
            state->y = 0;
            state->fg = COLOR_WHITE;
            state->bg = COLOR_BLACK;
            gop_clean();
            break;

        case 13: //carriage return
            state->x = 0;
            break;

        default: {
            gop_draw_vga_char(state->x*FONT_X, state->y*FONT_Y, s[i], state->fg, state->bg);
            state->x++;
            break;
        }
        }
        if (state->x >= gop_info.HorizontalResolution/FONT_X) {
            state->x = 0;
            state->y++;
        }
        if (state->y >= gop_info.VerticalResolution/FONT_Y) {
            gop_scroll(gop_info.HorizontalResolution*FONT_Y);
            state->y = (gop_info.VerticalResolution/FONT_Y) - 1;
        }
    }
}
void gop_putbytes(const char *s, int len) { gop_putstr(s, len, &gop_state); }

EFI_STATUS console_use_gop(EFI_SYSTEM_TABLE *st) {
    EFI_STATUS status;
    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    
    status = st->BootServices->LocateProtocol(&gopGuid, 0, (void**)&gop);
    if(EFI_ERR & status) return status;

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
    uintn_t i, SizeOfInfo, numModes, nativeMode;
 
    status = gop->QueryMode(gop, gop->Mode ? gop->Mode->Mode : 0, &SizeOfInfo, &info);
    // this is needed to get the current video mode
    if (status == EFI_NOT_STARTED)
        status = gop->SetMode(gop, 0);
    if(EFI_ERR & status) return status;
        
    nativeMode = gop->Mode->Mode;
    numModes = gop->Mode->MaxMode;

    if (GOP_MAX_HEIGHT) {
        uint32_t preferMode = 0, maxHeight = 0;
        for (i = 0; GOP_MAX_HEIGHT && i < numModes; i++) {
            status = gop->QueryMode(gop, i, &SizeOfInfo, &info);
            if (info->PixelFormat == 1 &&
                info->VerticalResolution <= GOP_MAX_HEIGHT &&
                info->VerticalResolution > maxHeight &&
                info->HorizontalResolution == GOP_RATIO_WIDTH(info->VerticalResolution))
            {
                preferMode = i;
                maxHeight = info->VerticalResolution;
            }
        }
        status = gop->SetMode(gop, preferMode);
        if(EFI_ERR & status) return status;
    }

    efi_system_table = 0;
    gop_buffer = (uint32_t*)gop->Mode->FrameBufferBase;
    gop_info = *gop->Mode->Info;
    gop_state.x = 0;
    gop_state.y = 0;
    gop_state.fg = COLOR_WHITE;
    gop_state.bg = COLOR_BLACK;
    _putbytes = gop_putbytes;
    return EFI_SUCCESS;
}
