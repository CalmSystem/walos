#ifndef __STDIO_H
#define __STDIO_H

#include "extern.h"
#include "string.h"

static inline void putbytes(const uint8_t *data, size_t len) {
    srv_send("stdout:", data, len);
}

static inline int putstr(const char* s) {
    int len = strlen(s);
    putbytes((const uint8_t *)s, len);
    return len;
}
static inline int puts(const char* s) {
    int len = putstr(s);
    putstr("\n");
    return len;
}
/** Compile-time puts(const char*) */
#define PUTS(s) putstr(s"\n");

#define SEND_BUF_SIZE 128
static inline void _putchar(char ch) {
    static char buf[SEND_BUF_SIZE];
    static int len = 0;

    buf[len++] = ch;
    if (len >= SEND_BUF_SIZE || ch == '\n' || ch == '\0') {
        putbytes((const uint8_t*)&buf, len);
        len = 0;
    }
}

#include "printf.h"

#endif
