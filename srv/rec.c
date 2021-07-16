#define SRV_PACKET_COUNT 19
#include "service.h"
#include "stdio.h"

void _start() { }
int32_t handle(const uint8_t* sub, const uint8_t* data, uint32_t len) {
    _putchar(*data);
    _putchar('\n');
    if (*data > '0') {
        *(uint8_t*)data -= 1;
        return srv_send("rec:", data, len);
    }
    return 0;
}
