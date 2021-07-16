#define SRV_PACKET_COUNT 0
#include "service.h"
#include "extern.h"
#include "string.h"

#define hello "Hello world !\n"
void _start() {
    srv_send("echo:", (const uint8_t*)hello, strlen(hello));
}

int32_t handle(const uint8_t* sub, const uint8_t* data, uint32_t len) {
    __builtin_unreachable();
}
