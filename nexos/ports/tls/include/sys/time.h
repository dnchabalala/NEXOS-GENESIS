#ifndef NEXOS_SYS_TIME_H
#define NEXOS_SYS_TIME_H
#include <stdint.h>
struct timeval { int64_t tv_sec; int64_t tv_usec; };
int gettimeofday(struct timeval *, void *);
#endif
