/*
 * This file has been generated from klibc :
 * ftp://ftp.kernel.org/pub/linux/libs/klibc/
 *
 * By issuing the command :
 * cat memccpy.c memchr.c memrchr.c memcmp.c memcpy.c memmove.c memset.c strxspn.c memmem.c memswap.c strcat.c strchr.c strrchr.c strcmp.c strcpy.c strcspn.c strlen.c strnlen.c strncat.c strlcat.c strncmp.c strncpy.c strpbrk.c strsep.c strspn.c strstr.c strtok.c
 *
 * in the subdirectory :
 * usr/klibc/
 *
 * and then slightly edited.
 */
#define UCHAR_MAX 255U


/*
 * memccpy.c
 *
 * memccpy()
 */

#include <stddef.h>
#include <string.h>

void *memccpy(void *dst, const void *src, int c, size_t n)
{
	char *q = dst;
	const char *p = src;
	char ch;

	while (n--) {
		*q++ = ch = *p++;
		if (ch == (char)c)
			return q;
	}

	return NULL;		/* No instance of "c" found */
}
/*
 * memchr.c
 */

#include <stddef.h>
#include <string.h>

void *memchr(const void *s, int c, size_t n)
{
	const unsigned char *sp = s;

	while (n--) {
		if (*sp == (unsigned char)c)
			return (void *)sp;
		sp++;
	}

	return NULL;
}
/*
 * memrchr.c
 */

#include <stddef.h>
#include <string.h>

void *memrchr(const void *s, int c, size_t n)
{
	const unsigned char *sp = (const unsigned char *)s + n - 1;

	while (n--) {
		if (*sp == (unsigned char)c)
			return (void *)sp;
		sp--;
	}

	return NULL;
}
/*
 * memcmp.c
 */

#include <string.h>

int memcmp(const void *s1, const void *s2, size_t n)
{
	const unsigned char *c1 = s1, *c2 = s2;
	int d = 0;

	while (n--) {
		d = (int)*c1++ - (int)*c2++;
		if (d)
			break;
	}

	return d;
}
/*
 * memcpy.c
 */

#include <string.h>

void *memcpy(void *dst, const void *src, size_t n)
{
	const char *p = src;
	char *q = dst;
#if defined(__i386__)
	size_t nl = n >> 2;
	__asm__ __volatile__ ("cld ; rep ; movsl ; movl %3,%0 ; rep ; movsb":"+c" (nl),
		      "+S"(p), "+D"(q)
		      :"r"(n & 3));
#elif defined(__x86_64__)
	size_t nq = n >> 3;
	__asm__ __volatile__ ("cld ; rep ; movsq ; movl %3,%%ecx ; rep ; movsb":"+c"
		      (nq), "+S"(p), "+D"(q)
		      :"r"((uint32_t) (n & 7)));
#else
	while (n--) {
		*q++ = *p++;
	}
#endif

	return dst;
}
/*
 * memmove.c
 */

#include <string.h>

void *memmove(void *dst, const void *src, size_t n)
{
	const char *p = src;
	char *q = dst;
#if defined(__i386__) || defined(__x86_64__)
	if (q < p) {
		__asm__ __volatile__("cld ; rep ; movsb"
			     : "+c" (n), "+S"(p), "+D"(q));
	} else {
		p += (n - 1);
		q += (n - 1);
		__asm__ __volatile__("std ; rep ; movsb"
			     : "+c" (n), "+S"(p), "+D"(q));
	}
#else
	if (q < p) {
		while (n--) {
			*q++ = *p++;
		}
	} else {
		p += n;
		q += n;
		while (n--) {
			*--q = *--p;
		}
	}
#endif

	return dst;
}
/*
 * memset.c
 */

#include <string.h>

void *memset(void *dst, int c, size_t n)
{
	char *q = dst;

#if defined(__i386__)
	size_t nl = n >> 2;
	__asm__ __volatile__ ("cld ; rep ; stosl ; movl %3,%0 ; rep ; stosb"
		      : "+c" (nl), "+D" (q)
		      : "a" ((unsigned char)c * 0x01010101U), "r" (n & 3));
#elif defined(__x86_64__)
	size_t nq = n >> 3;
	__asm__ __volatile__ ("cld ; rep ; stosq ; movl %3,%%ecx ; rep ; stosb"
		      :"+c" (nq), "+D" (q)
		      : "a" ((unsigned char)c * 0x0101010101010101U),
			"r" ((uint32_t) n & 7));
#else
	while (n--) {
		*q++ = c;
	}
#endif

	return dst;
}
/*
 * strpbrk
 */

#include <string.h>
#include <stddef.h>

size_t __strxspn(const char *s, const char *map, int parity)
{
	char matchmap[UCHAR_MAX + 1];
	size_t n = 0;

	/* Create bitmap */
	memset(matchmap, 0, sizeof matchmap);
	while (*map)
		matchmap[(unsigned char)*map++] = 1;

	/* Make sure the null character never matches */
	matchmap[0] = parity;

	/* Calculate span length */
	while (matchmap[(unsigned char)*s++] ^ parity)
		n++;

	return n;
}
/*
 * memmem.c
 *
 * Find a byte string inside a longer byte string
 *
 * This uses the "Not So Naive" algorithm, a very simple but
 * usually effective algorithm, see:
 *
 * http://www-igm.univ-mlv.fr/~lecroq/string/
 */

#include <string.h>

