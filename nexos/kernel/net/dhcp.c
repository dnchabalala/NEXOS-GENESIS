/* NexOS — kernel/net/dhcp.c | DHCPv4 client | MIT License */
#include "dhcp.h"
#include "ethernet.h"
#include "udp.h"
#include "../drivers/rtl8139.h"
#include "../drivers/timer.h"
#include "../kernel.h"

#define DHCP_CLIENT_PORT 68
#define DHCP_SERVER_PORT 67
#define DHCP_MAGIC       0x63825363U
#define DHCP_DISCOVER    1
#define DHCP_OFFER       2
#define DHCP_REQUEST     3
#define DHCP_ACK         5
#define DHCP_TIMEOUT_MS  5000

static volatile uint32_t dhcp_xid;
static volatile int dhcp_offered;
static volatile int dhcp_bound;
static uint32_t offered_ip;
static uint32_t offered_server;

static uint16_t dhcp_checksum(const uint8_t *p, int len) {
    uint32_t sum = 0;
    for (int i = 0; i + 1 < len; i += 2) sum += (p[i] << 8) | p[i + 1];
    if (len & 1) sum += p[len - 1] << 8;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

static void dhcp_put32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);  p[3] = (uint8_t)v;
}

static int dhcp_send(uint8_t type, uint32_t requested, uint32_t server) {
    uint8_t bootp[548];
    uint8_t broadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    for (int i = 0; i < (int)sizeof(bootp); i++) bootp[i] = 0;

    bootp[0] = 1; bootp[1] = 1; bootp[2] = 6;
    dhcp_put32(bootp + 4, dhcp_xid);
    bootp[10] = 0x80; bootp[11] = 0x00;
    for (int i = 0; i < 6; i++) bootp[28 + i] = eth_our_mac[i];
    dhcp_put32(bootp + 236, DHCP_MAGIC);

    int n = 240;
    bootp[n++] = 53; bootp[n++] = 1; bootp[n++] = type;
    if (type == DHCP_REQUEST) {
        bootp[n++] = 50; bootp[n++] = 4; dhcp_put32(bootp + n, requested); n += 4;
        bootp[n++] = 54; bootp[n++] = 4; dhcp_put32(bootp + n, server); n += 4;
    }
    bootp[n++] = 55; bootp[n++] = 3;
    bootp[n++] = 1; bootp[n++] = 3; bootp[n++] = 6;
    bootp[n++] = 255;

    uint8_t ip[20 + 8 + sizeof(bootp)];
    for (int i = 0; i < (int)sizeof(ip); i++) ip[i] = 0;
    int ip_len = 20 + 8 + n;
    ip[0] = 0x45; ip[2] = (uint8_t)(ip_len >> 8); ip[3] = (uint8_t)ip_len;
    ip[8] = 64; ip[9] = 17;
    ip[16] = 255; ip[17] = 255; ip[18] = 255; ip[19] = 255;
    uint16_t ck = dhcp_checksum(ip, 20);
    ip[10] = (uint8_t)(ck >> 8); ip[11] = (uint8_t)ck;
    ip[21] = DHCP_CLIENT_PORT; ip[23] = DHCP_SERVER_PORT;
    ip[24] = (uint8_t)((8 + n) >> 8); ip[25] = (uint8_t)(8 + n);
    for (int i = 0; i < n; i++) ip[28 + i] = bootp[i];
    return ethernet_send(broadcast, ETH_TYPE_IP, ip, (uint16_t)ip_len);
}

static void dhcp_option(const uint8_t *opts, int len, uint8_t wanted,
                        uint8_t *out, int out_len) {
    int i = 0;
    while (i < len) {
        uint8_t tag = opts[i++];
        if (tag == 0) continue;
        if (tag == 255 || i >= len) break;
        uint8_t size = opts[i++];
        if (i + size > len) break;
        if (tag == wanted) {
            int copy = size < out_len ? size : out_len;
            for (int j = 0; j < copy; j++) out[j] = opts[i + j];
            return;
        }
        i += size;
    }
}

static uint32_t dhcp_read_ip(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static void dhcp_udp_handler(uint32_t src_ip, uint16_t src_port,
                             const uint8_t *data, uint16_t len) {
    (void)src_ip; (void)src_port;
    if (len < 244 || data[0] != 2 || data[1] != 1 || data[2] != 6) return;
    if (dhcp_read_ip(data + 4) != dhcp_xid) return;

    uint32_t yiaddr = dhcp_read_ip(data + 16);
    const uint8_t *opts = data + 240;
    int opt_len = len - 240;
    uint8_t msg = 0, server_bytes[4] = {0};
    dhcp_option(opts, opt_len, 53, &msg, 1);
    dhcp_option(opts, opt_len, 54, server_bytes, 4);
    uint32_t server = dhcp_read_ip(server_bytes);

    if (msg == DHCP_OFFER && !dhcp_offered) {
        offered_ip = yiaddr; offered_server = server; dhcp_offered = 1;
        dhcp_send(DHCP_REQUEST, offered_ip, offered_server);
        return;
    }
    if (msg == DHCP_ACK) {
        uint8_t mask[4] = {0}, router[4] = {0}, dns[4] = {0};
        dhcp_option(opts, opt_len, 1, mask, 4);
        dhcp_option(opts, opt_len, 3, router, 4);
        dhcp_option(opts, opt_len, 6, dns, 4);
        eth_our_ip  = yiaddr;
        eth_netmask = dhcp_read_ip(mask);
        eth_gw_ip   = dhcp_read_ip(router);
        eth_dns_ip  = dhcp_read_ip(dns);
        dhcp_bound  = 1;
    }
}

int dhcp_configure(void) {
    dhcp_xid = (uint32_t)timer_get_ticks() ^ 0x4E584F53U;
    dhcp_offered = 0; dhcp_bound = 0;
    udp_register(DHCP_CLIENT_PORT, dhcp_udp_handler);
    klog(LOG_INFO, "DHCP: requesting IPv4 configuration...");
    if (dhcp_send(DHCP_DISCOVER, 0, 0) < 0) {
        udp_unregister(DHCP_CLIENT_PORT); return -1;
    }
    uint64_t deadline = timer_get_ticks() + DHCP_TIMEOUT_MS;
    while (timer_get_ticks() < deadline && !dhcp_bound) rtl8139_receive();
    udp_unregister(DHCP_CLIENT_PORT);
    if (!dhcp_bound) return -1;
    klog(LOG_INFO, "DHCP: lease acquired");
    return 0;
}
