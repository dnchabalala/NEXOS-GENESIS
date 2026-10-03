#ifndef NEXOS_SYS_SOCKET_H
#define NEXOS_SYS_SOCKET_H

#include <stddef.h>
#include <sys/select.h>
#include <netinet/in.h>

#define SOCK_STREAM 1
#define SOCK_DGRAM 2

typedef unsigned int socklen_t;
int socket(int, int, int);
int connect(int, const struct sockaddr *, socklen_t);
int close(int);

#endif
