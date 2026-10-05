/* NexOS - kernel/net/tcp.c | bounded reliable TCP | MIT License */
#include "tcp.h"
#include "ip.h"
#include "ethernet.h"
#include "../kernel.h"
#include "../mm/heap.h"
#include "../drivers/rtl8139.h"
#include "../drivers/timer.h"

#ifndef NEXOS_TCP_TRACE
#define NEXOS_TCP_TRACE 0
#endif

#define TCP_HDR_LEN 20
#define TCP_RTO_MS 500
#define TCP_MAX_RETRIES 5
#define TCP_CONNECT_MS 5000
#define TCP_CLOSE_MS 2000

static tcp_conn_t *connections[TCP_MAX_CONNECTIONS];
static uint16_t tcp_next_port = 49152;
static unsigned int tcp_trace_rx_logs;
static unsigned int tcp_trace_ooo_logs;
static unsigned int tcp_trace_window_logs;
static uint64_t tcp_trace_last_rx;

static uint32_t rd32(const uint8_t *p) { return ((uint32_t)p[0] << 24) |
    ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }
static uint16_t rd16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }

static uint16_t checksum(uint32_t src, uint32_t dst, const uint8_t *p, uint16_t n) {
    uint32_t sum = (src >> 16) + (src & 0xffff) + (dst >> 16) + (dst & 0xffff);
    sum += 6 + n;
    for (uint16_t i = 0; i + 1 < n; i += 2) sum += (p[i] << 8) | p[i + 1];
    if (n & 1) sum += p[n - 1] << 8;
    while (sum >> 16) sum = (sum & 0xffff) + (sum >> 16);
    return (uint16_t)~sum;
}

static uint16_t advertised_window(const tcp_conn_t *c) {
    return (uint16_t)(TCP_RX_BUF_SIZE - c->rx_len);
}

static int send_segment(tcp_conn_t *c, uint32_t seq, uint32_t ack,
                        const uint8_t *payload, uint16_t plen, uint8_t flags) {
    uint16_t n = (uint16_t)(TCP_HDR_LEN + plen);
    uint8_t *seg = (uint8_t *)kmalloc(n);
    if (!seg) return -1;
    seg[0] = (uint8_t)(c->local_port >> 8); seg[1] = (uint8_t)c->local_port;
    seg[2] = (uint8_t)(c->remote_port >> 8); seg[3] = (uint8_t)c->remote_port;
    seg[4] = (uint8_t)(seq >> 24); seg[5] = (uint8_t)(seq >> 16);
    seg[6] = (uint8_t)(seq >> 8); seg[7] = (uint8_t)seq;
    seg[8] = (uint8_t)(ack >> 24); seg[9] = (uint8_t)(ack >> 16);
    seg[10] = (uint8_t)(ack >> 8); seg[11] = (uint8_t)ack;
    seg[12] = 0x50; seg[13] = flags;
    uint16_t win = advertised_window(c);
    seg[14] = (uint8_t)(win >> 8); seg[15] = (uint8_t)win;
    seg[16] = seg[17] = seg[18] = seg[19] = 0;
    for (uint16_t i = 0; i < plen; i++) seg[TCP_HDR_LEN + i] = payload[i];
    uint16_t sum = checksum(eth_our_ip, c->remote_ip, seg, n);
    seg[16] = (uint8_t)(sum >> 8); seg[17] = (uint8_t)sum;
    int ret = ip_send(c->remote_ip, IP_PROTO_TCP, seg, n);
    kfree(seg); return ret;
}

static int register_conn(tcp_conn_t *c) {
    for (int i = 0; i < TCP_MAX_CONNECTIONS; i++)
        if (!connections[i]) { connections[i] = c; return 0; }
    return -1;
}
static void unregister_conn(tcp_conn_t *c) {
    for (int i = 0; i < TCP_MAX_CONNECTIONS; i++) if (connections[i] == c) connections[i] = 0;
}
static tcp_conn_t *find_conn(uint32_t ip, uint16_t sport, uint16_t dport) {
    for (int i = 0; i < TCP_MAX_CONNECTIONS; i++) {
        tcp_conn_t *c = connections[i];
        if (c && c->remote_ip == ip && c->remote_port == sport &&
            c->local_port == dport && c->state != TCP_STATE_CLOSED) return c;
    }
    return 0;
}

