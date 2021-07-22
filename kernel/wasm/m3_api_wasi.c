//
//  m3_api_wasi.c
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#define _POSIX_C_SOURCE 200809L

#include "m3_api_wasi.h"

#include "m3_env.h"
#include "m3_exception.h"

#if defined(d_m3HasWASI)

// Fixup wasi_core.h
#if defined (M3_COMPILER_MSVC)
#  define _Static_assert(...)
#  define __attribute__(...)
#  define _Noreturn
#endif

#include "extra/wasi_core.h"

#include "sys/services.h"
#include "stdio.h"
#include "errno.h"
#include "time.h"

typedef struct wasi_iovec_t
{
    __wasi_size_t buf;
    __wasi_size_t buf_len;
} wasi_iovec_t;

#define PREOPEN_CNT   5

typedef struct Preopen {
    int         fd;
    const char* path;
    const char* real_path;
} Preopen;

Preopen preopen[PREOPEN_CNT] = {
    {  0, "<stdin>" , "" },
    {  1, "<stdout>", "" },
    {  2, "<stderr>", "" },
    { -1, "/"       , "." },
    { -1, "./"      , "." },
};

/*static
__wasi_errno_t errno_to_wasi(int errnum) {
    switch (errnum) {
    case EPERM:   return __WASI_ERRNO_PERM;   break;
    case ENOENT:  return __WASI_ERRNO_NOENT;  break;
    case ESRCH:   return __WASI_ERRNO_SRCH;   break;
    case EINTR:   return __WASI_ERRNO_INTR;   break;
    case EIO:     return __WASI_ERRNO_IO;     break;
    case ENXIO:   return __WASI_ERRNO_NXIO;   break;
    case E2BIG:   return __WASI_ERRNO_2BIG;   break;
    case ENOEXEC: return __WASI_ERRNO_NOEXEC; break;
    case EBADF:   return __WASI_ERRNO_BADF;   break;
    case ECHILD:  return __WASI_ERRNO_CHILD;  break;
    case EAGAIN:  return __WASI_ERRNO_AGAIN;  break;
    case ENOMEM:  return __WASI_ERRNO_NOMEM;  break;
    case EACCES:  return __WASI_ERRNO_ACCES;  break;
    case EFAULT:  return __WASI_ERRNO_FAULT;  break;
    case EBUSY:   return __WASI_ERRNO_BUSY;   break;
    case EEXIST:  return __WASI_ERRNO_EXIST;  break;
    case EXDEV:   return __WASI_ERRNO_XDEV;   break;
    case ENODEV:  return __WASI_ERRNO_NODEV;  break;
    case ENOTDIR: return __WASI_ERRNO_NOTDIR; break;
    case EISDIR:  return __WASI_ERRNO_ISDIR;  break;
    case EINVAL:  return __WASI_ERRNO_INVAL;  break;
    case ENFILE:  return __WASI_ERRNO_NFILE;  break;
    case EMFILE:  return __WASI_ERRNO_MFILE;  break;
    case ENOTTY:  return __WASI_ERRNO_NOTTY;  break;
    case ETXTBSY: return __WASI_ERRNO_TXTBSY; break;
    case EFBIG:   return __WASI_ERRNO_FBIG;   break;
    case ENOSPC:  return __WASI_ERRNO_NOSPC;  break;
    case ESPIPE:  return __WASI_ERRNO_SPIPE;  break;
    case EROFS:   return __WASI_ERRNO_ROFS;   break;
    case EMLINK:  return __WASI_ERRNO_MLINK;  break;
    case EPIPE:   return __WASI_ERRNO_PIPE;   break;
    case EDOM:    return __WASI_ERRNO_DOM;    break;
    case ERANGE:  return __WASI_ERRNO_RANGE;  break;
    }
    return __WASI_ERRNO_INVAL;
}

static inline
int convert_clockid(__wasi_clockid_t in) {
    switch (in) {
    case __WASI_CLOCKID_MONOTONIC:            return CLOCK_MONOTONIC;
    case __WASI_CLOCKID_PROCESS_CPUTIME_ID:   return CLOCK_PROCESS_CPUTIME_ID;
    case __WASI_CLOCKID_REALTIME:             return CLOCK_REALTIME;
    case __WASI_CLOCKID_THREAD_CPUTIME_ID:    return CLOCK_THREAD_CPUTIME_ID;
    default: return -1;
    }
}*/

