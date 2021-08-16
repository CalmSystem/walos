#ifndef __DISK_H
#define __DISK_H

#include "stddef.h"

enum disk_command {
    /** Read disk_status */
    DISK_GET_STATUS,
    /** Flush pending writes */
    DISK_SYNC,
    /** Read sector size in bytes (size_t) */
    DISK_GET_SECTOR_SIZE,
    /** Read sector count (size_t) */
    DISK_GET_SECTOR_COUNT,
    /** Read block size in sectors (size_t) */
    DISK_GET_BLOCK_SIZE,
    /** Annonce block no longer contains data (size_t) */
    DISK_TRIM,
    /** Toggle DISK_OFF */
    DISK_POWER,
    /** Remove disk */
    DISK_EJECT
};

enum disk_status {
    DISK_OFF = 1 << 0,
    DISK_MISSING = 1 << 1,
    DISK_READ_ONLY = 1 << 4
};

typedef size_t sector_n;

/** Abstract disk interface */
typedef struct disk {
    int (*read)(struct disk* self, void* out, sector_n count, sector_n offset);
    int (*write)(struct disk* self, const void* in, sector_n count, sector_n offset);
    int (*cmd)(struct disk* self, enum disk_command cmd, void* io);
} disk;

static inline int disk_read(disk* d, void* out, sector_n count, sector_n offset) {
    return d->read(d, out, count, offset);
}
static inline int disk_write(disk* d, const void* in, sector_n count, sector_n offset) {
    return d->write(d, in, count, offset);
}
static inline int disk_cmd(disk* d, enum disk_command cmd, void* io) {
    return d->cmd(d, cmd, io);
}

#endif
