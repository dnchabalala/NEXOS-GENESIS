/* NexOS - kernel/net/net.c | Network subsystem init | MIT License */
#include "net.h"
#include "ethernet.h"
#include "arp.h"
#include "icmp.h"
#include "udp.h"
#include "tcp.h"
#include "dhcp.h"
#include "../kernel.h"
#include "../drivers/rtl8139.h"

/* ────────────────────────────────────────────────────────────────────────────
 * net_init() — boot-time network stack bringup.
 *
 * IMPORTANT: rtl8139_init() MUST be called before net_init().
 *   rtl8139_init() → detects NIC, allocates RX/TX buffers, registers eth0.
 *   net_init()     → hooks Ethernet callback, probes gateway, pings, logs.
 * ──────────────────────────────────────────────────────────────────────────── */
void net_init(void) {
    if (!rtl8139_found()) {
        klog(LOG_WARN, "net: RTL8139 not detected — networking disabled");
        klog(LOG_WARN, "     (hint: check PCI scan covers q35 bus 1)");
        return;
    }

    /* ── Layer 2: wire Ethernet RX callback, read MAC ─────────────────────── */
    ethernet_init();
    if (dhcp_configure() < 0) {
        klog(LOG_WARN, "net: DHCP failed; Ethernet link is up but has no IPv4 configuration");
        return;
    }

    klog(LOG_INFO, "eth0  %d.%d.%d.%d  gw %d.%d.%d.%d",
         (eth_our_ip >> 24) & 255, (eth_our_ip >> 16) & 255,
         (eth_our_ip >> 8) & 255, eth_our_ip & 255,
         (eth_gw_ip >> 24) & 255, (eth_gw_ip >> 16) & 255,
         (eth_gw_ip >> 8) & 255, eth_gw_ip & 255);

    /* ── Layer 3/ARP: resolve gateway so we can route IP packets ─────────── */
    uint8_t gw_mac[6] = {0};
    klog(LOG_INFO, "ARP: probing configured gateway...");
    if (arp_request(eth_gw_ip, gw_mac)) {
        klog(LOG_INFO,
             "ARP: gateway -> %02x:%02x:%02x:%02x:%02x:%02x",
             gw_mac[0], gw_mac[1], gw_mac[2],
             gw_mac[3], gw_mac[4], gw_mac[5]);
    } else {
        klog(LOG_WARN, "ARP: no reply from 10.0.2.2 (QEMU user-net down?)");
        /* Non-fatal — stack is still usable for LAN */
    }

    /* ── ICMP: send one echo to verify IP reachability ───────────────────── */
    klog(LOG_INFO, "ICMP: pinging configured gateway...");
    int rtt = icmp_send_echo(eth_gw_ip);
    if (rtt >= 0)
        klog(LOG_INFO, "ICMP: gateway reply in %d ms — IP layer OK", rtt);
    else
        klog(LOG_WARN, "ICMP: no reply from gateway (unreachable)");

    klog(LOG_INFO, "Network stack ready: Ethernet / DHCP / ARP / IPv4 / ICMP / "
                   "UDP / TCP / DNS / HTTP");
}
