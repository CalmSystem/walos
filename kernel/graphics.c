#include "graphics.h"
#include "string.h"
#include "stdio.h"
#include "assert.h"

PUTBYTES_STATE pb_state = {0};

struct {
    void (*clear)();
    void (*scroll)(uint32_t);
    void (*draw_char)(uint32_t, uint32_t, char, uint32_t, uint32_t);
    uint32_t screen_width, screen_height;
    uint16_t char_width, char_height;
} graphics;

inline void reset_state(PUTBYTES_STATE* st) {
    st->x = 0;
    st->y = 0;
    st->fg = RGB_WHITE;
    st->bg = RGB_BLACK;
}

void putbytes(const char *s, int len) { putbytes_at(s, len, &pb_state); }
void putbytes_at(const char *s, int len, PUTBYTES_STATE* st) {
    for(int i = 0; i < len && s[i]; i++) {
        switch (s[i])
        {
        case 8: //back
            if (st->x > 0) st->x--;
            break;

        case 9: //tab
            st->x = ((st->x / 8) + 1) * 8;
            if (st->x >= graphics.screen_width/graphics.char_width) st->x = (graphics.screen_width/graphics.char_width)-1;
            break;

        case 10: //line break
            st->x = 0;
            st->y++;
            break;

        case 12: //flush
            reset_state(st);
            graphics.clear();
            break;

        case 13: //carriage return
            st->x = 0;
            break;

        default: {
            graphics.draw_char(st->x*graphics.char_width, st->y*graphics.char_height, s[i], st->fg, st->bg);
            st->x++;
            break;
        }
        }
        if (st->x >= graphics.screen_width/graphics.char_width) {
            st->x = 0;
            st->y++;
        }
        if (st->y >= graphics.screen_height/graphics.char_height) {
            graphics.scroll(graphics.screen_width*graphics.char_height);
            st->y = (graphics.screen_height/graphics.char_height) - 1;
        }
    }
}

void dummy_draw_char(uint32_t dx, uint32_t dy, char c, uint32_t fg, uint32_t bg) { }
void dummy_clear() { }
void dummy_scroll(uint32_t size) { }
void graphics_use_dummy() {
    reset_state(&pb_state);
    graphics.clear = dummy_clear;
    graphics.scroll = dummy_scroll;
    graphics.draw_char = dummy_draw_char;
    graphics.screen_width = 1920;
    graphics.screen_height = 1080;
    graphics.char_width = 8;
    graphics.char_height = 16;
}

LINEAR_FRAMEBUFFER* lfb;
PSF1_FONT* font;

inline uint32_t* lfb_pixel_addr(uint32_t x, uint32_t y) {
    return (uint32_t*)lfb->base_addr + lfb->scan_line_size * y + x;
}

void lfb_draw_char(uint32_t dx, uint32_t dy, char c, uint32_t fg, uint32_t bg) {
    uint8_t* font_p = (uint8_t*)font->glyph_buffer + (c * font->header->charsize);
    for (uint32_t y = 0; y < font->header->charsize; y++) {
        for (uint32_t x = 0; x < 8; x++) {
            uint32_t *px = lfb_pixel_addr(dx+x, dy+y);
            if (*(font_p+y) & (1 << (7-x))) {
                *px = fg;
            } else if (bg < RGB_NONE) {
                *px = bg;
            }
        }
    }
}

void lfb_clear() { memset(lfb->base_addr, 0, lfb->width*lfb->height*sizeof(uint32_t)); }
void lfb_scroll(uint32_t size) {
    const uint32_t bottom = lfb->width*lfb->height - size;
    memmove(lfb->base_addr, (uint32_t*)lfb->base_addr + size, bottom*sizeof(uint32_t));
    memset((uint32_t*)lfb->base_addr + bottom, 0, size*sizeof(uint32_t));
}

void graphics_use_lfb(LINEAR_FRAMEBUFFER* fb, PSF1_FONT* ft) {
    assert(fb && ft);
    lfb = fb;
    font = ft;
    reset_state(&pb_state);
    graphics.clear = lfb_clear;
    graphics.scroll = lfb_scroll;
    graphics.draw_char = lfb_draw_char;
    graphics.screen_width = lfb->width;
    graphics.screen_height = lfb->height;
    graphics.char_width = 8;
    graphics.char_height = ft->header->charsize;
}