/*static inline
__wasi_timestamp_t convert_timespec(const struct timespec *ts) {
    if (ts->tv_sec < 0)
        return 0;
    if ((__wasi_timestamp_t)ts->tv_sec >= UINT64_MAX / 1000000000)
        return UINT64_MAX;
    return (__wasi_timestamp_t)ts->tv_sec * 1000000000 + ts->tv_nsec;
}*/

#define m3ApiGetIov(o_iovs, wasi_iovs, iovs_len) \
    m3ApiCheckMem(wasi_iovs, iovs_len * sizeof(wasi_iovec_t)); \
    struct iovec o_iovs[iovs_len]; { \
    ssize_t sum_len = 0; \
    for (int i = 0; i < iovs_len; i++) { \
        o_iovs[i].iov_base = m3ApiOffsetToPtr(m3ApiReadMem32(&wasi_iovs[i].buf)); \
        o_iovs[i].iov_len  = m3ApiReadMem32(&wasi_iovs[i].buf_len); \
        m3ApiCheckMem(o_iovs[i].iov_base, o_iovs[i].iov_len); \
        if (sum_len > __LONG_MAX__ - o_iovs[i].iov_len) m3ApiReturn(__WASI_ERRNO_INVAL); \
        sum_len += o_iovs[i].iov_len; \
    } }

/*
 * WASI API implementation
 */

