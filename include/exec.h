#ifndef __EXEC_H
#define __EXEC_H

#include "stdint.h"
typedef struct {
    const char *service;
    const char *path;
} PROGRAM_RIGHT;
/** Program context (code and rights) */
typedef struct {
    const uint8_t *code;
    uint32_t code_size;
    PROGRAM_RIGHT *rights;
    uint32_t rights_size;
} PROGRAM;

/** Program instance aka process */
typedef void EXEC_INST;

typedef struct exec_engine_t {
    EXEC_INST *(*srv_load)(struct exec_engine_t*, PROGRAM*);
    int32_t (*srv_call)(EXEC_INST*, const char* sub, const uint8_t* data, uint64_t datalen);
} EXEC_ENGINE;

#endif
