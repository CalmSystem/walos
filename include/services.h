#ifndef __SERVICES_H
#define __SERVICES_H

#include "exec.h"

#define SRV_SEPARATOR ':'
typedef struct {
    PROGRAM *program;
    const char *name;
    /** pointer to EXEC_ENGINE instance or
     * pointer to function if program == UINT_MAX */
    EXEC_INST *instance;
} SERVICE;

typedef struct {
    SERVICE *ptr;
    uint32_t free_services;
} SERVICE_TABLE;

typedef struct {
    uint32_t sublen;
    uint32_t datalen;
    uint8_t buf[];
} srv_packet_t;
#define SRV_PACKET_ALOC "srv_prehandle"
#define SRV_PACKET_HNDL "srv_handle"
#define SRV_PACKET_SIZE(p) ((p).sublen + (p).datalen + 1)

#endif
