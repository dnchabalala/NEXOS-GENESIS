/* NexOS - kernel/net/http.c | HTTP/1.0 GET client | MIT License */
#include "http.h"
#include "tcp.h"
#include "dns.h"
#include "ethernet.h"
#include "../kernel.h"
#include "../mm/heap.h"
#include "../drivers/timer.h"
#include "../../ports/tls/nexos_tls.h"
#include "../../ports/tls/nexos_tls_tcp.h"

#define HTTP_BUF_SIZE   (64 * 1024)   /* 64 KB response buffer */
#define HTTP_RECV_MS    8000

/* ── static TCP connection (one HTTP request at a time) ────────────────────  */
static tcp_conn_t http_conn;
extern const unsigned char nexos_tls_ca_start[];
extern const unsigned char nexos_tls_ca_end[];

/* ── string helpers ──────────────────────────────────────────────────────── */
static void h_strcpy(char *d, const char *s, int max) {
    int i = 0;
    while (i < max - 1 && s[i]) { d[i] = s[i]; i++; }
    d[i] = 0;
}
static int h_strncmp(const char *a, const char *b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) return (unsigned char)a[i] - (unsigned char)b[i];
        if (!a[i]) return 0;
    }
    return 0;
}
static int h_atoi(const char *s) {
    int v = 0;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    return v;
}

/* ── parse URL: http://hostname[:port]/path ──────────────────────────────── */
static int parse_url(const char *url, char *host_out, int host_max,
                     uint16_t *port_out, char *path_out, int path_max) {
    const char *p = url;
    if (h_strncmp(p, "https://", 8) == 0) {
        p += 8;
        *port_out = 443;
    } else {
        *port_out = 80;
    }
    if (h_strncmp(p, "http://", 7) == 0) p += 7;

    /* host[:port] ends at first '/' or end of string */
    int hi = 0;
    while (*p && *p != '/' && *p != ':' && hi < host_max - 1)
        host_out[hi++] = *p++;
    host_out[hi] = 0;

    if (*p == ':') {
        p++;
        *port_out = (uint16_t)h_atoi(p);
        while (*p >= '0' && *p <= '9') p++;
    }

    /* path */
    if (*p == '/') {
        h_strcpy(path_out, p, path_max);
    } else {
        path_out[0] = '/'; path_out[1] = 0;
    }
    return 0;
}

/* ── header helpers ──────────────────────────────────────────────────────── */
static int h_ci_equal(const uint8_t *a, const char *b, int n) {
    for (int i = 0; i < n; i++) {
        uint8_t ca = a[i];
        char cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca = (uint8_t)(ca + 32);
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb + 32);
        if (ca != (uint8_t)cb) return 0;
    }
    return 1;
}

/* Return the value after a header name, or NULL. The returned pointer points
   into the response buffer and is only valid until that buffer is freed. */
static const uint8_t *find_header(const uint8_t *data, uint32_t len,
                                  const char *name, int name_len) {
    uint32_t i = 0;
    while (i + (uint32_t)name_len < len) {
        uint32_t line = i;
        uint32_t end = line;
        while (end + 1 < len && !(data[end] == '\r' && data[end+1] == '\n')) end++;
        if (end == line) break;
        if (end - line > (uint32_t)name_len &&
            h_ci_equal(data + line, name, name_len) &&
            data[line + name_len] == ':') {
            uint32_t p = line + (uint32_t)name_len + 1;
            while (p < end && (data[p] == ' ' || data[p] == '\t')) p++;
            return data + p;
        }
        i = (end + 2 <= len) ? end + 2 : len;
    }
    return 0;
}

static int header_value_len(const uint8_t *p, const uint8_t *end) {
    const uint8_t *q = p;
    while (q < end && *q != '\r' && *q != '\n') q++;
    return (int)(q - p);
}

static int has_chunked_encoding(const uint8_t *p, int len) {
    for (int i = 0; i + 7 < len; i++) {
        if ((p[i] == 'c' || p[i] == 'C') &&
            (p[i+1] == 'h' || p[i+1] == 'H') &&
            (p[i+2] == 'u' || p[i+2] == 'U') &&
            (p[i+3] == 'n' || p[i+3] == 'N') &&
            (p[i+4] == 'k' || p[i+4] == 'K') &&
            (p[i+5] == 'e' || p[i+5] == 'E') &&
            (p[i+6] == 'd' || p[i+6] == 'D')) return 1;
    }
    return 0;
}

