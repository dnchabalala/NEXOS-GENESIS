/* NexOS — kernel/libc.c | freestanding C library entry points | MIT License */

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <sys/utsname.h>
#include <iconv.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "drivers/rtc.h"
#include "drivers/timer.h"

#include "mm/heap.h"

typedef struct nexos_tls_FILE FILE;

static uint32_t nexos_rand_state = 0x6d2b79f5u;
int rand(void)
{
    nexos_rand_state = nexos_rand_state * 1664525u + 1013904223u;
    return (int)(nexos_rand_state & 0x7fffffffU);
}

iconv_t iconv_open(const char *to, const char *from) {
    (void)to; (void)from; return (iconv_t)-1;
}
size_t iconv(iconv_t cd, char **in, size_t *in_left,
             char **out, size_t *out_left) {
    (void)cd; (void)in; (void)in_left; (void)out; (void)out_left;
    return (size_t)-1;
}
int iconv_close(iconv_t cd) { (void)cd; return 0; }

static int64_t nexos_rtc_epoch(void)
{
    rtc_time_t t;
    rtc_get_time(&t);
    uint32_t y = t.year;
    if (y < 1970) return 0;
    uint64_t dy = y - 1970;
    static const uint16_t mdays[12] =
        {0,31,59,90,120,151,181,212,243,273,304,334};
    uint8_t month = t.month > 0 ? t.month - 1 : 0;
    if (month > 11) month = 11;
    return (int64_t)(dy * 365 * 86400 + (dy / 4) * 86400 +
        (uint64_t)mdays[month] * 86400 +
        (uint64_t)(t.day > 0 ? t.day - 1 : 0) * 86400 +
        (uint64_t)t.hour * 3600 + (uint64_t)t.minute * 60 + t.second);
}

time_t time(time_t *value)
{
    time_t now = (time_t)nexos_rtc_epoch();
    if (value) *value = now;
    return now;
}

struct tm *gmtime_r(const time_t *value, struct tm *out)
{
    static const int mdays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    uint64_t seconds = value ? (uint64_t)*value : 0;
    uint64_t days = seconds / 86400;
    uint32_t year = 1970;
    while (days >= 365 + (((year % 4) == 0 && (year % 100) != 0) ||
                          (year % 400) == 0)) {
        days -= 365 + (((year % 4) == 0 && (year % 100) != 0) ||
                        (year % 400) == 0);
        year++;
    }
    int month = 0;
    while (month < 12) {
        int length = mdays[month] + (month == 1 &&
            (((year % 4) == 0 && (year % 100) != 0) || (year % 400) == 0));
        if (days < (uint64_t)length) break;
        days -= length; month++;
    }
    out->tm_year = (int)year - 1900; out->tm_mon = month;
    out->tm_mday = (int)days + 1; out->tm_hour = (int)((seconds / 3600) % 24);
    out->tm_min = (int)((seconds / 60) % 60); out->tm_sec = (int)(seconds % 60);
    out->tm_wday = 0; out->tm_yday = 0; out->tm_isdst = 0;
    return out;
}

struct tm *gmtime(const time_t *value) { static struct tm out; return gmtime_r(value, &out); }
struct tm *localtime(const time_t *value) { return gmtime(value); }

size_t strftime(char *out, size_t size, const char *format, const struct tm *tm)
{
    size_t n = 0;
    for (size_t i = 0; format[i] && n + 1 < size; i++) {
        if (format[i] != '%') { out[n++] = format[i]; continue; }
        char temp[16]; int written = 0; char spec = format[++i];
        if (spec == 'Y') written = snprintf(temp, sizeof(temp), "%04d", tm->tm_year + 1900);
        else if (spec == 'm') written = snprintf(temp, sizeof(temp), "%02d", tm->tm_mon + 1);
        else if (spec == 'd') written = snprintf(temp, sizeof(temp), "%02d", tm->tm_mday);
        else if (spec == 'H') written = snprintf(temp, sizeof(temp), "%02d", tm->tm_hour);
        else if (spec == 'M') written = snprintf(temp, sizeof(temp), "%02d", tm->tm_min);
        else if (spec == 'S') written = snprintf(temp, sizeof(temp), "%02d", tm->tm_sec);
        else if (spec == '%') temp[written++] = '%';
        else continue;
        for (int j = 0; j < written && n + 1 < size; j++) out[n++] = temp[j];
    }
    if (size) out[n] = 0;
    return n;
}

