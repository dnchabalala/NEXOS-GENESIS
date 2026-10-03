/* mbed TLS callbacks backed directly by the existing NexOS TCP connection. */
#include "nexos_tls_tcp.h"
#include "../../kernel/drivers/timer.h"

static int tcp_send_cb(void *opaque, const unsigned char *buf, size_t len) {
    tcp_conn_t *c = (tcp_conn_t *)opaque;
    if (len > 0xffffU) return -1;
    return tcp_send(c, buf, (uint16_t)len);
}
static int tcp_recv_cb(void *opaque, unsigned char *buf, size_t len,
                       uint32_t timeout_ms) {
    tcp_conn_t *c = (tcp_conn_t *)opaque;
    if (len > 0xffffU) len = 0xffffU;
    int n = tcp_recv(c, buf, (uint16_t)len, timeout_ms);
    if (n > 0) return n;
    if (c->state == TCP_STATE_CLOSE_WAIT || c->state == TCP_STATE_CLOSED) return 0;
    return -2; /* timeout: mbed TLS retries with WANT_READ */
}
void nexos_tls_tcp_io(nexos_tls_io_t *io, tcp_conn_t *conn) {
    io->opaque = conn; io->send = tcp_send_cb; io->recv = tcp_recv_cb;
    io->entropy = nexos_tls_entropy_rdrand;
}
int nexos_tls_entropy_rdrand(void *opaque, unsigned char *buf, size_t len) {
    (void)opaque;
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(1), "c"(0));
    if (!(ecx & (1u << 30))) return -1;
    for (size_t i = 0; i < len; i++) {
        uint32_t v; uint8_t ok;
        __asm__ volatile("rdrand %0; setc %1" : "=r"(v), "=qm"(ok));
        if (!ok) return -1;
        buf[i] = (unsigned char)v;
    }
    return 0;
}
