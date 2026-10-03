#ifndef NEXOS_ERRNO_H
#define NEXOS_ERRNO_H

/* The first native NetSurf runtime is single-context. Keep errno as one
 * kernel runtime value until per-process TLS exists. */
extern int nexos_errno;
#define errno nexos_errno

#define EINVAL 22
#define E2BIG 7
#define EBADF 9
#define EIO 5
#define ENOENT 2
#define ENOSYS 38
#define ENOMEM 12
#define ENOSPC 28
#define ENOTDIR 20
#define EISDIR 21
#define EEXIST 17
#define ENOTEMPTY 39
#define ENOTSUP 95
#define ETIMEDOUT 110
#define ECONNREFUSED 111
#define EAGAIN 11
#define EWOULDBLOCK EAGAIN
#define EINTR 4
#define EPIPE 32
#define ECONNRESET 104
#define ERANGE 34
#define EILSEQ 84
const char *strerror(int);

#endif
