# NexOS TLS/HTTPS runtime

## Selection

The selected implementation is **Mbed TLS 3.6.7**, the maintained 3.6 LTS
branch. The locked official archive is recorded in
`ports/tls/sources.lock` and fetched by `tools/tls_fetch_source.sh`.

| Candidate | TLS | X.509 | NexOS decision |
|---|---|---|---|
| BearSSL | TLS 1.2; no TLS 1.3 | yes, but its minimal validator assumes a particular chain order | rejected for general public HTTPS compatibility |
| wolfSSL | TLS 1.2/1.3; X.509 and custom I/O | yes | larger configuration/licensing surface; not selected |
| Mbed TLS 3.6.7 | TLS 1.2/1.3; SNI and custom BIO callbacks | yes, with required hostname and time checks | selected |

Mbed TLS is dual Apache-2.0/GPL-2.0-or-later. Its TLS API supplies custom
send/receive callbacks, allocator/platform hooks, entropy callbacks, and a
replaceable X.509 trust chain.

## Adapter boundary

`ports/tls/nexos_tls.{h,c}` owns the TLS state machine. It requires:

* `nexos_tls_send_fn`: may perform a partial write;
* `nexos_tls_recv_fn`: returns positive bytes, `0` for EOF, `-2` for timeout,
  and another negative value for an I/O error;
* `nexos_tls_entropy_fn`: fills all requested bytes or fails;
* a NUL-terminated PEM trust bundle;
* a hostname passed to `mbedtls_ssl_set_hostname()`.

`ports/tls/nexos_tls_tcp.{h,c}` maps this boundary directly to the existing
`tcp_conn_t`, `tcp_send()`, and `tcp_recv()` implementation. It does not add a
second TCP stack or plaintext port-443 path. It refuses to report entropy when
the CPU lacks RDRAND or RDRAND fails.

The adapter uses `mbedtls_ssl_conf_authmode(..., MBEDTLS_SSL_VERIFY_REQUIRED)`,
`mbedtls_ssl_conf_ca_chain()`, `mbedtls_ssl_set_hostname()`,
`mbedtls_ssl_set_bio()`, `mbedtls_ssl_handshake()`, and
`mbedtls_ssl_get_verify_result()`. No certificate-verification bypass is
present.

## Platform audit

| Primitive | Status | Evidence/adaptation |
|---|---|---|
| allocation | ADAPT | Mbed TLS platform allocator must map to NexOS `kmalloc`/`kfree` in the freestanding library build |
| memory functions | EXISTS | NexOS libc implementations; TLS compile shims declare the same ABI |
| entropy | ADAPT | `nexos_tls_entropy_rdrand()` uses CPU entropy and fails closed when unavailable |
| TCP send/receive | ADAPT | `nexos_tls_tcp.c` calls existing `tcp_send`/`tcp_recv`; partial writes and timeout/EOF are represented |
| monotonic time | EXISTS | PIT `timer_get_ticks()` and `timer_get_uptime_seconds()` |
| wall-clock time | ADAPT | RTC-backed `gettimeofday` exists; Mbed TLS platform time/gmtime hooks still need final linkage |
| trust store | EXISTS/ADAPT | pinned Mozilla bundle at `ports/tls/trust/cacert.pem` |
| filesystem | ADAPT | final loader must read the bundle through the resource/VFS layer |
| threads/locks | OPTIONAL | first milestone is single-threaded; Mbed TLS threading is disabled |
| sockets | ADAPT | TLS callbacks receive an existing `tcp_conn_t`; no POSIX socket stack is added |

## Reproducible build and tests

```sh
make -C nexos tls-source
make -C nexos tls-host-test
```

`tls-host-test` builds the locked Mbed TLS source, uses the pinned CA bundle,
resolves `example.com`, opens TCP/443, performs a real handshake, validates
the chain and hostname, sends an HTTP/1.1 request, and checks the decrypted
HTTP response body. This is a pre-NetSurf adapter test; it is not QEMU proof.

The pinned bundle was downloaded from curl.se and has SHA-256
`a41b5d356aea97a529fe27e0f7316d2f9d946d75927476cf9cf1b90637d00505`.

## Remaining boundary

The real mbed TLS API adapter and host HTTPS proof are implemented. The
remaining work before `HTTPS WORKING` is the freestanding Mbed TLS library
build and its NexOS allocator/time/VFS linkage, followed by connecting this
adapter to the actual upstream NetSurf curl/fetcher path and exercising it in
QEMU. Compilation of the host library alone is not counted as that milestone.
