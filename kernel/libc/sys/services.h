#ifndef __SYS_SERVICES_H
#define __SYS_SERVICES_H

#include "sys/types.h"
#include "exec.h"

/** Send data to service defined by path */
ssize_t service_send(const char *path, struct iovec *iovec, size_t iovcnt, const PROGRAM_RIGHTS* rights);

#endif
