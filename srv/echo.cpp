#include "service.h"
#include "stdio.h"

extern "C" void _start() {
    puts("Echo loaded");
}

extern "C" int32_t handle(const uint8_t* sub, const uint8_t* data, uint32_t len) {
    (void)sub;
    putbytes(data, len);
    return 1;
}
