#include "fat_fs.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "stdlib.h"
#include "sys/time8.h"
#include "errno.h"

// Partially based on
/*----------------------------------------------------------------------------/
/  FatFs - Generic FAT Filesystem module  R0.14b                              /
/-----------------------------------------------------------------------------/
/
/ Copyright (C) 2021, ChaN, all right reserved.
/
/ FatFs module is an open source software. Redistribution and use of FatFs in
/ source and binary forms, with or without modification, are permitted provided
/ that the following condition is met:

/ 1. Redistributions of source code must retain the above copyright notice,
/    this condition and the following disclaimer.
/
/ This software is provided by the copyright holder and contributors "AS IS"
/ and any warranties related to this software are DISCLAIMED.
/ The copyright owner or contributors be NOT LIABLE for any damages caused
/ by use of this software.
/
/----------------------------------------------------------------------------*/

#define FF_MIN_SS 512
#define FF_MAX_SS 512
#if FF_MAX_SS != FF_MIN_SS
#define SS(fs) (fs)->sector_size
#else
#define SS(fs) FF_MAX_SS
#endif

#define BS_JmpBoot 0      /* x86 jump instruction (3-byte) */
#define BS_OEMName 3      /* OEM name (8-byte) */
#define BPB_BytsPerSec 11 /* Sector size [byte] (WORD) */
#define BPB_SecPerClus 13 /* Cluster size [sector] (BYTE) */
#define BPB_RsvdSecCnt 14 /* Size of reserved area [sector] (WORD) */
#define BPB_NumFATs 16    /* Number of FATs (BYTE) */
#define BPB_RootEntCnt 17 /* Size of root directory area for FAT [entry] (WORD) */
#define BPB_TotSec16 19   /* Volume size (16-bit) [sector] (WORD) */
#define BPB_Media 21      /* Media descriptor byte (BYTE) */
#define BPB_FATSz16 22    /* FAT size (16-bit) [sector] (WORD) */
#define BPB_SecPerTrk 24  /* Number of sectors per track for int13h [sector] (WORD) */
#define BPB_NumHeads 26   /* Number of heads for int13h (WORD) */
#define BPB_HiddSec 28    /* Volume offset from top of the drive (DWORD) */
#define BPB_TotSec32 32   /* Volume size (32-bit) [sector] (DWORD) */

#define BS_DrvNum 36      /* Physical drive number for int13h (BYTE) */
#define BS_NTres 37       /* WindowsNT error flag (BYTE) */
#define BS_BootSig 38     /* Extended boot signature (BYTE) */
#define BS_VolID 39       /* Volume serial number (DWORD) */
#define BS_VolLab 43      /* Volume label string (8-byte) */
#define BS_FilSysType 54  /* Filesystem type string (8-byte) */
#define BS_BootCode 62    /* Boot code (448-byte) */
#define BS_FilSysType32 82 /* FAT32: Filesystem type string (8-byte) */
#define BS_55AA 510       /* Signature word (u16) */

#define BPB_FATSz32 36     /* FAT32: FAT size [sector] (DWORD) */
#define BPB_ExtFlags32 40  /* FAT32: Extended flags (WORD) */
#define BPB_FSVer32 42     /* FAT32: Filesystem version (WORD) */
#define BPB_RootClus32 44  /* FAT32: Root directory cluster (DWORD) */
#define BPB_FSInfo32 48    /* FAT32: Offset of FSINFO sector (WORD) */
#define BPB_BkBootSec32 50 /* FAT32: Offset of backup boot sector (WORD) */
#define BS_DrvNum32 64     /* FAT32: Physical drive number for int13h (BYTE) */
#define BS_NTres32 65      /* FAT32: Error flag (BYTE) */
#define BS_BootSig32 66    /* FAT32: Extended boot signature (BYTE) */
#define BS_VolID32 67      /* FAT32: Volume serial number (DWORD) */
#define BS_VolLab32 71     /* FAT32: Volume label string (8-byte) */
#define BS_FilSysType32 82 /* FAT32: Filesystem type string (8-byte) */
#define BS_BootCode32 90   /* FAT32: Boot code (420-byte) */

#define FSI_LeadSig 0      /* FAT32 FSI: Leading signature (DWORD) */
#define FSI_StrucSig 484   /* FAT32 FSI: Structure signature (DWORD) */
#define FSI_Free_Count 488 /* FAT32 FSI: Number of free clusters (DWORD) */
#define FSI_Nxt_Free 492   /* FAT32 FSI: Last allocated cluster (DWORD) */

#define u8v(p) *(uint8_t*)(p)
#define u16v(p) *(uint16_t*)(p)
#define u32v(p) *(uint32_t*)(p)

/* Limits and boundaries */
#define MAX_DIR 0x200000      /* Max size of FAT directory */
#define MAX_DIR_EX 0x10000000 /* Max size of exFAT directory */
#define MAX_FAT12 0xFF5       /* Max FAT12 clusters (differs from specs, but right for real DOS/Windows behavior) */
#define MAX_FAT16 0xFFF5      /* Max FAT16 clusters (differs from specs, but right for real DOS/Windows behavior) */
#define MAX_FAT32 0x0FFFFFF5  /* Max FAT32 clusters (not specified, practical limit) */
#define MAX_EXFAT 0x7FFFFFFD  /* Max exFAT clusters (differs from specs, implementation limit) */

