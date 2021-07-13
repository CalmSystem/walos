#ifndef __EXEC_H
#define __EXEC_H

#include "stdint.h"
typedef struct {
    const char *service;
    const uint8_t *path;
} PROGRAM_RIGHT;
typedef struct {
    const uint8_t *code;
    uint32_t code_size;
    PROGRAM_RIGHT *rights;
    uint32_t rights_size;
} PROGRAM;

typedef void EXEC_INST;

enum EXEC_FLAGS {
    X_NONE = 0,
    X_START = 1 << 0
};

typedef struct exec_engine_t {
    EXEC_INST* (*load)(struct exec_engine_t*, PROGRAM*, int flags);
    int (*call)(struct exec_engine_t*, EXEC_INST*, const char* name, uint32_t argc, uint32_t* argv);
} EXEC_ENGINE;

#endif
