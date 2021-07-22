#include <stdio.h>
#define SRV_PACKET_COUNT 0
#include "service.h"
#include "extern.h"
#include "string.h"

#define hi_echo "Hi echo !\n"
#define hi_std "Hi send !"
int main() {
    srv_send("echo:", (const uint8_t*)hi_echo, strlen(hi_echo));
    puts(hi_std);
    return 1;
}

int32_t handle(const uint8_t* sub, const uint8_t* data, uint32_t len) {
    __builtin_unreachable();
}
