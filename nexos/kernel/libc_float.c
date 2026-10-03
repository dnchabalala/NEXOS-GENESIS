/* NexOS — kernel/libc_float.c | floating-point libc adapters | MIT License */
#include <stddef.h>
#include <stdlib.h>

double strtod(const char *s, char **end)
{
    const char *p = s;
    int negative = 0;
    double value = 0.0, scale = 0.1;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (*p == '-' || *p == '+') { negative = (*p == '-'); p++; }
    const char *digits = p;
    while (*p >= '0' && *p <= '9') value = value * 10.0 + (*p++ - '0');
    if (*p == '.') {
        p++;
        while (*p >= '0' && *p <= '9') {
            value += scale * (*p++ - '0');
            scale *= 0.1;
        }
    }
    if (p == digits) { if (end) *end = (char *)s; return 0.0; }
    if (*p == 'e' || *p == 'E') {
        int exp = atoi(p + 1);
        double factor = 1.0;
        if (exp > 0) while (exp--) factor *= 10.0;
        else while (exp++) factor *= 0.1;
        value *= factor;
        while (*p && *p != ' ') p++;
    }
    if (end) *end = (char *)p;
    return negative ? -value : value;
}

float strtof(const char *s, char **end)
{
    return (float)strtod(s, end);
}
