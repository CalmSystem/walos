#include "stdlib.h"
#include "../memory.h"
#include "assert.h"
#include "string.h"
#include "stdbool.h"

typedef struct {
    void* start;
    size_t size;
} alloc_t;

#define BORDER_SIZE sizeof(unsigned long)*4

// Write size and magic before and after allocated block
static inline unsigned long knuth_mmix_one_round(unsigned long in) {
    return in * 6364136223846793005UL % 1442695040888963407UL;
}
void* alloc_set(alloc_t a) {
    unsigned long* begin = a.start;
    unsigned long* end = a.start + a.size;

    *begin = a.size;
    *(end - 1) = a.size;

    unsigned long magic = knuth_mmix_one_round((unsigned long)a.start);
    *(begin + 1) = magic;
    *(end - 2) = magic;

    return begin + 2;
}
alloc_t alloc_get(void *ptr) {
    unsigned long* data = ptr;
    unsigned long* begin = data - 2;
    unsigned long magic = *(begin + 1);

    alloc_t a = {};
    a.start = begin;
    a.size = *begin;

    unsigned long *end = (unsigned long*)(ptr + a.size) - 2;
    assert(*(end - 1) == a.size);
    assert(*(end - 2) == magic);
    assert(magic == knuth_mmix_one_round((unsigned long)a.start));

    return a;
}

#define SLAB_MAX_EXP 7
#define SLAB_SIZE (1 << SLAB_MAX_EXP)
#define IS_SLAB(size) (size < SLAB_SIZE)
#define BUDDY_MIN_EXP (SLAB_MAX_EXP+1)
#define BUDDY_MAX_EXP 17
#define BUDDY_MAX (1 << BUDDY_MAX_EXP)-1
#define IS_BUDDY(size) (size <= BUDDY_MAX)

#define TO_PAGES(size) ((size-1) / PAGE_SIZE + 1)

/** Linked list of slab chunks */
void* slab_head = NULL;
/** Two exponant of next slab page allocation */
uint64_t slab_next_exp = 12;

/** Linked lists of (i+BUDDY_MIN_EXP)^2 buddy chunks */
void* buddy_tab[BUDDY_MAX_EXP-BUDDY_MIN_EXP] = {0};

static inline unsigned int exp_of_2(unsigned long size) {
    unsigned int p = 0;
    size--; // allocation start in 0
    while(size) { // get the largest bit
        p++;
        size >>= 1;
    }
    if (size > (1 << p)) p++;
    return p;
}

static inline void* alloc(size_t size, bool zero) {
    alloc_t a;
    a.size = size + BORDER_SIZE;
    if (IS_SLAB(a.size)) {
        if (slab_head == NULL) {
            uint64_t slab_size = 1 << (slab_next_exp++);
            slab_head = page_alloc(TO_PAGES(slab_size));
            if (slab_head == NULL) return NULL;

            uint64_t offset = 0;
            while (offset < slab_size) {
                *(void**)(slab_head + offset) = slab_head + offset + SLAB_SIZE;
                offset += SLAB_SIZE;
            }
            *(void**)(slab_head + offset - SLAB_SIZE) = NULL;
        }
        a.start = slab_head;
        slab_head = *(void**)a.start;
        if (zero) memset(a.start + BORDER_SIZE / 2, 0, SLAB_SIZE - BORDER_SIZE);
    } else if (IS_BUDDY(a.size)) {
        const unsigned int pow2 = exp_of_2(a.size);
        unsigned int parent2 = pow2; // First free chunk
        while (parent2 < BUDDY_MAX_EXP && buddy_tab[parent2-BUDDY_MIN_EXP] == NULL) {
            parent2++;
        }
        if (parent2 == BUDDY_MAX_EXP) {
            a.start = page_alloc(TO_PAGES((1UL << BUDDY_MAX_EXP)));
            if (a.start == NULL) return NULL;
        } else {
            a.start = buddy_tab[parent2-BUDDY_MIN_EXP];
            buddy_tab[parent2-BUDDY_MIN_EXP] = *(void**)a.start;
        }

        parent2--; // Store compagnons
        for (;parent2 >= pow2; parent2--) {
            void* half = a.start + (1UL<<parent2);
            *(void**)half = NULL;
            buddy_tab[parent2-BUDDY_MIN_EXP] = half;
        }
        if (zero) memset(a.start + BORDER_SIZE / 2, 0, a.size - BORDER_SIZE);
    } else {
        a.start = (zero ? page_calloc : page_alloc)(TO_PAGES(a.size));
        if (a.start == NULL) return NULL;
    }
    return alloc_set(a);
}
void free(void* ptr) {
    if (ptr == NULL) return;
    alloc_t a = alloc_get(ptr);
    if (IS_SLAB(a.size)) {
        *(void**)a.start = slab_head;
        slab_head = a.start;
    } else if (IS_BUDDY(a.size)) {
        unsigned int pow2 = exp_of_2(a.size);
        a.size = 1UL<<pow2;
        while (pow2 < BUDDY_MAX_EXP-BUDDY_MIN_EXP) {
            void *buddy = (void*)((uint64_t)a.start ^ a.size);
            void *before_buddy = buddy_tab[pow2-BUDDY_MIN_EXP];
            while (before_buddy != NULL) {
                if (*(void**)before_buddy == buddy) {
                    // Extract buddy
                    *(void**)before_buddy = *(void**)buddy;
                    break;
                }
                before_buddy = *(void**)before_buddy;
            }
            if (before_buddy == NULL) {
                *(void**)a.start = buddy_tab[pow2-BUDDY_MIN_EXP];
                buddy_tab[pow2-BUDDY_MIN_EXP] = a.start;
                break;
            }
            pow2++;
            a.size = 1UL<<pow2;
            if (buddy < a.start)
                a.start = buddy;
        }
    } else {
        page_free((page*)a.start, TO_PAGES(a.size));
    }
}

void* malloc(size_t size) {
    return alloc(size, false);
}
void* calloc(size_t num, size_t size) {
    return alloc(num * size, true);
}
void* realloc(void* ptr, size_t size) {
    if (ptr == NULL) return malloc(size);

    alloc_t a = alloc_get(ptr);
    // Fit in alloc
    if (size <= a.size - BORDER_SIZE)
        return ptr;

    // Fit in pages
    if (!IS_BUDDY(a.size) && size <= TO_PAGES(a.size) * PAGE_SIZE - BORDER_SIZE) {
        a.size = size + BORDER_SIZE;
        return alloc_set(a);
    }

    // Move
    void* to = malloc(size);
    memcpy(to, ptr, a.size - BORDER_SIZE);
    free(ptr);
    return to;
}
