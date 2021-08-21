#include <stddef.h>

static struct fake_initrd* initrd = NULL;
static void loader_srv_list(size_t to_skip, void (*cb)(void *arg, const struct loader_srv_file_t *files, size_t nfiles), void *arg) {

	if (initrd && to_skip < initrd->count)
		cb(arg, initrd->list + to_skip, initrd->count - to_skip);
	else
		cb(arg, NULL, 0);

}
static void loader_srv_read(cstr name, void *ptr, size_t len, size_t offset, void (*cb)(void *arg, size_t read), void *arg) {
	cb(arg, 0); /* Data already in srv_list */
}