#define DIR_Name 0		  /* Short file name (11-byte) */
#define DIR_Attr 11		  /* Attribute (BYTE) */
#define DIR_NTres 12	  /* Lower case flag (BYTE) */
#define DIR_CrtTime10 13  /* Created time sub-second (BYTE) */
#define DIR_CrtTime 14	  /* Created time (DWORD) */
#define DIR_LstAccDate 18 /* Last accessed date (WORD) */
#define DIR_FstClusHI 20  /* Higher 16-bit of first cluster (WORD) */
#define DIR_ModTime 22	  /* Modified time (DWORD) */
#define DIR_FstClusLO 26  /* Lower 16-bit of first cluster (WORD) */
#define DIR_FileSize 28	  /* File size (DWORD) */
#define LDIR_Ord 0		  /* LFN: LFN order and LLE flag (BYTE) */
#define LDIR_Attr 11	  /* LFN: LFN attribute (BYTE) */
#define LDIR_Type 12	  /* LFN: Entry type (BYTE) */
#define LDIR_Chksum 13	  /* LFN: Checksum of the SFN (BYTE) */
#define LDIR_FstClusLO 26 /* LFN: MBZ field (WORD) */
static const uint8_t LFN_CHARS[] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};
#define LFN_SZCHAR (sizeof(LFN_CHARS)/sizeof(LFN_CHARS[0]))

/* File attribute bits for directory entry (FILINFO.fattrib) */
#define AM_RDO 0x01 /* Read only */
#define AM_HID 0x02 /* Hidden */
#define AM_SYS 0x04 /* System */
#define AM_VID 0x08 /* Volume id */
#define AM_DIR 0x10 /* Directory */
#define AM_ARC 0x20 /* Archive */
#define AM_LFN 0x0F /* Long File Name */

#define SZDIRE 32 /* Size of a directory entry */
#define DDEM 0xE5  /* Deleted directory entry mark set to DIR_Name[0] */
#define RDDEM 0x05 /* Replacement of the character collides with DDEM */
#define LLEF 0x40  /* Last long entry flag in LDIR_Ord */
typedef uint32_t lba_t;

/** Time packed as FAT. Multiply seconds by 2. */
struct fat_time_t {
	uint8_t second : 5;
	// offset of packed bit-field ‘minutes’ has changed in GCC 4.4
	uint16_t minute : 6;
	uint8_t hour : 5;
} __attribute__((packed));
#define FAT_YEAR_OFFSET 1980
/** Date packed as FAT */
struct fat_date_t {
	uint8_t day : 5;
	uint16_t month : 4;
	uint8_t year : 7;
} __attribute__((packed));
/** Date and time packed as FAT */
struct fat_datetime_t {
	struct fat_time_t time;
	struct fat_date_t date;
} __attribute__((packed));
static inline time_t mktime_fat(const struct fat_datetime_t *dt) {
	uint16_t year = (uint16_t)dt->date.year + FAT_YEAR_OFFSET;
	struct tm8_t t = {
		.second = dt->time.second * 2,
		.minute = dt->time.minute,
		.hour = dt->time.hour,
		.day = dt->date.day,
		.month = dt->date.month,
		.year = year % 100,
		.century = year / 100
	};
	return mktime8(&t);
}
/*static inline struct fat_datetime_t * gmtime_fat(time_t time, struct fat_datetime_t *dt) {
	struct tm8_t t;
	if (!gmtime8(time, &t))
		return NULL;

	dt->time.second = t.second / 2;
	dt->time.minute = t.minute;
	dt->time.hour = t.hour;
	dt->date.day = t.day;
	dt->date.month = t.month;
	dt->date.year = (uint16_t)t.century * 100 + t.year - FAT_YEAR_OFFSET;
	return dt;
}*/

enum fat_flag {
	FF_NONE = 0,
	FF_DIRTY_WRITE = 1 << 0,
	FF_NO_FSI = 1 << 4,
	FF_DIRTY_FSI = 1 << 5
};

struct fat_fs {
	fs self;
	disk* disk;
	enum fat_type type; /* FAT version */
	uint8_t n_fats; /* Number of FATs (1 or 2) */
	enum fat_flag flags;
	uint16_t n_rootdir; /* Number of root directory entries (FAT12/16) */
	uint8_t cluster_size; /* Cluster size [sectors] */
#if FF_MAX_SS != FF_MIN_SS
	uint16_t sector_size; /* Sector size [bytes] */
#endif
// MAYBE: char* lfnbuf;
// FIXME: lock
	uint32_t last_clst; /* Last allocated cluster */
	uint32_t free_clst; /* Number of free clusters */
	uint32_t n_fatent; /* Number of FAT entries (number of clusters + 2) */
	uint32_t fat_size; /* Size of an FAT [sectors] */
	lba_t fatbase; /* FAT base sector */
	lba_t dirbase; /* Root directory base sector/cluster */
	lba_t database; /* Data base sector */
#if EXFAT
	lba_t bitbase; /* Allocation bitmap base sector */
#endif
	lba_t bufbase; /* Sector of buf */
	uint8_t buf[FF_MAX_SS]; /* Sector buffer */
};