m3ApiRawFunction(m3_wasi_generic_args_get)
{
    m3ApiReturnType  (uint32_t)
    m3ApiGetArgMem   (uint32_t *           , argv)
    m3ApiGetArgMem   (char *               , argv_buf)

    m3_wasi_context_t* context = m3_GetWasiContext(runtime);

    if (context == NULL) { m3ApiReturn(__WASI_ERRNO_INVAL); }

    m3ApiCheckMem(argv, context->argc * sizeof(uint32_t));

    for (u32 i = 0; i < context->argc; ++i)
    {
        m3ApiWriteMem32(&argv[i], m3ApiPtrToOffset(argv_buf));

        size_t len = strlen (context->argv[i]);

        m3ApiCheckMem(argv_buf, len);
        memcpy (argv_buf, context->argv[i], len);
        argv_buf += len;
        * argv_buf++ = 0;
    }

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_generic_args_sizes_get)
{
    m3ApiReturnType  (uint32_t)
    m3ApiGetArgMem   (__wasi_size_t *      , argc)
    m3ApiGetArgMem   (__wasi_size_t *      , argv_buf_size)

    m3ApiCheckMem(argc,             sizeof(__wasi_size_t));
    m3ApiCheckMem(argv_buf_size,    sizeof(__wasi_size_t));

    m3_wasi_context_t* context =  m3_GetWasiContext(runtime);

    if (context == NULL) { m3ApiReturn(__WASI_ERRNO_INVAL); }

    __wasi_size_t buf_len = 0;
    for (u32 i = 0; i < context->argc; ++i)
    {
        buf_len += strlen (context->argv[i]) + 1;
    }

    m3ApiWriteMem32(argc, context->argc);
    m3ApiWriteMem32(argv_buf_size, buf_len);

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_generic_environ_get)
{
    m3ApiReturnType  (uint32_t)
    m3ApiGetArgMem   (uint32_t *           , env)
    m3ApiGetArgMem   (char *               , env_buf)

    (void)env;
    (void)env_buf;
    // TODO
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_generic_environ_sizes_get)
{
    m3ApiReturnType  (uint32_t)
    m3ApiGetArgMem   (__wasi_size_t *      , env_count)
    m3ApiGetArgMem   (__wasi_size_t *      , env_buf_size)

    m3ApiCheckMem(env_count,    sizeof(__wasi_size_t));
    m3ApiCheckMem(env_buf_size, sizeof(__wasi_size_t));

    // TODO
    m3ApiWriteMem32(env_count,    0);
    m3ApiWriteMem32(env_buf_size, 0);

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_generic_fd_prestat_dir_name)
{
    m3ApiReturnType  (uint32_t)
    m3ApiGetArg      (__wasi_fd_t          , fd)
    m3ApiGetArgMem   (char *               , path)
    m3ApiGetArg      (__wasi_size_t        , path_len)

    m3ApiCheckMem(path, path_len);

    if (fd < 3 || fd >= PREOPEN_CNT) { m3ApiReturn(__WASI_ERRNO_BADF); }
    size_t slen = strlen(preopen[fd].path) + 1;
    memcpy(path, preopen[fd].path, M3_MIN(slen, path_len));
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_generic_fd_prestat_get)
{
    m3ApiReturnType  (uint32_t)
    m3ApiGetArg      (__wasi_fd_t          , fd)
    m3ApiGetArgMem   (uint8_t *            , buf)

    m3ApiCheckMem(buf, 8);

    if (fd < 3 || fd >= PREOPEN_CNT) { m3ApiReturn(__WASI_ERRNO_BADF); }

    m3ApiWriteMem32(buf+0, __WASI_PREOPENTYPE_DIR);
    m3ApiWriteMem32(buf+4, strlen(preopen[fd].path) + 1);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_generic_fd_fdstat_get)
{
    m3ApiReturnType  (uint32_t)
    m3ApiGetArg      (__wasi_fd_t          , fd)
    m3ApiGetArgMem   (__wasi_fdstat_t *    , fdstat)

    m3ApiCheckMem(fdstat, sizeof(__wasi_fdstat_t));

    // TODO: This needs a proper implementation
    if (fd < 3) { // stdio
        fdstat->fs_filetype = __WASI_FILETYPE_CHARACTER_DEVICE;
    } else if (fd < PREOPEN_CNT) { // dot direcotory
        fdstat->fs_filetype = __WASI_FILETYPE_DIRECTORY;
    } else {
        fdstat->fs_filetype = __WASI_FILETYPE_REGULAR_FILE;
    }

    fdstat->fs_flags = 0;
    fdstat->fs_rights_base = (uint64_t)-1; // all rights
    fdstat->fs_rights_inheriting = (uint64_t)-1; // all rights
    m3ApiReturn(__WASI_ERRNO_SUCCESS);

/*
    struct stat fd_stat;

    int fl = fcntl(fd, F_GETFL);
    if (fl < 0) { m3ApiReturn(errno_to_wasi(errno)); }

    fstat(fd, &fd_stat);
    int mode = fd_stat.st_mode;
    fdstat->fs_filetype = (S_ISBLK(mode)   ? __WASI_FILETYPE_BLOCK_DEVICE     : 0) |
                          (S_ISCHR(mode)   ? __WASI_FILETYPE_CHARACTER_DEVICE : 0) |
                          (S_ISDIR(mode)   ? __WASI_FILETYPE_DIRECTORY        : 0) |
                          (S_ISREG(mode)   ? __WASI_FILETYPE_REGULAR_FILE     : 0) |
                          //(S_ISSOCK(mode)  ? __WASI_FILETYPE_SOCKET_STREAM    : 0) |
                          (S_ISLNK(mode)   ? __WASI_FILETYPE_SYMBOLIC_LINK    : 0);

    m3ApiWriteMem16(&fdstat->fs_flags,
                       ((fl & O_APPEND)    ? __WASI_FDFLAGS_APPEND    : 0) |
                       ((fl & O_DSYNC)     ? __WASI_FDFLAGS_DSYNC     : 0) |
                       ((fl & O_NONBLOCK)  ? __WASI_FDFLAGS_NONBLOCK  : 0) |
                       //((fl & O_RSYNC)     ? __WASI_FDFLAGS_RSYNC     : 0) |
                       ((fl & O_SYNC)      ? __WASI_FDFLAGS_SYNC      : 0));

    fdstat->fs_rights_base = (uint64_t)-1; // all rights

    // Make descriptors 0,1,2 look like a TTY
    if (fd <= 2) {
        fdstat->fs_rights_base &= ~(__WASI_RIGHTS_FD_SEEK | __WASI_RIGHTS_FD_TELL);
    }

    fdstat->fs_rights_inheriting = (uint64_t)-1; // all rights
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
*/
}