time_t mktime(struct tm *tm) { (void)tm; return (time_t)-1; }
char *strptime(const char *input, const char *format, struct tm *tm) {
    (void)input; (void)format; (void)tm; return NULL;
}

int gettimeofday(struct timeval *tv, void *tz)
{
    (void)tz;
    if (!tv) return -1;
    tv->tv_sec = nexos_rtc_epoch();
    tv->tv_usec = (int64_t)((timer_get_ticks() % 1000) * 1000);
    return 0;
}

const char *strerror(int error) { (void)error; return "NexOS error"; }
int stat(const char *path, struct stat *st) { (void)path; (void)st; return -1; }
int unlink(const char *path) { (void)path; return -1; }
int access(const char *path, int mode) { (void)path; (void)mode; return -1; }
DIR *opendir(const char *path) { (void)path; return NULL; }
struct dirent *readdir(DIR *dir) { (void)dir; return NULL; }
int closedir(DIR *dir) { (void)dir; return -1; }

int uname(struct utsname *name)
{
    if (!name) return -1;
    const char *values[5] = {"NexOS", "nexos", "0.1", "freestanding", "x86_64"};
    char *fields[5] = {name->sysname, name->nodename, name->release,
                       name->version, name->machine};
    for (int i = 0; i < 5; i++) {
        int j = 0;
        while (values[i][j] && j < 64) { fields[i][j] = values[i][j]; j++; }
        fields[i][j] = 0;
    }
    return 0;
}

int inet_aton(const char *text, struct in_addr *out)
{
    unsigned part[4] = {0, 0, 0, 0};
    int count = 0;
    const char *p = text;
    while (*p && count < 4) {
        unsigned v = 0;
        int digits = 0;
        while (*p >= '0' && *p <= '9') {
            v = v * 10 + (unsigned)(*p - '0');
            p++; digits++;
        }
        if (!digits || v > 255) return 0;
        part[count++] = v;
        if (*p == '.') p++;
        else if (*p) return 0;
    }
    if (*p || count != 4) return 0;
    out->s_addr = ((uint32_t)part[0] << 24) |
                  ((uint32_t)part[1] << 16) |
                  ((uint32_t)part[2] << 8) | (uint32_t)part[3];
    return 1;
}

int inet_pton(int family, const char *text, void *dst)
{
    if (family != AF_INET) return 0;
    return inet_aton(text, (struct in_addr *)dst);
}

FILE *fopen(const char *path, const char *mode) {
    (void)path; (void)mode; return NULL;
}
int fclose(FILE *stream) { (void)stream; return -1; }
char *fgets(char *buf, int size, FILE *stream) {
    (void)buf; (void)size; (void)stream; return NULL;
}
int fseek(FILE *stream, long offset, int origin) {
    (void)stream; (void)offset; (void)origin; return -1;
}
long ftell(FILE *stream) { (void)stream; return -1; }
size_t fread(void *ptr, size_t size, size_t count, FILE *stream) {
    (void)ptr; (void)size; (void)count; (void)stream; return 0;
}
int feof(FILE *stream) { (void)stream; return 1; }
int fprintf(FILE *stream, const char *format, ...) {
    (void)stream; (void)format; return -1;
}
int vfprintf(FILE *stream, const char *format, va_list ap) {
    (void)stream; (void)format; (void)ap; return -1;
}
int fputc(int c, FILE *stream) { (void)c; (void)stream; return -1; }
int fputs(const char *s, FILE *stream) { (void)s; (void)stream; return -1; }
int fflush(FILE *stream) { (void)stream; return 0; }
int atexit(void (*function)(void)) { (void)function; return 0; }

int atoi(const char *s)
{
    int sign = 1;
    int value = 0;

    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9') {
        value = value * 10 + (*s - '0');
        s++;
    }
    return sign * value;
}