static int fat_root(struct fs *self, struct inode_stat *i);
#define CHECK_FS(fs) { if (fs->self.root != fat_root || !fs->disk || fs->type <= FAT_ANY || fs->type > FAT_ALL) return -EINVAL; }

static inline bool disk_power(disk* d) {
	enum disk_status status = {0};
	disk_cmd(d, DISK_GET_STATUS, &status);
	if (status & DISK_OFF) {
		bool on = true;
		disk_cmd(d, DISK_POWER, &on);
	}
	disk_cmd(d, DISK_GET_STATUS, &status);
	return (status & DISK_OFF) == 0;
}

static inline bool ff_lock(struct fat_fs *fs) {
	(void)fs;
	return true;
}
static inline void ff_unlock(struct fat_fs *fs) {
	(void)fs;
}
#define CHECK_LOCK(fs) {}

static int inline ff_sync(struct fat_fs* fs) {
	CHECK_FS(fs);
	CHECK_LOCK(fs);
	if (!disk_power(fs->disk)) return -EIO;
	if (fs->flags & FF_DIRTY_WRITE) {
		int ret = disk_write(fs->disk, fs->buf, 1, fs->bufbase);
		if (ret < 0) return ret;
		fs->flags &= ~FF_DIRTY_WRITE;
	}
	//FIXME: if (fs->flags & FF_DIRTY_FSI)
	//MAYBE: flush internals
	return 1;
}
static inline int ff_view(struct fat_fs *fs, lba_t bufbase) {
	if (fs->bufbase == bufbase) return 1;
	int ret = ff_sync(fs);
	if (ret < 0) return ret;
	fs->bufbase = bufbase;
	return disk_read(fs->disk, fs->buf, 1, bufbase);
}

#define IS_VALID_CLST(fs, clst) (clst >= 2 && clst < fs->n_fatent && clst != 0xFFFFFFFF)
static uint32_t get_fat(struct fat_fs *fs, uint32_t clst) {
	if (!IS_VALID_CLST(fs, clst)) {	/* Check if in valid range */
		return 1;	/* Internal error */
	} else {
		uint32_t ret = 0xFFFFFFFF;	/* Default retue falls on disk error */

		switch (fs->type) {
		case FAT_12: {
			uint32_t bc = clst; bc += bc / 2;
			if (ff_view(fs, fs->fatbase + (bc / SS(fs))) < 0) break;
			uint32_t wc = fs->buf[bc++ % SS(fs)];		/* Get 1st byte of the entry */
			if (ff_view(fs, fs->fatbase + (bc / SS(fs))) < 0) break;
			wc |= fs->buf[bc % SS(fs)] << 8;	/* Merge 2nd byte of the entry */
			ret = (clst & 1) ? (wc >> 4) : (wc & 0xFFF);	/* Adjust bit position */
			break;
		}

		case FAT_16:
			if (ff_view(fs, fs->fatbase + (clst / (SS(fs) / 2))) < 0) break;
			ret = u16v(fs->buf + clst * 2 % SS(fs));		/* Simple WORD array */
			break;

		case FAT_32:
			if (ff_view(fs, fs->fatbase + (clst / (SS(fs) / 4))) < 0) break;
			ret = u32v(fs->buf + clst * 4 % SS(fs)) & 0x0FFFFFFF;	/* Simple DWORD array but mask out upper 4 bits */
			break;
#if FF_FS_EXFAT
		case FAT_EX:
			if ((obj->objsize != 0 && obj->sclust != 0) || obj->stat == 0) {	/* Object except root dir must have retid data length */
				DWORD cofs = clst - obj->sclust;	/* Offset from start cluster */
				DWORD clen = (DWORD)((LBA_t)((obj->objsize - 1) / SS(fs)) / fs->csize);	/* Number of clusters - 1 */

				if (obj->stat == 2 && cofs <= clen) {	/* Is it a contiguous chain? */
					ret = (cofs == clen) ? 0x7FFFFFFF : clst + 1;	/* No data on the FAT, generate the retue */
					break;
				}
				if (obj->stat == 3 && cofs < obj->n_cont) {	/* Is it in the 1st fragment? */
					ret = clst + 1; 	/* Generate the retue */
					break;
				}
				if (obj->stat != 2) {	/* Get retue from FAT if FAT chain is retid */
					if (obj->n_frag != 0) {	/* Is it on the growing edge? */
						ret = 0x7FFFFFFF;	/* Generate EOC */
					} else {
						if (move_window(fs, fs->fatbase + (clst / (SS(fs) / 4))) != FR_OK) break;
						ret = ld_dword(fs->win + clst * 4 % SS(fs)) & 0x7FFFFFFF;
					}
					break;
				}
			}
			ret = 1;	/* Internal error */
			break;
#endif
		default:
			ret = 1;	/* Internal error */
		}
		return ret;
	}
}

