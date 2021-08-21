#ifndef __BITMAP_H
#define __BITMAP_H

#include "stdint.h"
#include "stdbool.h"
#include "limits.h"

typedef uint64_t bitword_t;
enum { BITS_PER_WORD = sizeof(bitword_t) * CHAR_BIT };
#define WORD_OFFSET(b) ((b) / BITS_PER_WORD)
#define BIT_OFFSET(b)  ((b) % BITS_PER_WORD)

static inline void bit_mark(bitword_t *words, unsigned int n) {
    words[WORD_OFFSET(n)] |= (1 << BIT_OFFSET(n));
}
static inline void bit_clear(bitword_t *words, unsigned int n) {
    words[WORD_OFFSET(n)] &= ~(1 << BIT_OFFSET(n));
}
static inline void bit_set(bitword_t *words, unsigned int n, int val) {
    val ? bit_mark(words, n) : bit_clear(words, n);
}
static inline bool bit_get(bitword_t *words, unsigned int n) {
    bitword_t bit = words[WORD_OFFSET(n)] & (1 << BIT_OFFSET(n));
    return bit != 0;
}

#endif