static int hex_value(uint8_t c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Decode HTTP chunked transfer coding in place. */
static uint32_t decode_chunked(uint8_t *data, uint32_t len) {
    uint32_t src = 0, dst = 0;
    while (src < len) {
        uint32_t size = 0;
        int digits = 0;
        while (src < len && data[src] != '\r' && data[src] != '\n') {
            int v = hex_value(data[src++]);
            if (v < 0) return 0;
            size = (size << 4) | (uint32_t)v;
            digits++;
            if (size > HTTP_BUF_SIZE) return 0;
        }
        if (!digits || src + 1 >= len || data[src] != '\r' || data[src+1] != '\n') return 0;
        src += 2;
        if (size == 0) return dst;
        if (src + size > len || dst + size > len) return 0;
        for (uint32_t i = 0; i < size; i++) data[dst++] = data[src++];
        if (src + 1 >= len || data[src] != '\r' || data[src+1] != '\n') return 0;
        src += 2;
    }
    return 0;
}

/* ── build GET request into buf ─────────────────────────────────────────── */
static int build_request(uint8_t *buf, int buf_sz,
                         const char *host, const char *path) {
    /* HTTP/1.1 is required by many current web servers. Connection: close
       keeps the TCP implementation simple while still allowing framing via
       Content-Length or chunked transfer encoding. */
    const char *method = "GET ";
    const char *ver    = " HTTP/1.1\r\nHost: ";
    const char *ua     = "\r\nUser-Agent: NexOS/1.0\r\nAccept: text/html,text/plain\r\nConnection: close\r\n\r\n";
    int pos = 0;
    for (int i = 0; method[i] && pos < buf_sz - 1; i++) buf[pos++] = (uint8_t)method[i];
    for (int i = 0; path[i]   && pos < buf_sz - 1; i++) buf[pos++] = (uint8_t)path[i];
    for (int i = 0; ver[i]    && pos < buf_sz - 1; i++) buf[pos++] = (uint8_t)ver[i];
    for (int i = 0; host[i]   && pos < buf_sz - 1; i++) buf[pos++] = (uint8_t)host[i];
    for (int i = 0; ua[i]     && pos < buf_sz - 1; i++) buf[pos++] = (uint8_t)ua[i];
    return pos;
}

/* ── parse status line: "HTTP/1.x NNN ..." → NNN ────────────────────────── */
static int parse_status(const uint8_t *data, uint32_t len) {
    /* Find first space */
    uint32_t i = 0;
    while (i < len && data[i] != ' ') i++;
    if (i >= len) return 0;
    i++;
    int code = 0;
    while (i < len && data[i] >= '0' && data[i] <= '9') {
        code = code * 10 + (data[i] - '0');
        i++;
    }
    return code;
}

/* ── find \r\n\r\n in buffer, return offset of body start ────────────────── */
static int find_body(const uint8_t *data, uint32_t len) {
    for (uint32_t i = 0; i + 3 < len; i++) {
        if (data[i]=='\r' && data[i+1]=='\n' &&
            data[i+2]=='\r' && data[i+3]=='\n')
            return (int)(i + 4);
    }
    return -1;
}

/* ── public API ──────────────────────────────────────────────────────────── */
http_response_t *http_get(const char *url) {
    char     host[128];
    char     path[256];
    uint16_t port;

    int parse_result = parse_url(url, host, sizeof(host), &port,
                                 path, sizeof(path));
    if (parse_result < 0)
        return 0;
    int is_https = h_strncmp(url, "https://", 8) == 0;

    /* Resolve hostname */
    uint8_t host_ip[4];
    uint32_t dst_ip;
    /* Check if it's a dotted-decimal IP first */
    int a0 = 0, a1 = 0, a2 = 0, a3 = 0, dot_count = 0;
    const char *hp = host;
    int digits = 0;
    while (*hp) {
        if (*hp >= '0' && *hp <= '9') digits++;
        else if (*hp == '.') dot_count++;
        hp++;
    }
    if (dot_count == 3 && digits > 0) {
        /* Parse as IP */
        const char *q = host;
        a0 = h_atoi(q); while (*q && *q != '.') q++; if (*q) q++;
        a1 = h_atoi(q); while (*q && *q != '.') q++; if (*q) q++;
        a2 = h_atoi(q); while (*q && *q != '.') q++; if (*q) q++;
        a3 = h_atoi(q);
        dst_ip = ((uint32_t)a0 << 24) | ((uint32_t)a1 << 16)
               | ((uint32_t)a2 <<  8) | (uint32_t)a3;
    } else {
        if (dns_resolve(host, host_ip) < 0) {
            klog(LOG_WARN, "HTTP: DNS failed for %s", host);
            return 0;
        }
        dst_ip = ((uint32_t)host_ip[0] << 24) | ((uint32_t)host_ip[1] << 16)
               | ((uint32_t)host_ip[2] <<  8) |  host_ip[3];
    }

    /* Connect */
    if (tcp_connect(&http_conn, dst_ip, port) < 0) {
        klog(LOG_WARN, "HTTP: TCP connect failed to %s:%d", host, (int)port);
        return 0;
    }

    nexos_tls_t tls;
    int tls_active = 0;
    if (is_https) {
        nexos_tls_io_t io;
        nexos_tls_tcp_io(&io, &http_conn);
        size_t ca_len = (size_t)(nexos_tls_ca_end - nexos_tls_ca_start);
        int tls_rc = nexos_tls_init(&tls, &io, host, nexos_tls_ca_start, ca_len);
        if (!tls_rc) tls_rc = nexos_tls_handshake(&tls);
        if (tls_rc) {
            klog(LOG_WARN, "HTTPS: TLS failed for %s rc=%d verify=0x%x",
                 host, tls_rc, nexos_tls_verify_result(&tls));
            nexos_tls_free(&tls);
            tcp_close(&http_conn);
            return 0;
        }
        tls_active = 1;
        klog(LOG_INFO, "HTTPS: TLS=%s verify=0x%x hostname=%s",
             nexos_tls_version(&tls), nexos_tls_verify_result(&tls), host);
    }

    /* Build and send GET request */
    uint8_t req[512];
    int req_len = build_request(req, (int)sizeof(req), host, path);
    int send_rc = tls_active
        ? nexos_tls_write(&tls, req, (size_t)req_len)
        : tcp_send(&http_conn, req, (uint16_t)req_len);
    if (send_rc < 0) {
        if (tls_active) nexos_tls_free(&tls);
        tcp_close(&http_conn);
        return 0;
    }

    /* Receive full response */
    uint8_t *resp_buf = (uint8_t *)kmalloc(HTTP_BUF_SIZE);
    if (!resp_buf) {
        if (tls_active) nexos_tls_free(&tls);
        tcp_close(&http_conn);
        return 0;
    }

    uint32_t total = 0;
    uint64_t deadline = timer_get_ticks() + HTTP_RECV_MS;
    while (timer_get_ticks() < deadline && total < HTTP_BUF_SIZE - 1) {
        int n = tls_active
            ? nexos_tls_read(&tls, resp_buf + total,
                             HTTP_BUF_SIZE - 1 - total, 500)
            : tcp_recv(&http_conn, resp_buf + total,
                       (uint16_t)(HTTP_BUF_SIZE - 1 - total), 500);
        if (n > 0) {
            total += (uint32_t)n;
            deadline = timer_get_ticks() + HTTP_RECV_MS; /* reset on data */
        }
        if (!tls_active && (http_conn.state == TCP_STATE_CLOSE_WAIT
            || http_conn.state == TCP_STATE_CLOSED)) break;
    }
    if (tls_active) nexos_tls_free(&tls);
    tcp_close(&http_conn);

    if (total == 0) {
        kfree(resp_buf);
        klog(LOG_WARN, "HTTP: no data received");
        return 0;
    }

    /* Parse response */
    http_response_t *r = (http_response_t *)kmalloc(sizeof(http_response_t));
    if (!r) { kfree(resp_buf); return 0; }

    r->status_code = parse_status(resp_buf, total);
    int body_off   = find_body(resp_buf, total);

    r->location[0] = 0;
    r->content_type[0] = 0;
    const uint8_t *location = find_header(resp_buf, total, "Location", 8);
    if (location) {
        int n = header_value_len(location, resp_buf + total);
        if (n > (int)sizeof(r->location) - 1) n = (int)sizeof(r->location) - 1;
        for (int i = 0; i < n; i++) r->location[i] = (char)location[i];
        r->location[n] = 0;
    }

    const uint8_t *content_type = find_header(resp_buf, total,
                                               "Content-Type", 12);
    if (content_type) {
        int n = header_value_len(content_type, resp_buf + total);
        if (n > (int)sizeof(r->content_type) - 1)
            n = (int)sizeof(r->content_type) - 1;
        for (int i = 0; i < n; i++)
            r->content_type[i] = (char)content_type[i];
        r->content_type[n] = 0;
    }

    if (body_off < 0 || (uint32_t)body_off >= total) {
        r->body     = 0;
        r->body_len = 0;
    } else {
        r->body_len = total - (uint32_t)body_off;
        r->body     = (uint8_t *)kmalloc(r->body_len + 1);
        if (r->body) {
            for (uint32_t i = 0; i < r->body_len; i++)
                r->body[i] = resp_buf[body_off + i];
            const uint8_t *encoding = find_header(resp_buf, (uint32_t)body_off,
                                                   "Transfer-Encoding", 17);
            if (encoding && has_chunked_encoding(encoding,
                                                  header_value_len(encoding,
                                                                   resp_buf + body_off))) {
                r->body_len = decode_chunked(r->body, r->body_len);
            }
            r->body[r->body_len] = 0;
        }
    }
    kfree(resp_buf);
    klog(LOG_INFO, "HTTP: %s status=%d body=%u bytes",
         url, r->status_code, r->body_len);
    return r;
}

void http_free(http_response_t *r) {
    if (!r) return;
    if (r->body) kfree(r->body);
    kfree(r);
}
