/* NexOS TLS boundary over an already-established TCP stream. */
#ifndef NEXOS_TLS_H
#define NEXOS_TLS_H

#include <stddef.h>
#include <stdint.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/x509_crt.h>

typedef int (*nexos_tls_send_fn)(void *opaque, const unsigned char *buf,
                                size_t len);
typedef int (*nexos_tls_recv_fn)(void *opaque, unsigned char *buf,
                                size_t len, uint32_t timeout_ms);
typedef int (*nexos_tls_entropy_fn)(void *opaque, unsigned char *buf,
                                    size_t len);

typedef struct {
    void *opaque;
    nexos_tls_send_fn send;
    nexos_tls_recv_fn recv;
    nexos_tls_entropy_fn entropy;
} nexos_tls_io_t;

typedef struct {
    nexos_tls_io_t io;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config config;
    mbedtls_ctr_drbg_context drbg;
    mbedtls_entropy_context entropy;
    mbedtls_x509_crt trust;
    int initialized;
    int handshaken;
} nexos_tls_t;

/* ca_pem may contain one or more genuine PEM trust anchors. */
int nexos_tls_init(nexos_tls_t *tls, const nexos_tls_io_t *io,
                   const char *hostname, const unsigned char *ca_pem,
                   size_t ca_len);
int nexos_tls_handshake(nexos_tls_t *tls);
int nexos_tls_write(nexos_tls_t *tls, const unsigned char *buf, size_t len);
int nexos_tls_read(nexos_tls_t *tls, unsigned char *buf, size_t len,
                   uint32_t timeout_ms);
uint32_t nexos_tls_verify_result(const nexos_tls_t *tls);
const char *nexos_tls_version(const nexos_tls_t *tls);
void nexos_tls_free(nexos_tls_t *tls);

#endif
