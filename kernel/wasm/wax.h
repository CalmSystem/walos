#ifndef __WAX_H
#define __WAX_H

#include "exec.h"

/** Virtual WASM runtime */
typedef struct exec_engine_t {
    EXEC_INST *(*srv_load)(struct exec_engine_t*, PROGRAM*);
    ssize_t (*srv_call)(EXEC_INST*, const char* sub, struct iovec*, size_t);
} EXEC_ENGINE;

/** Initialize Wasm3 execution engine */
EXEC_ENGINE* exec_load_m3();

#endif
