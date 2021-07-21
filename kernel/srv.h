#ifndef __SRV_H
#define __SRV_H

#include "services.h"

/** Setup services table and runtime engine */
void srv_use(SERVICE_TABLE st, EXEC_ENGINE* engine);

SERVICE *srv_find(const char *name);
SERVICE *srv_findn(const char *path, size_t name_len);
/** Add Wasm program as service */
int srv_add(const char* name, PROGRAM*, SERVICE** out);
/** Add kernel internal function as service */
int srv_add_internal(const char* name, int32_t (*fn)(const char*, const uint8_t*, size_t), SERVICE** out);
/** Send data to service defined by path
 *  Rights depends on emitter if set */
int srv_send(const char *path, const uint8_t *data, size_t len, PROGRAM* emitter);

#endif
