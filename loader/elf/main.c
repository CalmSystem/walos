#include <kernel/os.h>
#include "/usr/include/stdio.h"
#include "/usr/include/stdlib.h"
#include "/usr/include/unistd.h"
#include "/usr/include/dirent.h"
#include <sys/stat.h>
#include "/usr/include/string.h"

static void loader_log(cstr str, size_t len) {
	fwrite(str, len, 1, stdout);
}
static void loader_wait() {
	//TODO: wait for event
	sleep(1); /* Sleep for now */
}

static void loader_srv_list(size_t to_skip, void (*cb)(void *arg, const struct loader_srv_file_t *files, size_t nfiles), void *arg) {

	DIR *srv_dir = opendir("srv");
	if (!srv_dir) {
		cb(arg, NULL, 0);
		return;
	}

	struct dirent* info;
	while (1) {
		if ((info = readdir(srv_dir)) == NULL) {
			/* No more files */
			cb(arg, NULL, 0);
			break;
		}

		if (info->d_type != DT_REG) continue;

		if (to_skip > 0) {
			to_skip--;
			continue;
		}

		struct loader_srv_file_t srv = {info->d_name, 0, NULL};
		{
			struct stat st = {0};
			size_t d_len = strlen(info->d_name);
			char path[5 + d_len];
			memcpy(path, "srv/", 4);
			memcpy(path+4, info->d_name, d_len+1);
			stat(path, &st);
			srv.size = st.st_size;
		}
		cb(arg, &srv, 1);
		break;
	}
	closedir(srv_dir);

}
static void loader_srv_read(cstr name, void *ptr, size_t len, size_t offset, void (*cb)(void *arg, size_t read), void *arg) {

	FILE *file = NULL;
	{
		size_t d_len = strlen(name);
		char path[7 + d_len];
		memcpy(path, "./srv/", 6);
		memcpy(path + 6, name, d_len + 1);
		file = fopen(path, "r");
	}
	if (!file) {
		cb(arg, 0);
		return;
	}

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	if (size > offset) {
		if (len > size - offset) len = size - offset;
		fseek(file, offset, SEEK_SET);
		len = fread(ptr, 1, len, file);
	} else 
		len = 0;

	fclose(file);
	cb(arg, len);

}

/* NOTE: system libc replace kernel malloc and free.
 * Those functions are only useful for an eventual page_alloc */
static page_t* loader_alloc(size_t n_pages) {
	return (page_t*)aligned_alloc(PAGE_SIZE, n_pages * PAGE_SIZE);
}
static void loader_free(page_t* first, size_t n_pages) {
	(void)n_pages;
	free(first);
}

int main(int argc, char const *argv[]) {
	puts("ELF loader");

	struct os_ctx_t os_ctx = {
		{
			.log=loader_log,
			.wait=loader_wait,
			.take_pages=loader_alloc,
			.release_pages=loader_free,
			.srv_list=loader_srv_list,
			.srv_read=loader_srv_read
		}, {
			NULL, 0
		}
	};
	os_entry(&os_ctx);
	puts("OS Stopped");
	return 0;
}
