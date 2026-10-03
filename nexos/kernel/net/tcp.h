/* NexOS - kernel/net/tcp.h | Minimal TCP (RFC 793) | MIT License */
#ifndef TCP_H
#define TCP_H

#include <stdint.h>

#define TCP_RX_BUF_SIZE  8192
#define TCP_MSS          1460
#define TCP_MAX_CONNECTIONS 8
#define TCP_MAX_OOO      4

#define TCP_STATE_CLOSED      0
#define TCP_STATE_SYN_SENT    1
#define TCP_STATE_ESTABLISHED 2
#define TCP_STATE_FIN_WAIT    3
#define TCP_STATE_CLOSE_WAIT  4

#define TCP_FLAG_FIN  0x01
#define TCP_FLAG_SYN  0x02
#define TCP_FLAG_RST  0x04
#define TCP_FLAG_PSH  0x08
#define TCP_FLAG_ACK  0x10

typedef struct {
    uint32_t seq;
    uint16_t len;
    uint8_t  used;
    uint8_t  data[TCP_MSS];
} tcp_ooo_t;

typedef struct {
    uint32_t remote_ip;
    uint16_t remote_port;
    uint16_t local_port;
    uint32_t seq;             /* next sequence number to allocate */
    uint32_t ack;             /* next byte expected from peer */
    uint32_t rx_next;
    uint16_t remote_window;
    uint16_t rx_len;
    uint8_t  state;
    uint8_t  fin_sent;
    uint8_t  tx_inflight;
    uint8_t  tx_retries;
    uint32_t tx_seq;
    uint16_t tx_len;
    uint64_t tx_deadline;
    uint8_t  tx_buf[TCP_MSS];
    uint8_t  rx_buf[TCP_RX_BUF_SIZE];
    tcp_ooo_t ooo[TCP_MAX_OOO];
} tcp_conn_t;

/* Open TCP connection. conn is caller-allocated (use static or kmalloc).
 * Returns 0 on success, -1 on failure. */
int  tcp_connect(tcp_conn_t *conn, uint32_t ip, uint16_t port);

/* Send data over an established connection. */
int  tcp_send(tcp_conn_t *conn, const uint8_t *data, uint16_t len);

/* Receive data. Polls up to timeout_ms. Returns bytes received. */
int  tcp_recv(tcp_conn_t *conn, uint8_t *buf, uint16_t maxlen,
              uint32_t timeout_ms);

/* Close connection gracefully. */
void tcp_close(tcp_conn_t *conn);

/* Called from ip_receive when protocol == 6. */
void tcp_receive(const uint8_t *data, uint16_t len, uint32_t src_ip);

#endif