m3ApiRawFunction(m3_wasi_generic_fd_fdstat_set_flags)
{
    m3ApiReturnType  (uint32_t)
    m3ApiGetArg      (__wasi_fd_t          , fd)
    m3ApiGetArg      (__wasi_fdflags_t     , flags)

    (void)fd;
    (void)flags;
    // TODO

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_unstable_fd_seek)
{
    m3ApiReturnType  (uint32_t)
    m3ApiGetArg      (__wasi_fd_t          , fd)
    m3ApiGetArg      (__wasi_filedelta_t   , offset)
    m3ApiGetArg      (uint32_t             , wasi_whence)
    m3ApiGetArgMem   (__wasi_filesize_t *  , result)

    m3ApiCheckMem(result, sizeof(__wasi_filesize_t));

    (void)fd;
    (void)offset;
    (void)wasi_whence;
    (void)result;
    /*int whence;

    switch (wasi_whence) {
    case 0: whence = SEEK_CUR; break;
    case 1: whence = SEEK_END; break;
    case 2: whence = SEEK_SET; break;
    default:                m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    int64_t ret;
#if defined(M3_COMPILER_MSVC) || defined(__MINGW32__)
    ret = _lseeki64(fd, offset, whence);
#else
    ret = lseek(fd, offset, whence);
#endif
    if (ret < 0) { m3ApiReturn(errno_to_wasi(errno)); }
    m3ApiWriteMem64(result, ret);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);*/
    m3ApiReturn(__WASI_ERRNO_NOSYS);
}

m3ApiRawFunction(m3_wasi_snapshot_preview1_fd_seek)
{
    m3ApiReturnType  (uint32_t)
    m3ApiGetArg      (__wasi_fd_t          , fd)
    m3ApiGetArg      (__wasi_filedelta_t   , offset)
    m3ApiGetArg      (uint32_t             , wasi_whence)
    m3ApiGetArgMem   (__wasi_filesize_t *  , result)

    m3ApiCheckMem(result, sizeof(__wasi_filesize_t));

    (void)fd;
    (void)offset;
    (void)wasi_whence;
    (void)result;
    /*int whence;

    switch (wasi_whence) {
    case 0: whence = SEEK_SET; break;
    case 1: whence = SEEK_CUR; break;
    case 2: whence = SEEK_END; break;
    default:                m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    int64_t ret;
#if defined(M3_COMPILER_MSVC) || defined(__MINGW32__)
    ret = _lseeki64(fd, offset, whence);
#else
    ret = lseek(fd, offset, whence);
#endif
    if (ret < 0) { m3ApiReturn(errno_to_wasi(errno)); }
    m3ApiWriteMem64(result, ret);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);*/
    m3ApiReturn(__WASI_ERRNO_NOSYS)
}


