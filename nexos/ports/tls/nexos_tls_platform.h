#ifndef NEXOS_TLS_PLATFORM_H
#define NEXOS_TLS_PLATFORM_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

void *nexos_tls_calloc(size_t count, size_t size);
void nexos_tls_free_mem(void *ptr);
int nexos_tls_snprintf(char *buf, size_t size, const char *fmt, ...);
int nexos_tls_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
int nexos_tls_printf(const char *fmt, ...);
int64_t nexos_tls_time(int64_t *out);
int64_t mbedtls_ms_time(void);
struct tm *mbedtls_platform_gmtime_r(const int64_t *value, struct tm *out);

#endif
