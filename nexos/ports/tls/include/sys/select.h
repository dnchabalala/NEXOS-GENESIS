#ifndef NEXOS_SYS_SELECT_H
#define NEXOS_SYS_SELECT_H

#include <stdint.h>

typedef struct { uint64_t bits[16]; } fd_set;
#define FD_ZERO(s) do { for (unsigned _i = 0; _i < 16; _i++) (s)->bits[_i] = 0; } while (0)
#define FD_SET(fd, s) ((s)->bits[(unsigned)(fd) >> 6] |= (1ULL << ((unsigned)(fd) & 63)))
#define FD_CLR(fd, s) ((s)->bits[(unsigned)(fd) >> 6] &= ~(1ULL << ((unsigned)(fd) & 63)))
#define FD_ISSET(fd, s) (((s)->bits[(unsigned)(fd) >> 6] >> ((unsigned)(fd) & 63)) & 1U)

#endif
