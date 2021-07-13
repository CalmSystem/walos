#include "stdint.h"
#include "stdio.h"

void _start() {
    puts("Hello world !\n");
}

void handle(const uint8_t* sub, const uint8_t* data, size_t len) {
    __builtin_unreachable();
}