static void promote_ooo(tcp_conn_t *c) {
    int changed;
    do {
        changed = 0;
        for (int i = 0; i < TCP_MAX_OOO; i++) if (c->ooo[i].used) {
            tcp_ooo_t *o = &c->ooo[i];
            if (o->seq == c->rx_next && c->rx_len + o->len <= TCP_RX_BUF_SIZE) {
                for (uint16_t j = 0; j < o->len; j++) c->rx_buf[c->rx_len++] = o->data[j];
                c->rx_next += o->len; o->used = 0; changed = 1;
                if (NEXOS_TCP_TRACE && tcp_trace_ooo_logs < 16) {
                    klog(LOG_INFO, "TCP OOO RELEASE seq=%u len=%u next=%u",
                         o->seq, o->len, c->rx_next);
                    tcp_trace_ooo_logs++;
                }
            }
        }
    } while (changed);
}
static void accept_payload(tcp_conn_t *c, uint32_t seq, const uint8_t *p, uint16_t n) {
    if (!n) return;
    if (seq == c->rx_next && c->rx_len + n <= TCP_RX_BUF_SIZE) {
        for (uint16_t i = 0; i < n; i++) c->rx_buf[c->rx_len++] = p[i];
        c->rx_next += n; promote_ooo(c); return;
    }
    if (seq < c->rx_next || seq - c->rx_next >= TCP_RX_BUF_SIZE) return;
    for (int i = 0; i < TCP_MAX_OOO; i++) if (!c->ooo[i].used) {
        uint16_t copy = n > TCP_MSS ? TCP_MSS : n;
        c->ooo[i].seq = seq; c->ooo[i].len = copy; c->ooo[i].used = 1;
        for (uint16_t j = 0; j < copy; j++) c->ooo[i].data[j] = p[j];
        if (NEXOS_TCP_TRACE && tcp_trace_ooo_logs < 16) {
            klog(LOG_INFO, "TCP OOO QUEUE seq=%u len=%u expected=%u",
                 seq, copy, c->rx_next);
            tcp_trace_ooo_logs++;
        }
        return;
    }
}

void tcp_receive(const uint8_t *data, uint16_t len, uint32_t src_ip) {
    if (len < TCP_HDR_LEN) return;
    uint16_t sport = rd16(data), dport = rd16(data + 2);
    tcp_conn_t *c = find_conn(src_ip, sport, dport);
    if (!c) return;
    uint8_t off = (uint8_t)((data[12] >> 4) * 4), flags = data[13];
    if (off < TCP_HDR_LEN || off > len) return;
    if (checksum(src_ip, eth_our_ip, data, len) != 0) return;
    c->remote_window = rd16(data + 14);
    uint32_t seq = rd32(data + 4), ack = rd32(data + 8);
    if (flags & TCP_FLAG_RST) { c->state = TCP_STATE_CLOSED; return; }
    if (c->state == TCP_STATE_SYN_SENT) {
        if ((flags & (TCP_FLAG_SYN | TCP_FLAG_ACK)) == (TCP_FLAG_SYN | TCP_FLAG_ACK) &&
            ack == c->seq + 1) {
            c->ack = c->rx_next = seq + 1; c->seq++;
            c->state = TCP_STATE_ESTABLISHED;
            send_segment(c, c->seq, c->ack, 0, 0, TCP_FLAG_ACK);
        }
        return;
    }
    if (flags & TCP_FLAG_ACK) {
        if (c->tx_inflight && ack >= c->tx_seq + c->tx_len) {
            c->tx_inflight = 0; c->seq = c->tx_seq + c->tx_len;
        }
        if (c->state == TCP_STATE_FIN_WAIT && c->fin_sent && ack >= c->seq)
            c->state = TCP_STATE_CLOSED;
    }
    if (c->state != TCP_STATE_ESTABLISHED && c->state != TCP_STATE_FIN_WAIT) return;
    uint16_t plen = (uint16_t)(len - off);
    if (plen) {
        uint64_t now = timer_get_ticks();
        uint64_t gap = tcp_trace_last_rx ? now - tcp_trace_last_rx : 0;
        if (NEXOS_TCP_TRACE && (tcp_trace_rx_logs < 24 || gap >= 1000)) {
            klog(LOG_INFO, "TCP DATA seq=%u ack=%u len=%u peer_win=%u gap=%u",
                 seq, c->ack, plen, c->remote_window, (uint32_t)gap);
            tcp_trace_rx_logs++;
        }
        tcp_trace_last_rx = now;
    }
    accept_payload(c, seq, data + off, plen);
    if (plen || (flags & TCP_FLAG_FIN)) {
        if (flags & TCP_FLAG_FIN) {
            if (seq + plen == c->rx_next) c->rx_next++;
            c->state = TCP_STATE_CLOSE_WAIT;
        }
        c->ack = c->rx_next;
        send_segment(c, c->seq, c->ack, 0, 0, TCP_FLAG_ACK);
        if (NEXOS_TCP_TRACE && tcp_trace_rx_logs < 24) {
            klog(LOG_INFO, "TCP ACK ack=%u win=%u", c->ack,
                 advertised_window(c));
        }
    }
}

static void service_tx(tcp_conn_t *c) {
    if (!c->tx_inflight || timer_get_ticks() < c->tx_deadline) return;
    if (c->tx_retries++ >= TCP_MAX_RETRIES) { c->state = TCP_STATE_CLOSED; return; }
    send_segment(c, c->tx_seq, c->ack, c->tx_buf, c->tx_len, TCP_FLAG_ACK | TCP_FLAG_PSH);
    c->tx_deadline = timer_get_ticks() + TCP_RTO_MS;
}
static void poll_net(void) {
    rtl8139_service();
    for (int i = 0; i < TCP_MAX_CONNECTIONS; i++) if (connections[i]) service_tx(connections[i]);
}

