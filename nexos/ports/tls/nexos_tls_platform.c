/* Freestanding Mbed TLS platform services backed by existing NexOS APIs. */
#include "nexos_tls_platform.h"
#include "../../kernel/mm/heap.h"
#include "../../kernel/drivers/rtc.h"
#include "../../kernel/drivers/timer.h"

void *nexos_tls_calloc(size_t count, size_t size) {
    if (size && count > (size_t)-1 / size) return 0;
    size_t total = count * size;
    unsigned char *p = (unsigned char *)kmalloc(total ? total : 1);
    if (!p) return 0;
    for (size_t i = 0; i < total; ++i) p[i] = 0;
    return p;
}

void nexos_tls_free_mem(void *ptr) { kfree(ptr); }

static int emit(char *buf, size_t size, int *written, char c) {
    if (size && (size_t)*written + 1 < size) buf[*written] = c;
    (*written)++;
    return 0;
}

static void emit_unsigned(char *buf, size_t size, int *written,
                          unsigned long long value, unsigned base,
                          int upper, int width, char pad) {
    char digits[32]; int n = 0;
    const char *hex = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    if (!value) digits[n++] = '0';
    while (value) { digits[n++] = hex[value % base]; value /= base; }
    while (n < width) emit(buf, size, written, pad), --width;
    while (n) emit(buf, size, written, digits[--n]);
}

int nexos_tls_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap) {
    int written = 0;
    while (*fmt) {
        if (*fmt != '%') { emit(buf, size, &written, *fmt++); continue; }
        ++fmt;
        if (*fmt == '%') { emit(buf, size, &written, *fmt++); continue; }
        char pad = ' '; int width = 0; int long_count = 0;
        if (*fmt == '0') { pad = '0'; ++fmt; }
        while (*fmt >= '0' && *fmt <= '9') width = width * 10 + *fmt++ - '0';
        while (*fmt == 'l') { ++long_count; ++fmt; }
        if (*fmt == 'z') { ++fmt; long_count = 1; }
        switch (*fmt++) {
        case 'd': case 'i': {
            long long v = long_count ? va_arg(ap, long long) : va_arg(ap, int);
            if (v < 0) { emit(buf, size, &written, '-'); v = -v; }
            emit_unsigned(buf, size, &written, (unsigned long long)v, 10, 0, width, pad);
            break;
        }
        case 'u': emit_unsigned(buf, size, &written,
            long_count ? va_arg(ap, unsigned long long) : va_arg(ap, unsigned), 10, 0, width, pad); break;
        case 'x': case 'X': emit_unsigned(buf, size, &written,
            long_count ? va_arg(ap, unsigned long long) : va_arg(ap, unsigned), 16, *(fmt - 1) == 'X', width, pad); break;
        case 's': { const char *s = va_arg(ap, const char *); if (!s) s = "(null)"; while (*s) emit(buf, size, &written, *s++); break; }
        case 'c': emit(buf, size, &written, (char)va_arg(ap, int)); break;
        default: emit(buf, size, &written, '?'); break;
        }
    }
    if (size) {
        size_t end = written < (int)size ? (size_t)written : size - 1;
        buf[end] = 0;
    }
    return written;
}

int nexos_tls_snprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int n = nexos_tls_vsnprintf(buf, size, fmt, ap);
    va_end(ap); return n;
}

int nexos_tls_printf(const char *fmt, ...) {
    (void)fmt;
    return 0;
}

static int leap(int y) { return (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)); }

int64_t nexos_tls_time(int64_t *out) {
    rtc_time_t t; rtc_get_time(&t);
    int64_t days = 0;
    for (int y = 1970; y < t.year; ++y) days += leap(y) ? 366 : 365;
    static const int mdays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    for (int m = 1; m < t.month; ++m) days += mdays[m - 1] + (m == 2 && leap(t.year));
    days += t.day > 0 ? t.day - 1 : 0;
    int64_t value = days * 86400 + t.hour * 3600 + t.minute * 60 + t.second;
    if (out) *out = value;
    return value;
}

int64_t mbedtls_ms_time(void) { return (int64_t)timer_get_ticks(); }

struct tm *mbedtls_platform_gmtime_r(const int64_t *value, struct tm *out) {
    if (!value || !out) return 0;
    int64_t days = *value / 86400, rem = *value % 86400;
    if (rem < 0) rem += 86400, --days;
    int year = 1970;
    while (days >= (leap(year) ? 366 : 365)) days -= leap(year) ? 366 : 365, ++year;
    static const int mdays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int month = 0;
    while (month < 11 && days >= mdays[month] + (month == 1 && leap(year)))
        days -= mdays[month] + (month == 1 && leap(year)), ++month;
    out->tm_sec = rem % 60; out->tm_min = (rem / 60) % 60; out->tm_hour = rem / 3600;
    out->tm_mday = days + 1; out->tm_mon = month; out->tm_year = year - 1900;
    out->tm_wday = 0; out->tm_yday = 0; out->tm_isdst = 0;
    return out;
}
