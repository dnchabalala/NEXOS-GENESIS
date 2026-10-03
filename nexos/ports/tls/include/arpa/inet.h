#ifndef NEXOS_ARPA_INET_H
#define NEXOS_ARPA_INET_H

#include <netinet/in.h>

int inet_aton(const char *, struct in_addr *);
int inet_pton(int, const char *, void *);

#endif
