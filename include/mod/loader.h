#ifndef __MOD_LOADER_H
#define __MOD_LOADER_H
#include "./common.h"

#define PAGE_SIZE 512

struct loader_srv_stat_t {
	cstr name;
	uintw_t size;
};
/** List available services with name and size from offset.
 *  srv_list: (uintw_t) ~> n (loader_srv_stat_t*) */
/** Get service content from offset.
 *  srv_read: (cstr, uint8_t*, uintw_t, uintw_t) ~> 0 (uintw_t) */

/** Probably output log with data and size in bytes.
 *  log: (const uint8_t*, uintw_t) -> 0 () */

/** Wait until next event.
 *  wait: () -> 0 () */

/** Allocate n*PAGE_SIZE memory block.
 *  alloc: (uintw_t) -> 0 (void*) */
/** Release n*PAGE_SIZE memory block.
 *  free: (void*, uintw_t) -> 0 () */

#endif
