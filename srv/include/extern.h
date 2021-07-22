#ifndef __SRV_H
#define __SRV_H

#include "stdint.h"
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __wasi__
typedef uint32_t size_t;
typedef uint32_t ptrdiff_t;
struct iovec {
    void* iov_base;
    size_t iov_len;
};
#endif

int srv_send(const char *path, const uint8_t *data, size_t len)
    __attribute__((__import_module__("srv"), __import_name__("send")));
int srv_sendv(const char *path, struct iovec *iov, size_t iovcnt, size_t* out)
    __attribute__((__import_module__("srv"), __import_name__("sendv")));

#ifdef __cplusplus
}
#endif

#endif