static inline lba_t clst2sect(struct fat_fs *fs, uint32_t clst) {
	if (!IS_VALID_CLST(fs, clst)) return 0;
	return fs->database + (lba_t)fs->cluster_size * (clst - 2);
}
/** Get sector and entry index (ofs) within it in SZDIRE.
 * idx is entry index from clst in SZDIRE */
static inline lba_t entry_idx(struct fat_fs *fs, uint32_t clst, uint32_t idx, uint32_t* ofs) {
	if (clst == 0 && fs->type >= FAT_32) {
		clst = fs->dirbase;
	}
	lba_t sect = 0xFFFFFFFF;
	if (clst == 0) {
		if (idx >= fs->n_rootdir) return 0xFFFFFFFF;
		sect = fs->dirbase;
	} else {
		if (idx) {
			const uint32_t ent_p_clst = (uint32_t)fs->cluster_size * SS(fs) / SZDIRE;
			while (idx >= ent_p_clst) { /* Follow cluster chain */
				clst = get_fat(fs, clst);		/* Get next cluster */
				if (!IS_VALID_CLST(fs, clst))
					return 0xFFFFFFFF;	/* Disk error or end of chain */
				idx -= ent_p_clst;
			}
		}
		sect = clst2sect(fs, clst);
	}
	if (ofs) *ofs = idx % (SS(fs) / SZDIRE);
	return sect + idx / (SS(fs) / SZDIRE);
}
/** Get next entry */
static inline lba_t entry_next(struct fat_fs* fs, uint32_t head_clst, lba_t cur_sect, uint32_t* idx) {
	if (head_clst == 0 && fs->type < FAT_32) {
		if (*idx + (cur_sect - fs->dirbase) / (SS(fs) / SZDIRE) + 1 >= fs->n_rootdir)
			return 0xFFFFFFFF;

		if (*idx + 1 < SS(fs) / SZDIRE) {
			*idx += 1; /* Same sector */
			return cur_sect;
		} else {
			*idx = 0;
			return cur_sect + 1;
		}
	} else {
		if (*idx + 1 < SS(fs) / SZDIRE) {
			*idx += 1; /* Same sector */
			return cur_sect;
		}

		uint32_t cur_clst = (cur_sect + fs->database) / (lba_t)fs->cluster_size + 2;
		*idx = 0;
		if (cur_clst == (cur_sect + 1 + fs->database) / (lba_t)fs->cluster_size + 2)
			return cur_sect + 1; /* Same cluster */

		/* Follow cluster chain */
		cur_clst = get_fat(fs, cur_clst); /* Get next cluster */
		if (!IS_VALID_CLST(fs, cur_clst))
			return 0xFFFFFFFF;	/* Disk error or end of chain */

		return clst2sect(fs, cur_clst);
	}
}
static inline uint32_t entry_dataclst(struct fat_fs* fs, void* ent) {
	return (fs->type >= FAT_32 ? (uint32_t)u16v(ent + DIR_FstClusHI) << 16 : 0) + u16v(ent + DIR_FstClusLO);
}

static inline uint8_t lfn_read(const void* ent, char* s, size_t len) {
	if (!ent || u8v(ent + LDIR_Attr) != AM_LFN) return -1;
	const uint8_t ofs = ((u8v(ent) & ~LLEF)-1) * LFN_SZCHAR;
	for (size_t i = 0; i < LFN_SZCHAR && i < len; i++) {
		//TODO: wchar to utf8 (will need working buffer and later decode)
		*(s + ofs + i) = *(const char*)(ent + LFN_CHARS[i]);
	}
	return u8v(ent + LDIR_Chksum);
}
static inline void entry_read(struct inode_stat* i, const void* ent, bool name) {
	if (name /* || bad_sum */) {
		int len = 8;
		while (len > 0 && u8v(ent + DIR_Name + len - 1) == ' ')
			len--;
		memcpy(i->name, ent + DIR_Name, len);
		if (u8v(ent + DIR_Name + 8) != ' ') {
			i->name[8] = '.';
			len = 3;
			while (len > 0 && u8v(ent + DIR_Name + 8 + len - 1) == ' ')
				len--;
			memcpy(i->name + 9, ent + DIR_Name + 8, len);
		}
	}

	if(u8v(ent + DIR_Attr) & AM_DIR) i->attr |= FILE_DIRECTORY;
	if(u8v(ent + DIR_Attr) & AM_HID) i->attr |= FILE_HIDDEN;
	if(u8v(ent + DIR_Attr) & AM_RDO) i->attr |= FILE_READ_ONLY;
	i->ctime.tv_sec = mktime_fat(ent + DIR_CrtTime);
	i->ctime.tv_nsec = u8v(ent + DIR_CrtTime10);
	if (i->ctime.tv_nsec > 100) {
		i->ctime.tv_sec += 1;
		i->ctime.tv_nsec %= 100;
	}
	i->ctime.tv_nsec *= 1000000000/100;
	i->mtime.tv_sec = mktime_fat(ent + DIR_ModTime);
	{
		struct fat_datetime_t dt = {0};
		memcpy(&dt.date, ent + DIR_LstAccDate, sizeof(uint32_t));
		i->atime = mktime_fat(&dt);
	}
	i->size = u32v(ent + DIR_FileSize);
}