int vsnprintf(char *out, size_t size, const char *fmt, va_list ap)
{
    size_t pos = 0;
    while (*fmt) {
        if (*fmt != '%') { if (pos + 1 < size) out[pos] = *fmt; pos++; fmt++; continue; }
        fmt++;
        if (*fmt == '%') { if (pos + 1 < size) out[pos] = '%'; pos++; fmt++; continue; }
        int width = 0, precision = -1, long_value = 0, size_value = 0, zero = 0;
        if (*fmt == '0') { zero = 1; fmt++; }
        while (*fmt >= '0' && *fmt <= '9') { width = width * 10 + (*fmt++ - '0'); }
        if (*fmt == '.' && fmt[1] == '*') {
            precision = va_arg(ap, int);
            fmt += 2;
        }
        if (*fmt == 'z') { size_value = 1; fmt++; }
        else if (*fmt == 'l') { long_value = 1; fmt++; if (*fmt == 'l') fmt++; }
        char buf[32]; int n = 0; unsigned long long value = 0; int negative = 0;
        if (*fmt == 's') {
            const char *s = va_arg(ap, const char *); if (!s) s = "(null)";
            int count = 0;
            while (*s && (precision < 0 || count < precision)) {
                if (pos + 1 < size) out[pos] = *s;
                pos++; s++; count++;
            }
            fmt++; continue;
        } else if (*fmt == 'c') {
            if (pos + 1 < size) out[pos] = (char)va_arg(ap, int); pos++; fmt++; continue;
        } else if (*fmt == 'p') {
            value = (unsigned long long)(uintptr_t)va_arg(ap, void *); buf[n++] = '0'; buf[n++] = 'x';
            unsigned long long v = value; char digits[16]; int d = 0; do { digits[d++] = "0123456789abcdef"[v & 15]; v >>= 4; } while (v);
            while (d) buf[n++] = digits[--d]; fmt++;
        } else if (*fmt == 'd' || *fmt == 'i') {
            long long v = long_value ? va_arg(ap, long) : va_arg(ap, int); if (v < 0) { negative = 1; value = (unsigned long long)-v; } else value = (unsigned long long)v;
            fmt++;
        } else if (*fmt == 'u' || *fmt == 'x' || *fmt == 'X') {
            value = size_value ? va_arg(ap, size_t) : (long_value ? va_arg(ap, unsigned long) : va_arg(ap, unsigned));
            const char *digits = (*fmt == 'X') ? "0123456789ABCDEF" : "0123456789abcdef"; int base = (*fmt == 'u') ? 10 : 16; int d = 0; do { buf[d++] = digits[value % base]; value /= base; } while (value); n = d; while (d < n) { char c = buf[d - 1]; buf[d - 1] = buf[n - d]; buf[n - d] = c; d--; } fmt++;
        } else { if (pos + 1 < size) out[pos] = '%'; pos++; continue; }
        if (negative) { if (pos + 1 < size) out[pos] = '-'; pos++; }
        int pad = width - n - (negative ? 1 : 0); while (pad-- > 0) { if (pos + 1 < size) out[pos] = zero ? '0' : ' '; pos++; }
        for (int i = 0; i < n; i++) { if (pos + 1 < size) out[pos] = buf[i]; pos++; }
    }
    if (size) out[pos < size ? pos : size - 1] = 0;
    return (int)pos;
}

int snprintf(char *out, size_t size, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt); int n = vsnprintf(out, size, fmt, ap); va_end(ap); return n;
}

static unsigned long long parse_integer(const char *s, char **end,
                                        int base, int is_signed,
                                        int long_long)
{
    const char *p = s;
    unsigned long long value = 0;
    int negative = 0;
    if (base == 0) {
        base = 10;
        if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
            base = 16; p += 2;
        } else if (p[0] == '0') {
            base = 8;
        }
    }
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (*p == '-' || *p == '+') {
        negative = (*p == '-');
        p++;
    }
    const char *digits = p;
    while (*p) {
        unsigned d;
        if (*p >= '0' && *p <= '9') d = (unsigned)(*p - '0');
        else if (*p >= 'a' && *p <= 'z') d = (unsigned)(*p - 'a' + 10);
        else if (*p >= 'A' && *p <= 'Z') d = (unsigned)(*p - 'A' + 10);
        else break;
        if (d >= (unsigned)base) break;
        value = value * (unsigned)base + d;
        p++;
    }
    if (end) *end = (char *)(p == digits ? s : p);
    if (negative) value = (unsigned long long)(-(long long)value);
    (void)is_signed;
    (void)long_long;
    return value;
}

long strtol(const char *s, char **end, int base)
{
    return (long)parse_integer(s, end, base, 1, 0);
}

long long strtoll(const char *s, char **end, int base)
{
    return (long long)parse_integer(s, end, base, 1, 1);
}

unsigned long strtoul(const char *s, char **end, int base)
{
    return (unsigned long)parse_integer(s, end, base, 0, 0);
}

unsigned long long strtoull(const char *s, char **end, int base)
{
    return parse_integer(s, end, base, 0, 1);
}

