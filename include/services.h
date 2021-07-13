#ifndef __SERVICES_H
#define __SERVICES_H

#include "exec.h"

#define SRV_SEPARATOR ':'
typedef struct {
    PROGRAM *program;
    const char *name;
    EXEC_INST *instance;
} SERVICE;

typedef struct {
    SERVICE *ptr;
    uint32_t free_services;
} SERVICE_TABLE;

#endif
