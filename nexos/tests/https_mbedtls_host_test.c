/* Exercises the same NexOS TLS callback boundary against a public HTTPS site. */
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include "../ports/tls/nexos_tls.h"
#include <mbedtls/error.h>

typedef struct { int fd; } host_io_t;
static int send_cb(void *p, const unsigned char *b, size_t n) {
    host_io_t *io = p; return (int)send(io->fd, b, n, 0);
}
static int recv_cb(void *p, unsigned char *b, size_t n, uint32_t timeout_ms) {
    host_io_t *io = p; struct timeval tv = { (long)(timeout_ms / 1000),
                                             (long)((timeout_ms % 1000) * 1000) };
    setsockopt(io->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    int r = (int)recv(io->fd, b, n, 0);
    if (r >= 0) return r;
    return (errno == EAGAIN || errno == EWOULDBLOCK) ? -2 : -1;
}
static int entropy_cb(void *unused, unsigned char *b, size_t n) {
    (void)unused; int fd = open("/dev/urandom", O_RDONLY); if (fd < 0) return -1;
    ssize_t r = read(fd, b, n); close(fd); return r == (ssize_t)n ? 0 : -1;
}
static int load_file(const char *path, unsigned char **out, size_t *len) {
    FILE *f = fopen(path, "rb"); long n; if (!f) return -1;
    if (fseek(f, 0, SEEK_END) || (n = ftell(f)) <= 0 || fseek(f, 0, SEEK_SET)) { fclose(f); return -1; }
    *out = malloc((size_t)n + 1); if (!*out) { fclose(f); return -1; }
    *len = fread(*out, 1, (size_t)n, f); fclose(f);
    if (*len == (size_t)n) { (*out)[*len] = 0; (*len)++; return 0; }
    return -1;
}
int main(int argc, char **argv) {
    const char *host = argc > 1 ? argv[1] : "example.com";
    const char *ca = argc > 2 ? argv[2] : "/etc/ssl/certs/ca-certificates.crt";
    struct addrinfo hints = {0}, *res = 0; int fd = -1; unsigned char *pem = 0; size_t pem_len = 0;
    hints.ai_socktype = SOCK_STREAM; hints.ai_family = AF_INET;
    if (getaddrinfo(host, "443", &hints, &res) != 0 || !res) return 2;
    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0 || connect(fd, res->ai_addr, res->ai_addrlen) < 0 || load_file(ca, &pem, &pem_len)) return 3;
    host_io_t h = { fd }; nexos_tls_io_t io = { &h, send_cb, recv_cb, entropy_cb }; nexos_tls_t tls;
    int tls_rc = nexos_tls_init(&tls, &io, host, pem, pem_len);
    fprintf(stderr, "TLS init=%d\n", tls_rc);
    if (!tls_rc) tls_rc = nexos_tls_handshake(&tls);
    if (tls_rc) { char e[128]; mbedtls_strerror(tls_rc, e, sizeof(e));
        fprintf(stderr, "TLS failed: %d %s verify=0x%x\n", tls_rc, e,
                nexos_tls_verify_result(&tls)); return 4; }
    const char req[] = "GET / HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n";
    if (nexos_tls_write(&tls, (const unsigned char *)req, sizeof(req) - 1) < 0) return 5;
    unsigned char body[4096]; int n, total = 0;
    while ((n = nexos_tls_read(&tls, body + total, sizeof(body) - 1 - total, 5000)) > 0) total += n;
    body[total] = 0; printf("verified HTTPS %s: TLS=%s verify=0x%x %s\n", host,
        nexos_tls_version(&tls), nexos_tls_verify_result(&tls),
        strstr((char *)body, "HTTP/") && strstr((char *)body, "Example Domain") ? "HTTP/body OK" : "unexpected body");
    nexos_tls_free(&tls); free(pem); close(fd); freeaddrinfo(res);
    return strstr((char *)body, "HTTP/") && strstr((char *)body, "Example Domain") ? 0 : 6;
}
