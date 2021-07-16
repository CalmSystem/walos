#ifndef __SERVICE_H
#define __SERVICE_H

#include "stdint.h"

/** Number of recursive calls to handle allowed */
#ifndef SRV_PACKET_COUNT
#define SRV_PACKET_COUNT 1
#endif
/** Maximum size of srv_packet_t.sublen + srv_packet_t.datalen + 1 */
#ifndef SRV_PACKET_SIZE
#define SRV_PACKET_SIZE (64-12)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t sublen;
    uint32_t datalen;
    uint8_t buf[SRV_PACKET_SIZE];
} srv_packet_t;
typedef struct {
    srv_packet_t packet;
    uint8_t flying;
} srv_cell_t;

srv_packet_t* srv_prehandle() {
    static srv_cell_t memory[SRV_PACKET_COUNT] = {};
    for (uint32_t i = 0; i < SRV_PACKET_COUNT; i++) {
        if (!memory[i].flying) {
            memory[i].flying = 1;
            memory[i].packet.sublen = SRV_PACKET_SIZE - 1;
            memory[i].packet.datalen = 0;
            return &memory[i].packet;
        }
    }
    return 0;
}
int32_t handle(const uint8_t* sub, const uint8_t* data, uint32_t len);
int32_t srv_handle(srv_packet_t* in) {
    int32_t ret = handle(&in->buf[0], &in->buf[in->sublen + 1], in->datalen);
    ((srv_cell_t *)in)->flying = 0;
    return ret;
}

#ifdef __cplusplus
}
#endif

#endif
