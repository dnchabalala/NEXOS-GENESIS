#ifndef NEXOS_TLS_TCP_H
#define NEXOS_TLS_TCP_H

#include "nexos_tls.h"
#include "../../kernel/net/tcp.h"

void nexos_tls_tcp_io(nexos_tls_io_t *io, tcp_conn_t *conn);
int nexos_tls_entropy_rdrand(void *opaque, unsigned char *buf, size_t len);

#endif