/** Root packed inode id */
#define INODE_ROOT UINT64_MAX
/** Pointer to first lfn or real entry with lfn size */
#define INODE_PACK(clst, idx, lfn) ((uint64_t)(clst) << 32 | ((uint64_t)(lfn) & 0xFFFF) << 16 | (uint64_t)(idx) & 0xFFFF)
#define INODE_CLST(id) (uint32_t)((id) >> 32)
#define INODE_LFN(id) (uint16_t)(((id) >> 16) & 0xFFFF)
#define INODE_IDX(id) (uint16_t)((id) & 0xFFFF)

#define IS_VALID_IFLAG(flag, ent) true /* MAYBE: (flag == mktime_fat(ent + DIR_CrtTime)) */

#define CHECK_INODE(i, ent) { if (i.id == INODE_ROOT || !(ent) || !IS_VALID_IFLAG((i).flag, ent)) return -EINVAL; }
#define CHECK_IDIR(d, ent) { CHECK_INODE((d).i, ent); if ((u8v((ent) + DIR_Attr) & AM_DIR) == 0) return -EINVAL; }
#define CHECK_IFILE(f, ent) { CHECK_INODE((f).i, ent); if ((u8v((ent) + DIR_Attr) & AM_DIR) != 0) return -EINVAL; }

/** Get pointer to entry or NULL */
static inline void* ff_stat(inode i, char* lfn, size_t lfn_len) {
	struct fat_fs* fs = (void*)i.fs;
	CHECK_LOCK(fs);
	if (i.id == INODE_ROOT) return NULL;
	if (lfn && lfn_len) { /** Read lfn */
		memset(lfn, 0, lfn_len);
		uint32_t idx;
		lba_t sect = entry_idx(fs, INODE_CLST(i.id), INODE_IDX(i.id) - INODE_LFN(i.id), &idx);
		if (ff_view(fs, sect) < 0) return NULL;

		int lfn_ent = INODE_LFN(i.id);
		while (1) {
			//TODO: read fs->buf + idx*SZDIRE
			// if lfn copy to *lfn

			if (lfn_len <= 0) break;

			sect = entry_next(fs, INODE_CLST(i.id), sect, &idx);
			if (ff_view(fs, sect) < 0) return NULL;

			lfn_ent--;
			if (lfn_ent <= 0) /** Already on real entry */
				return fs->buf + idx * SZDIRE;
		}
	}
	uint32_t idx;
	lba_t sect = entry_idx(fs, INODE_CLST(i.id), INODE_IDX(i.id), &idx);
	if (ff_view(fs, sect) < 0) return NULL;
	return fs->buf + idx * SZDIRE;
}

