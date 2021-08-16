#ifndef __FAT_FS_H
#define __FAT_FS_H

#include "fs.h"
#include "disk.h"

enum fat_type {
    FAT_ANY, FAT_12, FAT_16, FAT_32, FAT_ALL
};

/** Load fat filesystem */
fs *fat_mount(disk *d);
/** Auto create fat filesystem */
int fat_mkfs(disk *d);

#endif
