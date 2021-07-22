#ifndef __EXEC_H
#define __EXEC_H

typedef struct {
    const char *service;
    const char *path;
} PROGRAM_RIGHT;
typedef struct {
    PROGRAM_RIGHT *ptr;
    uint32_t count;
} PROGRAM_RIGHTS;
/** Program context (code and rights) */
typedef struct {
    const uint8_t *code;
    uint32_t code_size;
    PROGRAM_RIGHTS rights;
} PROGRAM;

/** Program instance aka process */
typedef void EXEC_INST;

#endif
