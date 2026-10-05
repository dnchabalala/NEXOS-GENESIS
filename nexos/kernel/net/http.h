/* NexOS - kernel/net/http.h | Minimal HTTP/1.0 GET client | MIT License */
#ifndef HTTP_H
#define HTTP_H

#include <stdint.h>

typedef struct {
    int      status_code;
    uint32_t body_len;
    uint8_t *body;    /* kmalloc'd; caller must call http_free */
    char     location[256]; /* Location header for redirects, if present */
    char     content_type[128]; /* Content-Type header, if present */
} http_response_t;

/* Fetch a URL via HTTP GET.
 * url must start with "http://".
 * Returns allocated response on success, NULL on failure. */
http_response_t *http_get(const char *url);

/* Bounded high-level tracing for a browser fetch.  The HTTP client remains
 * synchronous; these hooks only annotate its existing phase boundaries. */
void http_trace_begin(int fetch_id, const char *url);
void http_trace_event(const char *stage, int value);
void http_trace_end(int success);
void http_trace_navigation_start(uint64_t start);
uint64_t http_trace_elapsed(void);

/* Free a response returned by http_get. */
void             http_free(http_response_t *r);

#endif
