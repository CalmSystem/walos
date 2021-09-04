#ifndef __STDLIB_H
#define __STDLIB_H

#include <stddef.h>

int atoi(const char *);
long atol(const char *);
long long atoll(const char *);
double atof(const char *);

float strtof(const char *, char **);
double strtod(const char *, char **);
long double strtold(const char *, char **);

long strtol(const char *, char **, int);
unsigned long strtoul(const char *, char **, int);
long long strtoll(const char *, char **, int);
unsigned long long strtoull(const char *, char **, int);

void abort(void);

void* malloc(size_t size) __attribute__((malloc, alloc_size(1)));
void* calloc(size_t num, size_t size) __attribute__((alloc_size(1, 2)));
void* realloc(void* ptr, size_t size) __attribute__((alloc_size(2)));
void free(void* ptr);

int rand(void);

#endif
