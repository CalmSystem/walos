#include <kernel/os.h>
#include "/usr/include/stdio.h"
#include "/usr/include/stdlib.h"
#include "/usr/include/unistd.h"
#include "/usr/include/dirent.h"
#include <sys/stat.h>
#include "/usr/include/string.h"
#include "/usr/include/termios.h"

static inline void llog_out(cstr str, unsigned len) {
	fwrite(str, len, 1, stdout);
}
#include <llog.h>

static void loader_log(cstr str, size_t len) {
	fwrite(str, len, 1, stdout);
}
static void loader_wait() {
	//TODO: wait for event
	sleep(1); /* Sleep for now */
}

static size_t loader_srv_list(struct loader_srv_file_t* files, size_t nfiles, size_t offset) {

	DIR *srv_dir = opendir("srv");
	if (!srv_dir) return 0;

	size_t read = 0;
	while (read < nfiles + offset) {
		struct dirent* info;
		if ((info = readdir(srv_dir)) == NULL) {
			/* No more files */
			break;
		}

		if (info->d_type != DT_REG) continue;

		if (offset) {
			offset--;
			continue;
		}

		struct loader_srv_file_t* file = &files[read++];
		strncpy((char*)file->name, info->d_name, sizeof(file->name));
		file->data = NULL;
		{
			struct stat st = {0};
			size_t d_len = strlen(info->d_name);
			char path[5 + d_len];
			memcpy(path, "srv/", 4);
			memcpy(path+4, info->d_name, d_len+1);
			stat(path, &st);
			file->size = st.st_size;
		}
	}
	closedir(srv_dir);
	return read;
}
static size_t loader_srv_read(cstr name, uint8_t *ptr, size_t len, size_t offset) {

	FILE *file = NULL;
	{
		size_t d_len = strlen(name);
		char path[7 + d_len];
		memcpy(path, "./srv/", 6);
		memcpy(path + 6, name, d_len + 1);
		file = fopen(path, "r");
	}
	if (!file) return 0;

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	if (size > offset) {
		if (len > size - offset)
			len = size - offset;

		fseek(file, offset, SEEK_SET);
		len = fread(ptr, 1, len, file);
	} else
		len = 0;

	fclose(file);
	return len;
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

static struct termios s_old_tio = {0};
static inline void hw_key_restore() {
	if (s_old_tio.c_lflag) {
		tcsetattr(STDIN_FILENO, TCSANOW, &s_old_tio);
	}
}
static K_SIGNED_HDL(hw_key_read) {
	if (!s_old_tio.c_lflag) {
		tcgetattr(STDIN_FILENO, &s_old_tio);
		struct termios new_tio = s_old_tio;
		new_tio.c_lflag &= (~ICANON & ~ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
	}
	K__RET(char, 0) = getchar();
	return NULL;
}

static k_signed_call_table hw_feats = {
	NULL, 1, {
		{hw_key_read, NULL, {"hw", "key_read", 1, 0, NULL}}
	}
};
static k_signed_call_table usr_feats = { NULL, 0 };
int main(int argc, char const *argv[]) {
	llogs(WL_NOTICE, "ELF loader");

	const struct loader_ctx_t ctx = {
		{
			.log=loader_log,
			.wait=loader_wait,
			.take_pages=loader_alloc,
			.release_pages=loader_free,
			.srv_list=loader_srv_list,
			.srv_read=loader_srv_read
		},
		.hw_feats=&hw_feats,
		.usr_feats=&usr_feats
	};
	os_entry(&ctx);
	llogs(WL_INFO, "OS Stopped");

	hw_key_restore();
	return 0;
}
