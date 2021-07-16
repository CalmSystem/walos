#ifndef __SRV_H
#define __SRV_H

#include "services.h"

void srv_use(SERVICE_TABLE st, EXEC_ENGINE* engine);

SERVICE *srv_find(const char *name);
SERVICE *srv_findn(const char *path, size_t name_len);
int srv_add(const char* name, PROGRAM*, SERVICE** out);
int srv_add_internal(const char* name, int32_t (*fn)(const char*, const uint8_t*, size_t), SERVICE** out);
int srv_send(const char *path, const uint8_t *data, size_t len, PROGRAM* emitter);

#endif