m3ApiRawFunction(m3_wasi_generic_path_open)
{
    m3ApiReturnType  (uint32_t)
    m3ApiGetArg      (__wasi_fd_t          , dirfd)
    m3ApiGetArg      (__wasi_lookupflags_t , dirflags)
    m3ApiGetArgMem   (const char *         , path)
    m3ApiGetArg      (__wasi_size_t        , path_len)
    m3ApiGetArg      (__wasi_oflags_t      , oflags)
    m3ApiGetArg      (__wasi_rights_t      , fs_rights_base)
    m3ApiGetArg      (__wasi_rights_t      , fs_rights_inheriting)
    m3ApiGetArg      (__wasi_fdflags_t     , fs_flags)
    m3ApiGetArgMem   (__wasi_fd_t *        , fd)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(fd,   sizeof(__wasi_fd_t));

    if (path_len >= 512)
        m3ApiReturn(__WASI_ERRNO_INVAL);

    // copy path so we can ensure it is NULL terminated
#if defined(M3_COMPILER_MSVC)
    char host_path[512];
#else
    char host_path[path_len+1];
#endif
    memcpy (host_path, path, path_len);
    host_path[path_len] = '\0'; // NULL terminator


    (void)dirfd;
    (void)dirflags;
    (void)oflags;
    (void)fs_rights_base;
    (void)fs_rights_inheriting;
    (void)fs_flags;
    (void)fd;
/*
#if defined(APE)
    // TODO: This all needs a proper implementation

    int flags = ((oflags & __WASI_OFLAGS_CREAT)             ? O_CREAT     : 0) |
                ((oflags & __WASI_OFLAGS_EXCL)              ? O_EXCL      : 0) |
                ((oflags & __WASI_OFLAGS_TRUNC)             ? O_TRUNC     : 0) |
                ((fs_flags & __WASI_FDFLAGS_APPEND)     ? O_APPEND    : 0);

    if ((fs_rights_base & __WASI_RIGHTS_FD_READ) &&
        (fs_rights_base & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_RDWR;
    } else if ((fs_rights_base & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_WRONLY;
    } else if ((fs_rights_base & __WASI_RIGHTS_FD_READ)) {
        flags |= O_RDONLY; // no-op because O_RDONLY is 0
    }
    int mode = 0644;

    int host_fd = open (host_path, flags, mode);

    if (host_fd < 0)
    {
        m3ApiReturn(errno_to_wasi (errno));
    }
    else
    {
        m3ApiWriteMem32(fd, host_fd);
        m3ApiReturn(__WASI_ERRNO_SUCCESS);
    }
#elif defined(_WIN32)
    // TODO: This all needs a proper implementation

    int flags = ((oflags & __WASI_OFLAGS_CREAT)             ? _O_CREAT     : 0) |
                ((oflags & __WASI_OFLAGS_EXCL)              ? _O_EXCL      : 0) |
                ((oflags & __WASI_OFLAGS_TRUNC)             ? _O_TRUNC     : 0) |
                ((fs_flags & __WASI_FDFLAGS_APPEND)         ? _O_APPEND    : 0) |
                _O_BINARY;

    if ((fs_rights_base & __WASI_RIGHTS_FD_READ) &&
        (fs_rights_base & __WASI_RIGHTS_FD_WRITE)) {
        flags |= _O_RDWR;
    } else if ((fs_rights_base & __WASI_RIGHTS_FD_WRITE)) {
        flags |= _O_WRONLY;
    } else if ((fs_rights_base & __WASI_RIGHTS_FD_READ)) {
        flags |= _O_RDONLY; // no-op because O_RDONLY is 0
    }
    int mode = 0644;

    int host_fd = open (host_path, flags, mode);

    if (host_fd < 0)
    {
        m3ApiReturn(errno_to_wasi (errno));
    }
    else
    {
        m3ApiWriteMem32(fd, host_fd);
        m3ApiReturn(__WASI_ERRNO_SUCCESS);
    }
#else
    // translate o_flags and fs_flags into flags and mode
    int flags = ((oflags & __WASI_OFLAGS_CREAT)             ? O_CREAT     : 0) |
                //((oflags & __WASI_OFLAGS_DIRECTORY)         ? O_DIRECTORY : 0) |
                ((oflags & __WASI_OFLAGS_EXCL)              ? O_EXCL      : 0) |
                ((oflags & __WASI_OFLAGS_TRUNC)             ? O_TRUNC     : 0) |
                ((fs_flags & __WASI_FDFLAGS_APPEND)     ? O_APPEND    : 0) |
                ((fs_flags & __WASI_FDFLAGS_DSYNC)      ? O_DSYNC     : 0) |
                ((fs_flags & __WASI_FDFLAGS_NONBLOCK)   ? O_NONBLOCK  : 0) |
                //((fs_flags & __WASI_FDFLAGS_RSYNC)      ? O_RSYNC     : 0) |
                ((fs_flags & __WASI_FDFLAGS_SYNC)       ? O_SYNC      : 0);
    if ((fs_rights_base & __WASI_RIGHTS_FD_READ) &&
        (fs_rights_base & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_RDWR;
    } else if ((fs_rights_base & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_WRONLY;
    } else if ((fs_rights_base & __WASI_RIGHTS_FD_READ)) {
        flags |= O_RDONLY; // no-op because O_RDONLY is 0
    }
    int mode = 0644;
    int host_fd = openat (preopen[dirfd].fd, host_path, flags, mode);

    if (host_fd < 0)
    {
        m3ApiReturn(errno_to_wasi (errno));
    }
    else
    {
        m3ApiWriteMem32(fd, host_fd);
        m3ApiReturn(__WASI_ERRNO_SUCCESS);
    }
#endif*/
    m3ApiReturn(__WASI_ERRNO_NOSYS);
}

