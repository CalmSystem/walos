#ifndef __SRV_BUILTIN
#define __SRV_BUILTIN

#include "srv.h"
#include "graphics.h"
#include "stddef.h"
#include "assert.h"

ssize_t srv_stdout(const char* sub, struct iovec* iov, size_t iovcnt) {
    ssize_t written = 0;
    for (size_t i = 0; i < iovcnt; i++) {
        putbytes((const char*)iov[i].iov_base, iov[i].iov_len);
        written += iov[i].iov_len;
    }
    return written;
}

/** Bind services to internal functions */
static inline void srv_register_builtin() {
    int ok =
    srv_add_internal("stdout", srv_stdout, NULL);
    assert(ok && "Failed to load internal services");
}

#endif
