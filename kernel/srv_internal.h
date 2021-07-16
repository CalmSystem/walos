#ifndef __SRV_INTERNAL
#define __SRV_INTERNAL

#include "srv.h"
#include "graphics.h"
#include "stddef.h"
#include "assert.h"

int32_t srv_stdout(const char* sub, const uint8_t* data, size_t len) {
    putbytes((const char*)data, len);
    return 1;
}

static inline void srv_add_internals() {
    int ok =
    srv_add_internal("stdout", srv_stdout, NULL);
    assert(ok && "Failed to load internal services");
}

#endif
