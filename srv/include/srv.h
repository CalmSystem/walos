#ifndef __SRV_H
#define __SRV_H

#include "native.h"
#ifdef __cplusplus
extern "C" {
#endif

int srv_send(const char *path, const uint8_t *data, size_t len)
    __attribute__((__import_module__("srv"), __import_name__("send")));

#ifdef __cplusplus
}
#endif

#endif
