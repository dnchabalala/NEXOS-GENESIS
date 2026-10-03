#ifndef NEXOS_TLS_STDLIB_H
#define NEXOS_TLS_STDLIB_H
#include <stddef.h>
int atoi(const char *s);
long strtol(const char *s, char **end, int base);
long long strtoll(const char *s, char **end, int base);
unsigned long strtoul(const char *s, char **end, int base);
unsigned long long strtoull(const char *s, char **end, int base);
double strtod(const char *s, char **end);
float strtof(const char *s, char **end);
void *bsearch(const void *key, const void *base, size_t count, size_t size,
              int (*compare)(const void *, const void *));
void *malloc(size_t size);
void *calloc(size_t count, size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);
void abort(void);
int abs(int);
int rand(void);
#define RAND_MAX 0x7fffffff
int atexit(void (*)(void));
#endif