void *bsearch(const void *key, const void *base, size_t count, size_t size,
              int (*compare)(const void *, const void *))
{
    size_t first = 0;
    while (first < count) {
        size_t middle = first + (count - first) / 2;
        const void *item = (const uint8_t *)base + middle * size;
        int result = compare(key, item);
        if (result == 0) return (void *)item;
        if (result < 0) count = middle;
        else first = middle + 1;
    }
    return NULL;
}

void *malloc(size_t size)
{
    return kmalloc(size);
}

void *calloc(size_t count, size_t size)
{
    if (size && count > (size_t)-1 / size) return NULL;
    size_t total = count * size;
    uint8_t *p = (uint8_t *)kmalloc(total ? total : 1);
    if (!p) return NULL;
    for (size_t i = 0; i < total; i++) p[i] = 0;
    return p;
}

void free(void *ptr)
{
    kfree(ptr);
}

void *realloc(void *ptr, size_t size)
{
    return krealloc(ptr, size);
}

void abort(void)
{
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

int abs(int value) { return value < 0 ? -value : value; }

static int scan_space(const char **input)
{
    int n = 0;
    while (**input == ' ' || **input == '\t' || **input == '\n' ||
           **input == '\r' || **input == '\f' || **input == '\v') {
        (*input)++;
        n++;
    }
    return n;
}

static int scan_unsigned(const char **input, unsigned base, int width,
                         unsigned long long *value)
{
    unsigned long long v = 0;
    int digits = 0;
    while (**input && (width <= 0 || digits < width)) {
        unsigned d;
        char c = **input;
        if (c >= '0' && c <= '9') d = (unsigned)(c - '0');
        else if (c >= 'a' && c <= 'f') d = (unsigned)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') d = (unsigned)(c - 'A' + 10);
        else break;
        if (d >= base) break;
        v = v * base + d;
        (*input)++;
        digits++;
    }
    *value = v;
    return digits;
}

int sscanf(const char *input, const char *format, ...)
{
    va_list ap;
    int assigned = 0;
    va_start(ap, format);

    while (*format) {
        if (*format != '%') {
            if (*format == ' ' || *format == '\t' || *format == '\n') {
                scan_space(&input);
                format++;
                continue;
            }
            if (*input != *format) break;
            input++;
            format++;
            continue;
        }

        format++;
        if (*format == '%') {
            if (*input != '%') break;
            input++; format++; continue;
        }

        int width = 0;
        while (*format >= '0' && *format <= '9') {
            width = width * 10 + (*format - '0');
            format++;
        }
        int size_modifier = 0; /* 1 = z, 2 = l */
        if (*format == 'z') { size_modifier = 1; format++; }
        else if (*format == 'l') { size_modifier = 2; format++; }

        if (*format == 'c') {
            char *out = va_arg(ap, char *);
            if (!*input) break;
            *out = *input++;
            assigned++;
            format++;
            continue;
        }

        scan_space(&input);
        int negative = 0;
        if ((*format == 'd' || *format == 'i') &&
            (*input == '-' || *input == '+')) {
            negative = (*input == '-');
            input++;
        }
        unsigned base = (*format == 'x' || *format == 'X') ? 16 : 10;
        unsigned long long value = 0;
        int digits = scan_unsigned(&input, base, width, &value);
        if (!digits) break;
        if (negative) value = (unsigned long long)(-(long long)value);

        switch (*format) {
        case 'd': case 'i':
            if (size_modifier == 1) *va_arg(ap, long *) = (long)value;
            else if (size_modifier == 2) *va_arg(ap, long *) = (long)value;
            else *va_arg(ap, int *) = (int)value;
            assigned++;
            break;
        case 'u':
            if (size_modifier == 1) *va_arg(ap, size_t *) = (size_t)value;
            else if (size_modifier == 2) *va_arg(ap, unsigned long *) = (unsigned long)value;
            else *va_arg(ap, unsigned *) = (unsigned)value;
            assigned++;
            break;
        case 'x': case 'X':
            if (size_modifier == 1) *va_arg(ap, size_t *) = (size_t)value;
            else if (size_modifier == 2) *va_arg(ap, unsigned long *) = (unsigned long)value;
            else *va_arg(ap, unsigned *) = (unsigned)value;
            assigned++;
            break;
        default:
            va_end(ap);
            return assigned;
        }
        format++;
    }

    va_end(ap);
    return assigned;
}
