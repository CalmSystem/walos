#ifndef __SRV_H
#define __SRV_H

#include "sys/services.h"
#include "services.h"
#include "wasm/wax.h"

/** Setup services table and runtime engine */
void srv_setup(SERVICE_TABLE st, EXEC_ENGINE* engine);

SERVICE *srv_find(const char *name);
SERVICE *srv_findn(const char *path, size_t name_len);
/** Add Wasm program as service */
int srv_add(const char* name, PROGRAM*, SERVICE** out);
/** Add kernel internal function as service */
int srv_add_internal(const char* name, ssize_t (*fn)(const char*, struct iovec*, size_t), SERVICE** out);

#endif
