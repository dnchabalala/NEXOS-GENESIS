#ifndef NEXOS_TLS_STDIO_H
#define NEXOS_TLS_STDIO_H
#include <stddef.h>
typedef struct nexos_tls_FILE FILE;
int snprintf(char *, size_t, const char *, ...);
int vsnprintf(char *, size_t, const char *, __builtin_va_list);
int printf(const char *, ...);
#endif
