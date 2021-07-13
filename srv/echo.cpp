#include "stdint.h"
#include "stdio.h"

extern "C" void _start() {
    puts("Echo loaded\n");
}

extern "C" void handle(const uint8_t* sub, const uint8_t* data, size_t len) {
    (void)sub;
    putbytes(data, len);
}