m3ApiRawFunction(m3_wasi_generic_fd_read)
{
    m3ApiReturnType  (uint32_t)
    m3ApiGetArg      (__wasi_fd_t          , fd)
    m3ApiGetArgMem   (wasi_iovec_t *       , wasi_iovs)
    m3ApiGetArg      (__wasi_size_t        , iovs_len)
    m3ApiGetArgMem   (__wasi_size_t *      , nread)

    m3ApiCheckMem(wasi_iovs,    iovs_len * sizeof(wasi_iovec_t));
    m3ApiCheckMem(nread,        sizeof(__wasi_size_t));

    (void)fd;
    /* TODO: struct iovec iovs[iovs_len];
    copy_iov_to_host(_mem, iovs, wasi_iovs, iovs_len);

    ssize_t ret = readv(fd, iovs, iovs_len);
    if (ret < 0) { m3ApiReturn(errno_to_wasi(errno)); }
    m3ApiWriteMem32(nread, ret);
    m3ApiReturn(__WASI_ERRNO_SUCCESS); */
    m3ApiReturn(__WASI_ERRNO_NOSYS);
}

m3ApiRawFunction(m3_wasi_generic_fd_write)
{
    m3ApiReturnType  (uint32_t)
    m3ApiGetArg      (__wasi_fd_t          , fd)
    m3ApiGetArgMem   (wasi_iovec_t *       , wasi_iovs)
    m3ApiGetArg      (__wasi_size_t        , iovs_len)
    m3ApiGetArgMem   (__wasi_size_t *      , nwritten)

    m3ApiGetIov(iovs, wasi_iovs, iovs_len);
    m3ApiCheckMem(nwritten,     sizeof(__wasi_size_t));

    PROGRAM *p = m3_GetUserData(runtime);
    if (p == NULL) m3ApiReturn(__WASI_ERRNO_NOTCAPABLE);

    if (fd == 1) {
        ssize_t ret = service_send("stdout:", iovs, iovs_len, &p->rights);
        if (ret < 0) m3ApiReturn(__WASI_ERRNO_RANGE);
        m3ApiWriteMem32(nwritten, ret);
        m3ApiReturn(__WASI_ERRNO_SUCCESS);
    }

    //TODO:
    m3ApiReturn(__WASI_ERRNO_NOSYS);
}

m3ApiRawFunction(m3_wasi_generic_fd_close)
{
    m3ApiReturnType  (uint32_t)
    m3ApiGetArg      (__wasi_fd_t, fd)

    (void)fd;
    // TODO: int ret = close(fd);
    // m3ApiReturn(ret == 0 ? __WASI_ERRNO_SUCCESS : ret);
    m3ApiReturn(__WASI_ERRNO_NOSYS);
}

m3ApiRawFunction(m3_wasi_generic_fd_datasync)
{
    m3ApiReturnType  (uint32_t)
    m3ApiGetArg      (__wasi_fd_t, fd)

    (void)fd;
    //TODO: int ret = fdatasync(fd);
    int ret = __WASI_ERRNO_NOSYS;
    m3ApiReturn(ret == 0 ? __WASI_ERRNO_SUCCESS : ret);
}

m3ApiRawFunction(m3_wasi_generic_random_get)
{
    m3ApiReturnType  (uint32_t)
    m3ApiGetArgMem   (uint8_t *            , buf)
    m3ApiGetArg      (__wasi_size_t        , buf_len)

    m3ApiCheckMem(buf, buf_len);

    while (1) {
        // ssize_t retlen = 0;

        // TODO: retlen = getrandom(buf, buf_len, 0);
        m3ApiReturn(__WASI_ERRNO_NOSYS);

        /*if (retlen < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            m3ApiReturn(errno_to_wasi(errno));
        } else if (retlen == buf_len) {
            m3ApiReturn(__WASI_ERRNO_SUCCESS);
        } else {
            buf     += retlen;
            buf_len -= retlen;
        }*/
    }
}

