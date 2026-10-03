#ifndef NEXOS_TLS_TIME_H
#define NEXOS_TLS_TIME_H
#include <stdint.h>
typedef int64_t time_t;
struct tm { int tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year,
                   tm_wday, tm_yday, tm_isdst; };
time_t time(time_t *value);
struct tm *gmtime(const time_t *value);
struct tm *gmtime_r(const time_t *value, struct tm *result);
struct tm *localtime(const time_t *value);
size_t strftime(char *, size_t, const char *, const struct tm *);
time_t mktime(struct tm *);
char *strptime(const char *, const char *, struct tm *);
#endif
