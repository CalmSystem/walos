#include "main.h"

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

void console_putbytes(const char *s, int len) {
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
        system_table->ConOut->OutputString(system_table->ConOut, buffer);
    }
}
