#ifndef __SRV_BUILTIN
#define __SRV_BUILTIN

#include "srv.h"
#include "graphics.h"
#include "stddef.h"
#include "assert.h"

int32_t srv_stdout(const char* sub, const uint8_t* data, size_t len) {
    putbytes((const char*)data, len);
    return 1;
}

/** Bind services to internal functions */
static inline void srv_register_builtin() {
    int ok =
    srv_add_internal("stdout", srv_stdout, NULL);
    assert(ok && "Failed to load internal services");
}

#endif
