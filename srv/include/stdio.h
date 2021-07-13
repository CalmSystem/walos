#ifndef __STDIO_H
#define __STDIO_H

#include "srv.h"
#include "string.h"

static inline void putbytes(const uint8_t *data, size_t len) {
    srv_send("screen:out", data, len);
}

static inline int puts(const char* s) {
    int len = strlen(s);
    putbytes((const uint8_t *)s, len);
    return len;
}

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
