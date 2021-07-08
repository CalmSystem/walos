#ifndef __CONSOLE_H
#define __CONSOLE_H

#include "efi.h"
extern void console_putbytes(const char *s, int len);
void console_use_efi(EFI_SYSTEM_TABLE *system_table);
EFI_STATUS console_use_gop(EFI_SYSTEM_TABLE *system_table);

#define GOP_MAX_HEIGHT 0 // 1080
#define GOP_RATIO_WIDTH(height) (height*16/9)
struct gop_state_t {
    uint32_t x, y, fg, bg;
};
void gop_putstr(const char *s, int len, struct gop_state_t* state);

#endif