m3ApiRawFunction(m3_wasi_generic_clock_res_get)
{
    m3ApiReturnType  (uint32_t)
    m3ApiGetArg      (__wasi_clockid_t     , wasi_clk_id)
    m3ApiGetArgMem   (__wasi_timestamp_t * , resolution)

    m3ApiCheckMem(resolution, sizeof(__wasi_timestamp_t));

    (void)wasi_clk_id;
    /*int clk = convert_clockid(wasi_clk_id);
    if (clk < 0) m3ApiReturn(__WASI_ERRNO_INVAL);

    struct timespec tp;
    if (clock_getres(clk, &tp) != 0) {
        m3ApiWriteMem64(resolution, 1000000);
    } else {
        m3ApiWriteMem64(resolution, convert_timespec(&tp));
    }

    m3ApiReturn(__WASI_ERRNO_SUCCESS);*/
    m3ApiReturn(__WASI_ERRNO_NOSYS);
}

m3ApiRawFunction(m3_wasi_generic_clock_time_get)
{
    m3ApiReturnType  (uint32_t)
    m3ApiGetArg      (__wasi_clockid_t     , wasi_clk_id)
    m3ApiGetArg      (__wasi_timestamp_t   , precision)
    m3ApiGetArgMem   (__wasi_timestamp_t * , time)

    m3ApiCheckMem(time, sizeof(__wasi_timestamp_t));

    (void)wasi_clk_id;
    (void)precision;
    /*int clk = convert_clockid(wasi_clk_id);
    if (clk < 0) m3ApiReturn(__WASI_ERRNO_INVAL);

    struct timespec tp;
    if (clock_gettime(clk, &tp) != 0) {
        m3ApiReturn(errno_to_wasi(errno));
    }

    m3ApiWriteMem64(time, convert_timespec(&tp));
    m3ApiReturn(__WASI_ERRNO_SUCCESS);*/
    m3ApiReturn(__WASI_ERRNO_NOSYS);
}

m3ApiRawFunction(m3_wasi_generic_proc_exit)
{
    m3ApiGetArg      (uint32_t, code)

    m3_wasi_context_t* context =  m3_GetWasiContext(runtime);

    if (context) {
        context->exit_code = code;
    }

    m3ApiTrap(m3Err_trapExit);
}


static
M3Result SuppressLookupFailure(M3Result i_result)
{
    if (i_result == m3Err_functionLookupFailed)
        return m3Err_none;
    else
        return i_result;
}

m3_wasi_context_t* m3_GetWasiContext(IM3Runtime io_runtime)
{
    return io_runtime->wasiContext;
}


M3Result  m3_LinkWASI  (IM3Module module)
{
    M3Result result = m3Err_none;

    // Preopen dirs
    /*for (int i = 3; i < PREOPEN_CNT; i++) {
        preopen[i].fd = open(preopen[i].real_path, O_RDONLY);
    }*/

    if (!m3_GetModuleRuntime(module)) {
        result = m3Err_mallocFailed;
        goto _catch;
    }
    if (!m3_GetModuleRuntime(module)->wasiContext) {
        m3_wasi_context_t* wasi_context = (m3_wasi_context_t*)malloc(sizeof(m3_wasi_context_t));
        wasi_context->exit_code = 0;
        wasi_context->argc = 0;
        wasi_context->argv = 0;
        m3_GetModuleRuntime(module)->wasiContext = wasi_context;
    }

    static const char* namespaces[2] = { "wasi_unstable", "wasi_snapshot_preview1" };

    // fd_seek is incompatible
//_ (SuppressLookupFailure (m3_LinkRawFunction (module, "wasi_unstable",          "fd_seek",     "i(iIi*)", &m3_wasi_unstable_fd_seek)));
//_ (SuppressLookupFailure (m3_LinkRawFunction (module, "wasi_snapshot_preview1", "fd_seek",     "i(iIi*)", &m3_wasi_snapshot_preview1_fd_seek)));

    for (int i=0; i<2; i++)
    {
        const char* wasi = namespaces[i];

_       (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "args_get",           "i(**)",   &m3_wasi_generic_args_get)));
_       (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "args_sizes_get",     "i(**)",   &m3_wasi_generic_args_sizes_get)));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "clock_res_get",        "i(i*)",   &m3_wasi_generic_clock_res_get)));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "clock_time_get",       "i(iI*)",  &m3_wasi_generic_clock_time_get)));
_       (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "environ_get",          "i(**)",   &m3_wasi_generic_environ_get)));
_       (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "environ_sizes_get",    "i(**)",   &m3_wasi_generic_environ_sizes_get)));

