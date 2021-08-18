#ifndef __KERNEL_OS_H
#define __KERNEL_OS_H

#include "../mod/fn.h"
#include "../mod/loader.h"

/** Informations given to os_entry */
struct os_ctx_t {
	const module_fn *fns; /* Array of hardware handled functions */
	uintw_t fncnt; /* Size of fns */
};

void os_entry(struct os_ctx_t*);

#endif
