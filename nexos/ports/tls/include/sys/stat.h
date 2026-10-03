#ifndef NEXOS_SYS_STAT_H
#define NEXOS_SYS_STAT_H
#include <stdint.h>
struct stat { uint32_t st_mode; uint64_t st_size; };
#define S_IFMT 0170000
#define S_IFDIR 0040000
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
int stat(const char *, struct stat *);
#endif
