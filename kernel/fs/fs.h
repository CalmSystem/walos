#ifndef __FS_H
#define __FS_H

#include "stddef.h"
#include "sys/types.h"
#include "time.h"

enum file_attr {
	FILE_DIRECTORY = 1 << 0,
	FILE_HIDDEN = 1 << 1,
	FILE_READ_ONLY = 1 << 4
};
enum file_open_mode {
	FILE_OPEN_CREATE = 1 << 0,
	/** Error if exists == create */
	FILE_OPEN_STRICT = 1 << 1,
	FILE_OPEN_DIR_ONLY = 1 << 2,
	FILE_OPEN_FILE_ONLY = 1 << 3
};

struct fs;
#define FILE_SHORTNAME_SIZE 22

/** fs node identifier */
typedef struct inode {
	/** Owner filesystem */
	struct fs *fs;
	/** Implementation specific address */
	uint64_t id;
	/** Implementation specific guard */
	uint64_t flag;
} inode;

 /** inode with informations */
struct inode_stat {
	inode i;
	/** First part of the name. Partial if not double zero terminated */
	char name[FILE_SHORTNAME_SIZE + 1];
	/** Flags */
	enum file_attr attr;
	/** In bytes */
	size_t size;
	/** Creation time */
	struct timespec ctime;
	/** Modification time */
	struct timespec mtime;
	/** Access time */
	time_t atime;
};

typedef struct ifile {
	inode i;
} ifile;
typedef struct idir {
	inode i;
} idir;

typedef struct ipath {
	idir root;
	/** Path relative to root. NULL is "." */
	const char* path;
} ipath;

typedef struct fs {
	/** Get fs root entry */
	int (*root)(struct fs* self, struct inode_stat *i);
	/** Get long name */
	int (*get_name)(inode i, char *name, size_t len, size_t offset);
	/** Read data from the file */
	int (*read)(ifile f, struct iovec *iovec, size_t iovcnt, size_t offset, size_t *reed);
	/** Write data to the file */
	int (*write)(ifile f, const struct iovec *iovec, size_t iovcnt, size_t offset, size_t *written);
	/** Resize the file */
	int (*resize)(ifile f, size_t size);
	/** Flush cached data of the writing file */
	int (*sync)(ifile f);
	/** Get inode from path */
	int (*stat)(ipath path, struct inode_stat *i);
	/** Read directory entries */
	int (*list)(idir d, struct inode_stat *is, size_t icnt, size_t offset);
	/** Create a sub directory */
	int (*mkdir)(ipath path);
	/** Delete an existing file or directory */
	int (*remove)(ipath path);
	/** Rename/Move a file or directory */
	int (*rename)(ipath from, ipath to);
	/** Change timestamp of a file/dir or create empty file */
	int (*touch)(ipath path);
	/** Get number of free bytes on the drive */
	int (*get_free)(ipath path, size_t* space);
	/** Remove itself */
	int (*umount)(struct fs* self);
	//MAYBE: open/close
} fs;

static inline int fs_root(struct fs *self, struct inode_stat *i) {
	return self->root(self, i);
}
static inline int fs_get_name(inode i, char *name, size_t len, size_t offset) {
	return i.fs->get_name(i, name, len, offset);
}
static inline int fs_read(ifile f, struct iovec *iovec, size_t iovcnt, size_t offset, size_t *reed) {
	return f.i.fs->read(f, iovec, iovcnt, offset, reed);
}
static inline int fs_write(ifile f, const struct iovec *iovec, size_t iovcnt, size_t offset, size_t *written) {
	return f.i.fs->write(f, iovec, iovcnt, offset, written);
}
static inline int fs_resize(ifile f, size_t size) {
	return f.i.fs->resize(f, size);
}
static inline int fs_sync(ifile f) {
	return f.i.fs->sync(f);
}
static inline int fs_stat(ipath path, struct inode_stat *i) {
	return path.root.i.fs->stat(path, i);
}
static inline int fs_list(idir d, struct inode_stat *is, size_t icnt, size_t offset) {
	return d.i.fs->list(d, is, icnt, offset);
}
static inline int fs_mkdir(ipath path) {
	return path.root.i.fs->mkdir(path);
}
static inline int fs_remove(ipath path) {
	return path.root.i.fs->remove(path);
}
static inline int fs_rename(ipath from, ipath to) {
	return from.root.i.fs->rename(from, to);
}
static inline int fs_touch(ipath path) {
	return path.root.i.fs->touch(path);
}
static inline int fs_get_free(ipath path, size_t *space) {
	return path.root.i.fs->get_free(path, space);
}
static inline int fs_umount(struct fs *self) {
	return self->umount(self);
}

/** Smart fs_get_name from inode_stat */
#define FS_GET_NAME(stat, name, len, offset) \
	(stat).name[FILE_SHORTNAME_SIZE-1] ? \
		fs_get_name((stat).i, name, len, offset) : \
	(offset < FILE_SHORTNAME_SIZE ? \
		strncpy(name, (stat).name[offset], len) : -1);

#endif
