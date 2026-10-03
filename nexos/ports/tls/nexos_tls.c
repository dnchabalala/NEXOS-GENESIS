/* Thin mbed TLS adapter. Certificate and hostname verification are mandatory. */
#include "nexos_tls.h"
#include <string.h>

static int tls_send(void *opaque, const unsigned char *buf, size_t len) {
    nexos_tls_t *tls = (nexos_tls_t *)opaque;
    int n = tls->io.send(tls->io.opaque, buf, len);
    return n < 0 ? MBEDTLS_ERR_SSL_INTERNAL_ERROR : n;
}
static int tls_recv(void *opaque, unsigned char *buf, size_t len) {
    nexos_tls_t *tls = (nexos_tls_t *)opaque;
    int n = tls->io.recv(tls->io.opaque, buf, len, 1000);
    if (n == -2) return MBEDTLS_ERR_SSL_WANT_READ;
    return n < 0 ? MBEDTLS_ERR_SSL_INTERNAL_ERROR : n;
}
static int tls_recv_timeout(void *opaque, unsigned char *buf, size_t len,
                            uint32_t timeout_ms) {
    nexos_tls_t *tls = (nexos_tls_t *)opaque;
    int n = tls->io.recv(tls->io.opaque, buf, len, timeout_ms);
    if (n == -2) return MBEDTLS_ERR_SSL_TIMEOUT;
    return n < 0 ? MBEDTLS_ERR_SSL_INTERNAL_ERROR : n;
}
static int tls_entropy(void *opaque, unsigned char *buf, size_t len,
                       size_t *olen) {
    nexos_tls_t *tls = (nexos_tls_t *)opaque;
    int rc = tls->io.entropy(tls->io.opaque, buf, len);
    if (rc == 0) *olen = len;
    return rc == 0 ? 0 : -1;
}

int nexos_tls_init(nexos_tls_t *tls, const nexos_tls_io_t *io,
                   const char *hostname, const unsigned char *ca_pem,
                   size_t ca_len) {
    int rc;
    if (!tls || !io || !io->send || !io->recv || !io->entropy ||
        !hostname || !ca_pem || !ca_len) return -1;
    memset(tls, 0, sizeof(*tls));
    tls->io = *io;
    mbedtls_ssl_init(&tls->ssl); mbedtls_ssl_config_init(&tls->config);
    mbedtls_ctr_drbg_init(&tls->drbg); mbedtls_entropy_init(&tls->entropy);
    mbedtls_x509_crt_init(&tls->trust);
    rc = mbedtls_entropy_add_source(&tls->entropy, tls_entropy, tls,
                                    32, MBEDTLS_ENTROPY_SOURCE_STRONG);
    if (rc != 0) goto fail;
    rc = mbedtls_ctr_drbg_seed(&tls->drbg, mbedtls_entropy_func,
                               &tls->entropy, (const unsigned char *)hostname,
                               strlen(hostname));
    if (rc != 0) goto fail;
    rc = mbedtls_x509_crt_parse(&tls->trust, ca_pem, ca_len);
    if (rc < 0) goto fail;
    rc = mbedtls_ssl_config_defaults(&tls->config, MBEDTLS_SSL_IS_CLIENT,
                                     MBEDTLS_SSL_TRANSPORT_STREAM,
                                     MBEDTLS_SSL_PRESET_DEFAULT);
    if (rc != 0) goto fail;
    mbedtls_ssl_conf_authmode(&tls->config, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_read_timeout(&tls->config, 1000);
    mbedtls_ssl_conf_ca_chain(&tls->config, &tls->trust, 0);
    mbedtls_ssl_conf_rng(&tls->config, mbedtls_ctr_drbg_random, &tls->drbg);
    rc = mbedtls_ssl_setup(&tls->ssl, &tls->config);
    if (rc != 0) goto fail;
    rc = mbedtls_ssl_set_hostname(&tls->ssl, hostname);
    if (rc != 0) goto fail;
    mbedtls_ssl_set_bio(&tls->ssl, tls, tls_send, tls_recv, tls_recv_timeout);
    tls->initialized = 1;
    return 0;
fail:
    nexos_tls_free(tls); return rc;
}

int nexos_tls_handshake(nexos_tls_t *tls) {
    int rc;
    if (!tls || !tls->initialized) return -1;
    do { rc = mbedtls_ssl_handshake(&tls->ssl); }
    while (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE);
    if (rc != 0 || mbedtls_ssl_get_verify_result(&tls->ssl) != 0) return rc ? rc : -2;
    tls->handshaken = 1; return 0;
}
int nexos_tls_write(nexos_tls_t *tls, const unsigned char *buf, size_t len) {
    if (!tls || !tls->handshaken) return -1;
    return mbedtls_ssl_write(&tls->ssl, buf, len);
}
int nexos_tls_read(nexos_tls_t *tls, unsigned char *buf, size_t len,
                   uint32_t timeout_ms) {
    if (!tls || !tls->handshaken) return -1;
    (void)timeout_ms;
    return mbedtls_ssl_read(&tls->ssl, buf, len);
}
uint32_t nexos_tls_verify_result(const nexos_tls_t *tls) {
    return tls ? mbedtls_ssl_get_verify_result(&tls->ssl) : 0xffffffffU;
}
const char *nexos_tls_version(const nexos_tls_t *tls) {
    return tls ? mbedtls_ssl_get_version(&tls->ssl) : "none";
}
void nexos_tls_free(nexos_tls_t *tls) {
    if (!tls) return;
    mbedtls_x509_crt_free(&tls->trust); mbedtls_ssl_free(&tls->ssl);
    mbedtls_ssl_config_free(&tls->config); mbedtls_ctr_drbg_free(&tls->drbg);
    mbedtls_entropy_free(&tls->entropy); tls->initialized = 0; tls->handshaken = 0;
}
