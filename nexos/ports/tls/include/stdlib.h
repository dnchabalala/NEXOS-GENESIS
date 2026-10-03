#ifndef NEXOS_TLS_STDLIB_H
#define NEXOS_TLS_STDLIB_H
#include <stddef.h>
void *malloc(size_t size);
void *calloc(size_t count, size_t size);
void free(void *ptr);
void abort(void);
#endif
