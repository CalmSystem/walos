#include "mem_disk.h"
#include "../memory.h"
#include "assert.h"
#include "string.h"
#include "errno.h"

#define SECTOR_SIZE 512
#define SECTOR_PER_PAGE (PAGE_SIZE / SECTOR_SIZE)

struct mem_disk {
    disk self;
    size_t npages;
};
static inline bool check_dense_range(void* d, sector_n count, sector_n offset) {
    return offset + count < ((struct mem_disk *)d)->npages * SECTOR_PER_PAGE - 1;
}
static int mem_disk_read(disk* d, void* out, sector_n count, sector_n offset) {
    if (!check_dense_range(d, count, offset)) return -ERANGE;
    memcpy(out, (void *)d + SECTOR_SIZE * (offset + 1), count * SECTOR_SIZE);
    return 1;
}
static int mem_disk_write(disk* d, const void* in, sector_n count, sector_n offset) {
    if (!check_dense_range(d, count, offset)) return -ERANGE;
    memcpy((void *)d + SECTOR_SIZE * (offset + 1), in, count * SECTOR_SIZE);
    return 1;
}
static inline int _mem_disk_cmd(disk * d, enum disk_command cmd, void *io,
    size_t sector_count, size_t block_size, int (*trim)(disk *d, size_t block), int (*eject)(disk *d))
{
    switch (cmd)
    {
    case DISK_GET_STATUS:
        if (!io)
            return -EINVAL;
        *(enum disk_status *)io = 0;
        return 1;
    case DISK_SYNC:
        return 1;
    case DISK_GET_SECTOR_SIZE:
        if (!io)
            return -EINVAL;
        *(size_t *)io = SECTOR_SIZE;
        return 1;
    case DISK_GET_SECTOR_COUNT:
        if (!io)
            return -EINVAL;
        *(size_t *)io = sector_count;
        return 1;
    case DISK_GET_BLOCK_SIZE:
        if (!io)
            return -EINVAL;
        *(size_t *)io = block_size;
        return 1;
    case DISK_TRIM:
        if (!io)
            return -EINVAL;
        return trim ? trim(d, *(size_t *)io) : -EPROTONOSUPPORT;
    case DISK_POWER:
        if (!io)
            return -EINVAL;
        return *(bool *)io ? 1 : -EPROTONOSUPPORT;
    case DISK_EJECT:
        d->write = NULL;
        d->read = NULL;
        d->cmd = NULL;
        return eject(d);
    default:
        return -EINVAL;
    }
}
static int mem_disk_eject_dense(disk* d) {
    page_free((page *)(void *)d, ((struct mem_disk*)d)->npages);
    return 1;
}
static int mem_disk_cmd(disk * d, enum disk_command cmd, void *io) {
    size_t sectors = ((struct mem_disk *)d)->npages * SECTOR_PER_PAGE - 1;
    return _mem_disk_cmd(d, cmd, io, sectors, sectors, NULL, mem_disk_eject_dense);
}

//FIXME: protect pages with spinlock
struct mem_disk_sparse {
    disk self;
    size_t npages;
    page *pages[0];
};
#define SPARSE_HEAD_SIZE(npages) (sizeof(struct mem_disk_sparse)+npages*sizeof(page*))
#define SPARSE_HEAD_PAGES(npages) ((SPARSE_HEAD_SIZE(npages)-1)/PAGE_SIZE+1)
static int mem_disk_read_sparse(disk* d, void* out, sector_n count, sector_n offset) {
    struct mem_disk_sparse* s = (void*)d;
    const size_t head = SPARSE_HEAD_PAGES(s->npages);
    if (offset + count >= (s->npages - head) * SECTOR_PER_PAGE) return -ERANGE;
    for (size_t i = 0; i < count; i++) {
        const size_t page = head + (offset + i) / SECTOR_PER_PAGE;
        if (s->pages[page])
            memcpy(out + i * SECTOR_SIZE, (void*)s->pages[page] + (offset + i) % SECTOR_PER_PAGE, SECTOR_SIZE);
        else
            memset(out + i * SECTOR_SIZE, 0, SECTOR_SIZE);
    }
    return 1;
}
static int mem_disk_write_sparse(disk* d, const void* in, sector_n count, sector_n offset) {
    struct mem_disk_sparse* s = (void*)d;
    const size_t head = SPARSE_HEAD_PAGES(s->npages);
    if (offset + count >= (s->npages - head) * SECTOR_PER_PAGE) return -ERANGE;
    for (size_t i = 0; i < count; i++) {
        const size_t page = head + (offset + i) / SECTOR_PER_PAGE;
        if (!s->pages[page]) {
            s->pages[page] = page_calloc(1);
            if (!s->pages[page]) return -ENOMEM;
        }
        memcpy((void *)s->pages[page] + (offset + i) % SECTOR_PER_PAGE, in + i * SECTOR_SIZE, SECTOR_SIZE);
    }
    return 1;
}
static int mem_disk_trim_sparse(disk* d, size_t block) {
    struct mem_disk_sparse *s = (void *)d;
    const size_t head = SPARSE_HEAD_PAGES(s->npages);
    const size_t page = head + block;
    if (page >= s->npages) return -ERANGE;
    if (s->pages[page]) {
        page_free(s->pages[page], 1);
        s->pages[page] = NULL;
    }
    return 1;
}
static int mem_disk_eject_sparse(disk* d) {
    struct mem_disk_sparse* s = (void*)d;
    for (size_t i = 0; i < s->npages; i++) {
        if (s->pages[i]) page_free(s->pages[i], 1);
    }
    return 1;
}
static int mem_disk_cmd_sparse(disk* d, enum disk_command cmd, void *io) {
    size_t npages = ((struct mem_disk_sparse*)d)->npages;
    return _mem_disk_cmd(d, cmd, io, (npages - SPARSE_HEAD_PAGES(npages)) * SECTOR_PER_PAGE,
        PAGE_SIZE, mem_disk_trim_sparse, mem_disk_eject_sparse);
}

disk* mem_disk_create(size_t npages, bool sparse) {
    if (npages <= sparse ? SPARSE_HEAD_PAGES(npages) : 0) return NULL;

    if (sparse) {
        struct mem_disk_sparse *disk = (void*)page_calloc(SPARSE_HEAD_PAGES(npages));
        if (!disk) return NULL;

        disk->npages = npages;
        for (size_t i = 0; i < SPARSE_HEAD_PAGES(npages); i++) {
            disk->pages[i] = (void*)disk + i*PAGE_SIZE;
        }
        disk->self.read = mem_disk_read_sparse;
        disk->self.write = mem_disk_write_sparse;
        disk->self.cmd = mem_disk_cmd_sparse;
        return (void*)disk;
    } else {
        struct mem_disk *disk = (void*)page_calloc(npages);
        if (!disk) return NULL;

        disk->npages = npages;
        disk->self.read = mem_disk_read;
        disk->self.write = mem_disk_write;
        disk->self.cmd = mem_disk_cmd;
        return (void*)disk;
    }
}