static int fat_root(struct fs *self, struct inode_stat *i) {
	if (!i) return -EINVAL;
	struct fat_fs *fs = (void *)self;
	CHECK_FS(fs);
	memset(i, 0, sizeof(*i));
	i->i.fs = self;
	i->i.id = INODE_ROOT;
	i->attr = FILE_DIRECTORY | FILE_HIDDEN | FILE_READ_ONLY;
	i->size = fs->n_rootdir * SZDIRE;
	ff_lock(fs);
	if (ff_view(fs, 0) >= 0)
		memcpy(i->name, fs->buf + BS_VolLab, 8);
	ff_unlock(fs);
	return i->name[0] != 0;
}
static int fat_get_name(inode i, char *name, size_t len, size_t offset) {
	struct fat_fs *fs = (void *)i.fs;
	CHECK_FS(fs);
	//Check_inode
	//FIXME: impl
	return -ENOPROTOOPT;
}
static int fat_read(ifile f, struct iovec *iovec, size_t iovcnt, size_t offset, size_t *reed) {
	struct fat_fs *fs = (void *)f.i.fs;
	CHECK_FS(fs);
	//check_inode
	//FIXME: impl
	return -ENOPROTOOPT;
}
static int fat_write(ifile f, const struct iovec *iovec, size_t iovcnt, size_t offset, size_t *written) {
	struct fat_fs *fs = (void *)f.i.fs;
	CHECK_FS(fs);
	//check_inode
	//FIXME: impl
	return -ENOPROTOOPT;
}
static int fat_resize(ifile f, size_t size) {
	struct fat_fs *fs = (void *)f.i.fs;
	CHECK_FS(fs);
	//FIXME: impl
	return -ENOPROTOOPT;
}
static int fat_sync(ifile f) {
	struct fat_fs *fs = (void *)f.i.fs;
	CHECK_FS(fs);
	ff_lock(fs);
	//MAYBE: per file buffer
	int ret = ff_sync(fs);
	ff_unlock(fs);
	return ret;
}
static int fat_stat(ipath path, struct inode_stat *i) {
	if (!path.path && path.root.i.id == INODE_ROOT)
		return fat_root(path.root.i.fs, i);

	struct fat_fs *fs = (void *)path.root.i.fs;
	CHECK_FS(fs);
	//FIXME: impl
	//Check_node
	return -ENOPROTOOPT;
}
static int fat_list(idir d, struct inode_stat *is, size_t icnt, size_t offset) {
	struct fat_fs *fs = (void *)d.i.fs;
	CHECK_FS(fs);
	ff_lock(fs);
	uint32_t head_clst = 0;
	if (d.i.id != INODE_ROOT) {
		void* ent = ff_stat(d.i, NULL, 0);
		CHECK_IDIR(d, ent);
		//MAYBE: update adate
		head_clst = entry_dataclst(fs, ent);
	}
	memset(is, 0, sizeof(*is) * icnt);
	const uint32_t ENT_P_SECT = SS(fs) / SZDIRE;
	uint32_t n_lfn = 0;
	size_t n_read = 0;
	bool done = false;
	size_t i_clst = 0;
	uint32_t clst = head_clst;
	do {
		for (size_t i_sect = 0; i_sect < fs->cluster_size && !done; i_sect++) {
			if (ff_view(fs, entry_idx(fs, clst, i_sect * ENT_P_SECT, NULL)) < 0) {
				ff_unlock(fs);
				return -EIO;
			}
			for (size_t i_ent = 0; i_ent < ENT_P_SECT && !done; i_ent++) {
				void *ent = fs->buf + i_ent * SZDIRE;

				if (!u8v(ent)) {
					done = true;
					break; /* No more files/directories */
				}
				if (u8v(ent) == DDEM) continue; /* Deleted entry */

				struct inode_stat* i = is + n_read - offset;
				if (u8v(ent + LDIR_Attr) == AM_LFN) {
					if (n_read >= offset)
						lfn_read(ent, i->name, FILE_SHORTNAME_SIZE);

					n_lfn++;
					continue; /* Long file name entry */
				}

				if (u8v(ent + DIR_Attr) & AM_VID) continue; /* Volume id entry */
				if (memcmp(ent, ".          ", 11) == 0 || memcmp(ent, "..         ", 11) == 0) continue; /* Backlink entry */

				if (n_read >= offset) {
					i->i.fs = (void *)fs;
					i->i.id = INODE_PACK(head_clst, (i_clst * fs->cluster_size + i_sect) * ENT_P_SECT + i_ent, n_lfn);
					entry_read(i, ent, !n_lfn);
					i->i.flag = i->ctime.tv_sec;
				}

				n_lfn = 0;
				n_read++;
				if (n_read >= offset + icnt)
					done = true;
			}
		}
		clst = get_fat(fs, clst);
		i_clst++;
		if (!IS_VALID_CLST(fs, clst))
			done = true;
	} while (!done);
	ff_unlock(fs);
	return n_read > offset ? n_read - offset : -ERANGE;
}
static int fat_mkdir(ipath path) {
	struct fat_fs *fs = (void *)path.root.i.fs;
	CHECK_FS(fs);
	//FIXME: impl
	return -ENOPROTOOPT;
}
static int fat_remove(ipath path) {
	struct fat_fs *fs = (void *)path.root.i.fs;
	CHECK_FS(fs);
	//FIXME: impl
	return -ENOPROTOOPT;
}
static int fat_rename(ipath from, ipath to) {
	if (from.root.i.fs != to.root.i.fs) return -EINVAL;
	struct fat_fs *fs = (void *)from.root.i.fs;
	CHECK_FS(fs);
	//FIXME: impl
	return -ENOPROTOOPT;
}
static int fat_touch(ipath path) {
	struct fat_fs *fs = (void *)path.root.i.fs;
	CHECK_FS(fs);
	//FIXME: impl
	return -ENOPROTOOPT;
}
static int fat_get_free(ipath path, size_t *space) {
	struct fat_fs *fs = (void *)path.root.i.fs;
	CHECK_FS(fs);
	if (fs->free_clst <= fs->n_fatent - 2) {
		*space = (size_t)fs->free_clst * fs->cluster_size * SS(fs);
		return 1;
	} else {
		/* Scan FAT to obtain number of free clusters */
		ff_lock(fs);
		int res = 0;
		uint32_t nfree = 0;
		if (fs->type == FAT_12) {	/* FAT12: Scan bit field FAT entries */
			for (uint32_t clst = 2; clst < fs->n_fatent; clst++) {
				uint32_t stat = get_fat(fs, clst);
				if (stat == 0xFFFFFFFF || stat == 1) {
					res = -1;
					break;
				}
				if (stat == 0)
					nfree++;
			}
		} else {
#if FF_FS_EXFAT
			if (fs->type == FAT_EX) {	/* exFAT: Scan allocation bitmap */
				BYTE bm;
				UINT b;

				uint32_t clst = fs->n_fatent - 2; /* Number of clusters */
				sect = fs->bitbase;			/* Bitmap sector */
				i = 0;						/* Offset in the sector */
				do {	/* Counts numbuer of bits with zero in the bitmap */
					if (i == 0) {
						res = move_window(fs, sect++);
						if (res != FR_OK) break;
					}
					for (b = 8, bm = fs->win[i]; b && clst; b--, clst--) {
						if (!(bm & 1)) nfree++;
						bm >>= 1;
					}
					i = (i + 1) % SS(fs);
				} while (clst);
			} else
#endif
			{	/* FAT16/32: Scan WORD/DWORD FAT entries */
				lba_t sect = fs->fatbase;		/* Top of the FAT */
				uint32_t i = 0;					/* Offset in the sector */
				for (uint32_t clst = fs->n_fatent; clst; clst--) {
					if (i == 0) {
						res = ff_view(fs, sect++);
						if (res < 0) break;
					}
					if (fs->type == FAT_16) {
						if (u16v(fs->buf + i) == 0) nfree++;
						i += 2;
					} else {
						if ((u32v(fs->buf + i) & 0x0FFFFFFF) == 0) nfree++;
						i += 4;
					}
					i %= SS(fs);
				}
			}
		}
		if (res >= 0) {		/* Update parameters if succeeded */
			*space = nfree * fs->cluster_size * SS(fs);			/* Return the free clusters */
			fs->free_clst = nfree;	/* Now free_clst is valid */
			fs->flags |= FF_DIRTY_FSI;		/* FAT32: FSInfo is to be updated */
		}
		ff_unlock(fs);
		return res;
	}
}
static int fat_umount(struct fs *self) {
	struct fat_fs *fs = (void *)self;
	CHECK_FS(fs);
	ff_lock(fs);
	int ret = ff_sync(fs);
	if (ret < 0) {
		ff_unlock(fs);
		return ret;
	}

	fs->type = FAT_ANY;
	fs->disk = NULL;
	//FIXME: free all
	ff_unlock(fs);
	return 1;
}

