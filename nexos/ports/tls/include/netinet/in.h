#ifndef NEXOS_NETINET_IN_H
#define NEXOS_NETINET_IN_H

#include <stdint.h>

#define AF_INET 2
#define AF_INET6 10
#define PF_INET AF_INET
#define INET_ADDRSTRLEN 16

struct in_addr { uint32_t s_addr; };
struct in6_addr { unsigned char s6_addr[16]; };
struct sockaddr { unsigned short sa_family; char sa_data[14]; };
struct sockaddr_in {
    unsigned short sin_family;
    unsigned short sin_port;
    struct in_addr sin_addr;
    unsigned char sin_zero[8];
};

#endif
