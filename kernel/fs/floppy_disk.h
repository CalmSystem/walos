#ifndef __FLOPPY_DISK_H
#define __FLOPPY_DISK_H

#include "disk.h"

/** Try to open floppy disk drive.
 * Only support main 1.44mb drive */
disk* floppy_disk_get();

#if !FLOPPY_USE_THREAD
/** Pull loop stopping floppy during inactivity */
void floppy_sleeper_pull();
#endif

#endif