void *memmem(const void *haystack, size_t n, const void *needle, size_t m)
{
	const unsigned char *y = (const unsigned char *)haystack;
	const unsigned char *x = (const unsigned char *)needle;

	size_t j, k, l;

	if (m > n || !m || !n)
		return NULL;

	if (1 != m) {
		if (x[0] == x[1]) {
			k = 2;
			l = 1;
		} else {
			k = 1;
			l = 2;
		}

		j = 0;
		while (j <= n - m) {
			if (x[1] != y[j + 1]) {
				j += k;
			} else {
				if (!memcmp(x + 2, y + j + 2, m - 2)
				    && x[0] == y[j])
					return (void *)&y[j];
				j += l;
			}
		}
	} else
		do {
			if (*y == *x)
				return (void *)y;
			y++;
		} while (--n);

	return NULL;
}
/*
 * memswap()
 *
 * Swaps the contents of two nonoverlapping memory areas.
 * This really could be done faster...
 */

#include <string.h>

void memswap(void *m1, void *m2, size_t n)
{
	char *p = m1;
	char *q = m2;
	char tmp;

	while (n--) {
		tmp = *p;
		*p = *q;
		*q = tmp;

		p++;
		q++;
	}
}
/*
 * strcat.c
 */

#include <string.h>

char *strcat(char *dst, const char *src)
{
	strcpy(strchr(dst, '\0'), src);
	return dst;
}
/*
 * strchr.c
 */

#include <string.h>

char *strchr(const char *s, int c)
{
	while (*s != (char)c) {
		if (!*s)
			return NULL;
		s++;
	}

	return (char *)s;
}

/*
 * strrchr.c
 */

#include <string.h>

char *strrchr(const char *s, int c)
{
	const char *found = NULL;

	while (*s) {
		if (*s == (char)c)
			found = s;
		s++;
	}

	return (char *)found;
}

/*
 * strcmp.c
 */

#include <string.h>

int strcmp(const char *s1, const char *s2)
{
	const unsigned char *c1 = (const unsigned char *)s1;
	const unsigned char *c2 = (const unsigned char *)s2;
	unsigned char ch;
	int d = 0;

	while (1) {
		d = (int)(ch = *c1++) - (int)*c2++;
		if (d || !ch)
			break;
	}

	return d;
}
/*
 * strcpy.c
 *
 * strcpy()
 */

#include <string.h>

char *strcpy(char *dst, const char *src)
{
	char *q = dst;
	const char *p = src;
	char ch;

	do {
		*q++ = ch = *p++;
	} while (ch);

	return dst;
}
/*
 * strcspn
 */

size_t strcspn(const char *s, const char *reject)
{
	return __strxspn(s, reject, 1);
}
/*
 * strlen()
 */

#include <string.h>

size_t strlen(const char *s)
{
	const char *ss = s;
	while (*ss)
		ss++;
	return ss - s;
}
/*
 * strnlen()
 */

#include <string.h>

size_t strnlen(const char *s, size_t maxlen)
{
	const char *ss = s;

	/* Important: the maxlen test must precede the reference through ss;
	   since the byte beyond the maximum may segfault */
	while ((maxlen > 0) && *ss) {
		ss++;
		maxlen--;
	}
	return ss - s;
}
/*
 * strncat.c
 */

#include <string.h>

char *strncat(char *dst, const char *src, size_t n)
{
	char *q = strchr(dst, '\0');
	const char *p = src;
	char ch;

	while (n--) {
		*q++ = ch = *p++;
		if (!ch)
			return dst;
	}
	*q = '\0';

	return dst;
}
/*
 * strlcat.c
 */

#include <string.h>

size_t strlcat(char *dst, const char *src, size_t size)
{
	size_t bytes = 0;
	char *q = dst;
	const char *p = src;
	char ch;

	while (bytes < size && *q) {
		q++;
		bytes++;
	}
	if (bytes == size)
		return (bytes + strlen(src));

	while ((ch = *p++)) {
		if (bytes + 1 < size)
			*q++ = ch;

		bytes++;
	}

	*q = '\0';
	return bytes;
}
/*
 * strncmp.c
 */

#include <string.h>

int strncmp(const char *s1, const char *s2, size_t n)
{
	const unsigned char *c1 = (const unsigned char *)s1;
	const unsigned char *c2 = (const unsigned char *)s2;
	unsigned char ch;
	int d = 0;

	while (n--) {
		d = (int)(ch = *c1++) - (int)*c2++;
		if (d || !ch)
			break;
	}

	return d;
}
/*
 * strncpy.c
 */

#include <string.h>

char *strncpy(char *dst, const char *src, size_t n)
{
	char *q = dst;
	const char *p = src;
	char ch;

	while (n) {
		n--;
		*q++ = ch = *p++;
		if (!ch)
			break;
	}

	/* The specs say strncpy() fills the entire buffer with NUL.  Sigh. */
	memset(q, 0, n);

	return dst;
}
/*
 * strpbrk
 */

char *strpbrk(const char *s, const char *accept)
{
	const char *ss = s + __strxspn(s, accept, 1);

	return *ss ? (char *)ss : NULL;
}
/*
 * strsep.c
 */

#include <string.h>

char *strsep(char **stringp, const char *delim)
{
	char *s = *stringp;
	char *e;

	if (!s)
		return NULL;

	e = strpbrk(s, delim);
	if (e)
		*e++ = '\0';

	*stringp = e;
	return s;
}
/*
 * strspn
 */

size_t strspn(const char *s, const char *accept)
{
	return __strxspn(s, accept, 0);
}
/*
 * strstr.c
 */

#include <string.h>

char *strstr(const char *haystack, const char *needle)
{
	return (char *)memmem(haystack, strlen(haystack), needle,
			      strlen(needle));
}
/*
 * strtok.c
 */

#include <string.h>

char *strtok(char *s, const char *delim)
{
	static char *holder;

	if (s)
		holder = s;

	return strsep(&holder, delim);
}
