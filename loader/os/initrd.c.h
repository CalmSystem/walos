#include <stddef.h>
#include <string.h>
#include <tar.h>

static const void* initrd = NULL;
static size_t loader_srv_list(struct loader_srv_file_t* files, size_t nfiles, size_t offset) {
	if (!initrd) return 0;

	void* ptr = (void*)initrd;
	while (offset-- && (ptr = tar_read(ptr, NULL)));

	size_t n = 0;
	struct tar_file_t info;
	while (n < nfiles && (ptr = tar_read(ptr, &info))) {
		memcpy((char*)files[n].name, info.name, 100);
		files[n].size = info.len;
		files[n].data = info.data;
		n++;
	}
	return n;
}
static size_t loader_srv_read(cstr name, uint8_t *ptr, size_t len, size_t offset) {
	return 0; /* Data already in srv_list */
}