int tcp_connect(tcp_conn_t *c, uint32_t ip, uint16_t port) {
    uint8_t *z = (uint8_t *)c; for (uint32_t i = 0; i < sizeof(*c); i++) z[i] = 0;
    c->remote_ip = ip; c->remote_port = port; c->local_port = tcp_next_port++;
    if (tcp_next_port < 49152) tcp_next_port = 49152;
    c->seq = (uint32_t)timer_get_ticks() | 1U; c->remote_window = 65535;
    tcp_trace_rx_logs = 0;
    tcp_trace_ooo_logs = 0;
    tcp_trace_window_logs = 0;
    tcp_trace_last_rx = 0;
    c->state = TCP_STATE_SYN_SENT;
    if (register_conn(c) < 0) return -1;
    uint32_t syn_seq = c->seq;
    for (int attempt = 0; attempt <= TCP_MAX_RETRIES && c->state == TCP_STATE_SYN_SENT; attempt++) {
        if (send_segment(c, syn_seq, 0, 0, 0, TCP_FLAG_SYN) < 0) break;
        uint64_t until = timer_get_ticks() + (attempt ? TCP_RTO_MS : TCP_CONNECT_MS);
        while (timer_get_ticks() < until && c->state == TCP_STATE_SYN_SENT) poll_net();
    }
    if (c->state != TCP_STATE_ESTABLISHED) { unregister_conn(c); c->state = TCP_STATE_CLOSED; return -1; }
    klog(LOG_INFO, "TCP: connected to %d.%d.%d.%d:%d", (ip >> 24) & 255,
         (ip >> 16) & 255, (ip >> 8) & 255, ip & 255, port);
    return 0;
}

int tcp_send(tcp_conn_t *c, const uint8_t *data, uint16_t len) {
    if (!c || c->state != TCP_STATE_ESTABLISHED) return -1;
    uint16_t sent = 0;
    while (sent < len) {
        while (c->tx_inflight && c->state == TCP_STATE_ESTABLISHED) poll_net();
        if (c->state != TCP_STATE_ESTABLISHED) return sent ? sent : -1;
        uint16_t n = (uint16_t)(len - sent); if (n > TCP_MSS) n = TCP_MSS;
        if (c->remote_window == 0) { poll_net(); continue; }
        if (n > c->remote_window) n = c->remote_window;
        for (uint16_t i = 0; i < n; i++) c->tx_buf[i] = data[sent + i];
        c->tx_seq = c->seq; c->tx_len = n; c->tx_retries = 0; c->tx_inflight = 1;
        if (send_segment(c, c->tx_seq, c->ack, c->tx_buf, n, TCP_FLAG_ACK | TCP_FLAG_PSH) < 0)
            return sent ? sent : -1;
        c->tx_deadline = timer_get_ticks() + TCP_RTO_MS; sent += n;
    }
    return sent;
}

int tcp_recv(tcp_conn_t *c, uint8_t *buf, uint16_t maxlen, uint32_t timeout_ms) {
    uint64_t until = timer_get_ticks() + timeout_ms;
    while (timer_get_ticks() < until) {
        poll_net();
        if (c->rx_len) {
            uint16_t n = c->rx_len < maxlen ? c->rx_len : maxlen;
            for (uint16_t i = 0; i < n; i++) buf[i] = c->rx_buf[i];
            for (uint16_t i = n; i < c->rx_len; i++) c->rx_buf[i - n] = c->rx_buf[i];
            c->rx_len -= n;
            /* Reading from the socket reopens receive-buffer space. Send an
             * immediate window update so a peer that filled the window does
             * not wait for a retransmission timer. */
            c->ack = c->rx_next;
            send_segment(c, c->seq, c->ack, 0, 0, TCP_FLAG_ACK);
            if (NEXOS_TCP_TRACE && tcp_trace_window_logs < 24) {
                klog(LOG_INFO, "TCP WINDOW UPDATE ack=%u win=%u drained=%u",
                     c->ack, advertised_window(c), n);
                tcp_trace_window_logs++;
            }
            return n;
        }
        if (c->state == TCP_STATE_CLOSE_WAIT || c->state == TCP_STATE_CLOSED) return 0;
    }
    return 0;
}

void tcp_close(tcp_conn_t *c) {
    if (!c) return;
    if (c->state == TCP_STATE_ESTABLISHED) {
        send_segment(c, c->seq, c->ack, 0, 0, TCP_FLAG_FIN | TCP_FLAG_ACK);
        c->seq++; c->fin_sent = 1; c->state = TCP_STATE_FIN_WAIT;
        uint64_t until = timer_get_ticks() + TCP_CLOSE_MS;
        while (timer_get_ticks() < until && c->state == TCP_STATE_FIN_WAIT) poll_net();
    }
    c->state = TCP_STATE_CLOSED; unregister_conn(c); klog(LOG_INFO, "TCP: connection closed");
}