fs *fat_mount(disk *d) {
	if (!disk_power(d)) return NULL;

	size_t ssize = 0;
	disk_cmd(d, DISK_GET_SECTOR_SIZE, &ssize);
#if FF_MAX_SS != FF_MIN_SS
	if (ssize < FF_MIN_SS || ssize > FF_MAX_SS || (ssize & (ssize-1)) != 0) return NULL;
#else
	if (ssize != FF_MAX_SS) return NULL;
#endif

	struct fat_fs* fs = malloc(sizeof(struct fat_fs));
	if (!fs) return NULL;

	fs->disk = d;
#if FF_MAX_SS != FF_MIN_SS
	fs->sector_size = ssize;
#endif
	fs->type = FAT_ALL;
	fs->flags = FF_NONE;
	//TODO: setup lock

	fs->self.root = fat_root; // NOTE: used during CHECK_FS

	if (!ff_lock(fs)) goto err;
	fs->bufbase = -1;
	if (ff_view(fs, 0) < 0) goto err;

	uint8_t boot_jmp = fs->buf[BS_JmpBoot];
	if (boot_jmp != 0xEB && boot_jmp != 0xE9 && boot_jmp != 0xE8) goto err;

	uint16_t boot_sign = u16v(fs->buf + BS_55AA);
#if EXFAT
	if (boot_sign == 0xAA55 && !memcmp(fs->buf + BS_JmpBoot, "\xEB\x76\x90" "EXFAT   ", 11)) {
		fs->type = FAT_EX;
	} else
#endif
	if (boot_sign == 0xAA55 && !memcmp(fs->buf + BS_FilSysType32, "FAT32   ", 8)) {
		fs->type = FAT_32;
	} else {
		/* FAT volumes formatted with early MS-DOS lack BS_55AA and BS_FilSysType, so FAT VBR needs to be identified without them. */
		uint16_t bytes_per_sect = u16v(fs->buf + BPB_BytsPerSec);
		uint8_t sect_per_clus = fs->buf[BPB_SecPerClus];
		if ((bytes_per_sect & (bytes_per_sect - 1)) == 0 && bytes_per_sect >= FF_MIN_SS && bytes_per_sect <= FF_MAX_SS	/* Properness of sector size (512-4096 and 2^n) */
			&& sect_per_clus != 0 && (sect_per_clus & (sect_per_clus - 1)) == 0				/* Properness of cluster size (2^n) */
			&& u16v(fs->buf + BPB_RsvdSecCnt) != 0	/* Properness of reserved sectors (MNBZ) */
			&& fs->buf[BPB_NumFATs] - 1 <= 1		/* Properness of FATs (1 or 2) */
			&& u16v(fs->buf + BPB_RootEntCnt) != 0	/* Properness of root dir entries (MNBZ) */
			&& (u16v(fs->buf + BPB_TotSec16) >= 128 || u32v(fs->buf + BPB_TotSec32) >= 0x10000)	/* Properness of volume sectors (>=128) */
			&& u16v(fs->buf + BPB_FATSz16) != 0) {	/* Properness of FAT size (MNBZ) */
				fs->type = FAT_12; //NOTE: or FAT_16
		} else goto err;
	}

#if EXFAT
	if (fs->type == FAT_EX) {
		to_implement_or_wipe
	} else
#endif
	{ // It's a FAT partition.
		if (u16v(fs->buf + BPB_BytsPerSec) != SS(fs)) goto err;

		fs->fat_size = u16v(fs->buf + BPB_FATSz16);		/* Number of sectors per FAT */
		if (fs->fat_size == 0) fs->fat_size = u32v(fs->buf + BPB_FATSz32);

		fs->n_fats = fs->buf[BPB_NumFATs];          /* Number of FATs */
		if (fs->n_fats != 1 && fs->n_fats != 2) goto err;	/* (Must be 1 or 2) */
		uint32_t fasize = fs->fat_size * fs->n_fats;        /* Number of sectors for FAT area */

		fs->cluster_size = fs->buf[BPB_SecPerClus];                /* Cluster size */
		if (fs->cluster_size == 0 || (fs->cluster_size & (fs->cluster_size - 1))) goto err;	/* (Must be power of 2) */

		fs->n_rootdir = u16v(fs->buf + BPB_RootEntCnt);	/* Number of root directory entries */
		if (fs->n_rootdir % (SS(fs) / SZDIRE)) goto err;	/* (Must be sector aligned) */

		uint32_t tsect = u16v(fs->buf + BPB_TotSec16);		/* Number of sectors on the volume */
		if (tsect == 0) tsect = u32v(fs->buf + BPB_TotSec32);

		uint16_t nrsv = u16v(fs->buf + BPB_RsvdSecCnt);		/* Number of reserved sectors */
		if (nrsv == 0) goto err;			/* (Must not be 0) */

		/* Determine the FAT sub type */
		uint32_t sysect = fasize + nrsv + fs->n_rootdir / (SS(fs) / SZDIRE); /* RSV + FAT + DIR */
		if (tsect < sysect) goto err;	/* (Invalid volume size) */
		uint32_t nclst = (tsect - sysect) / fs->cluster_size;			/* Number of clusters */
		if (nclst == 0 || nclst > MAX_FAT32) goto err;		/* (Invalid volume size) */
		fs->type = FAT_32;
		if (nclst <= MAX_FAT16) fs->type = FAT_16;
		if (nclst <= MAX_FAT12) fs->type = FAT_12;

		fs->n_fatent = nclst + 2;
		fs->fatbase = nrsv;
		fs->database = sysect;
		uint32_t szbfat;
		if (fs->type == FAT_32) {
			if (u16v(fs->buf + BPB_FSVer32) != 0) goto err;	/* (Must be FAT32 revision 0.0) */
			if (fs->n_rootdir != 0) goto err;	/* (BPB_RootEntCnt must be 0) */
			fs->dirbase = u16v(fs->buf + BPB_RootClus32); /* Root directory start cluster */
			szbfat = fs->n_fatent * 4; /* (Needed FAT size) */
		} else {
			if (fs->n_rootdir == 0)	goto err;	/* (BPB_RootEntCnt must not be 0) */
			fs->dirbase = fs->fatbase + fasize; /* Root directory start sector */
			szbfat = (fs->type == FAT_16) ?				/* (Needed FAT size) */
				fs->n_fatent * 2 : fs->n_fatent * 3 / 2 + (fs->n_fatent & 1);
		}
		if (fs->fat_size < (szbfat + (SS(fs) - 1)) / SS(fs)) goto err;	/* (BPB_FATSz must not be less than the size needed) */

		/* Get FSInfo if available */
		fs->last_clst = fs->free_clst = UINT32_MAX;		/* Initialize cluster allocation information */
		/* Allow to update FSInfo only if BPB_FSInfo32 == 1 */
		if (fs->type == FAT_32 && u32v(fs->buf + BPB_FSInfo32) == 1) {
			if (ff_view(fs, 1) >= 0 /* Load FSInfo data if available */
				&& u16v(fs->buf + BS_55AA) == 0xAA55
				&& u32v(fs->buf + FSI_LeadSig) == 0x41615252
				&& u32v(fs->buf + FSI_StrucSig) == 0x61417272)
			{
				fs->free_clst = u32v(fs->buf + FSI_Free_Count);
				fs->last_clst = u32v(fs->buf + FSI_Nxt_Free);
			}
		} else {
			fs->flags |= FF_NO_FSI;
		}
	}

	fs->self.get_name = fat_get_name;
	fs->self.read = fat_read;
	fs->self.write = fat_write;
	fs->self.resize = fat_resize;
	fs->self.sync = fat_sync;
	fs->self.stat = fat_stat;
	fs->self.list = fat_list;
	fs->self.mkdir = fat_mkdir;
	fs->self.remove = fat_remove;
	fs->self.rename = fat_rename;
	fs->self.touch = fat_touch;
	fs->self.get_free = fat_get_free;
	fs->self.umount = fat_umount;

	ff_unlock(fs);
	return &fs->self;

err:
	ff_unlock(fs);
	free(fs);
	return NULL;
}
int fat_mkfs(disk *d) {
	return -ENOPROTOOPT;
}
