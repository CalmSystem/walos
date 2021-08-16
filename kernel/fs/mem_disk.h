#ifndef __MEM_DISK_H
#define __MEM_DISK_H

#include "disk.h"
#include "stdbool.h"

/** Create a in-memory disk.
 * Allocated initially or dynamicly. */
disk* mem_disk_create(size_t npages, bool sparse);

#endif
