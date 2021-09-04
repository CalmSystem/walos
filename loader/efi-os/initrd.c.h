#include <stddef.h>
#include <string.h>

static struct fake_initrd* initrd = NULL;
static size_t loader_srv_list(struct loader_srv_file_t* files, size_t nfiles, size_t offset) {
	if (!initrd || offset >= initrd->count) return 0;

	if (nfiles > initrd->count - offset)
		nfiles = initrd->count - offset;

	memcpy(files, initrd->list + offset, nfiles * sizeof(*files));
	return nfiles;
}
static size_t loader_srv_read(cstr name, uint8_t *ptr, size_t len, size_t offset) {
	return 0; /* Data already in srv_list */
}