//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "fd_advise",            "i(iIIi)", )));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "fd_allocate",          "i(iII)",  )));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "fd_close",             "i(i)",    &m3_wasi_generic_fd_close)));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "fd_datasync",          "i(i)",    &m3_wasi_generic_fd_datasync)));
_       (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "fd_fdstat_get",        "i(i*)",   &m3_wasi_generic_fd_fdstat_get)));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "fd_fdstat_set_flags",  "i(ii)",   &m3_wasi_generic_fd_fdstat_set_flags)));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "fd_fdstat_set_rights", "i(iII)",  )));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "fd_filestat_get",      "i(i*)",   )));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "fd_filestat_set_size", "i(iI)",   )));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "fd_filestat_set_times","i(iIIi)", )));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "fd_pread",             "i(i*iI*)",)));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "fd_prestat_get",       "i(i*)",   &m3_wasi_generic_fd_prestat_get)));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "fd_prestat_dir_name",  "i(i*i)",  &m3_wasi_generic_fd_prestat_dir_name)));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "fd_pwrite",            "i(i*iI*)",)));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "fd_read",              "i(i*i*)", &m3_wasi_generic_fd_read)));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "fd_readdir",           "i(i*iI*)",)));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "fd_renumber",          "i(ii)",   )));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "fd_sync",              "i(i)",    )));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "fd_tell",              "i(i*)",   )));
_       (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "fd_write",             "i(i*i*)", &m3_wasi_generic_fd_write)));

//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "path_create_directory",    "i(i*i)",       )));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "path_filestat_get",        "i(ii*i*)",     &m3_wasi_generic_path_filestat_get)));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "path_filestat_set_times",  "i(ii*iIIi)",   )));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "path_link",                "i(ii*ii*i)",   )));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "path_open",                "i(ii*iiIIi*)", &m3_wasi_generic_path_open)));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "path_readlink",            "i(i*i*i*)",    )));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "path_remove_directory",    "i(i*i)",       )));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "path_rename",              "i(i*ii*i)",    )));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "path_symlink",             "i(*ii*i)",     )));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "path_unlink_file",         "i(i*i)",       )));

//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "poll_oneoff",          "i(**i*)", &m3_wasi_generic_poll_oneoff)));
_       (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "proc_exit",          "v(i)",    &m3_wasi_generic_proc_exit)));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "proc_raise",           "i(i)",    )));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "random_get",           "i(*i)",   &m3_wasi_generic_random_get)));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "sched_yield",          "i()",     )));

//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "sock_recv",            "i(i*ii**)",        )));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "sock_send",            "i(i*ii*)",         )));
//_     (SuppressLookupFailure (m3_LinkRawFunction (module, wasi, "sock_shutdown",        "i(ii)",            )));
    }

_catch:
    return result;
}

m3ApiRawFunction(m3_srv_send) {
    m3ApiReturnType(int32_t);
    m3ApiGetArgMem(const char*, path);
    m3ApiGetArgMem(const uint8_t*, data);
    m3ApiGetArg(uint32_t, len);
    m3ApiCheckMem(path, strlen(path));
    m3ApiCheckMem(data, len);

    struct iovec iov = {(void*)data, len};
    PROGRAM *p = m3_GetUserData(runtime);
    if (p == NULL) m3ApiReturn(-1);

    m3ApiReturn(service_send(path, &iov, 1, &p->rights));
}
m3ApiRawFunction(m3_srv_sendv) {
    m3ApiReturnType  (int32_t)
    m3ApiGetArgMem(const char*, path);
    m3ApiGetArgMem   (wasi_iovec_t *       , wasi_iovs)
    m3ApiGetArg      (__wasi_size_t        , iovs_len)
    m3ApiGetArgMem   (__wasi_size_t *      , nout)

    m3ApiGetIov(iovs, wasi_iovs, iovs_len);
    m3ApiCheckMem(nout, sizeof(__wasi_size_t));

    PROGRAM *p = m3_GetUserData(runtime);
    if (p == NULL) m3ApiReturn(-1);

    ssize_t ret = service_send(path, iovs, iovs_len, &p->rights);
    if (ret < 0) m3ApiReturn(ret);
    m3ApiWriteMem32(nout, ret);
    m3ApiReturn(0);
}

#endif // d_m3HasWASI
