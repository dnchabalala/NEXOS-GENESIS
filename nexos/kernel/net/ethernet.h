/* NexOS - kernel/net/ethernet.h | Ethernet layer | MIT License */
#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>

#define ETH_ADDR_LEN  6
#define ETH_HDR_LEN   14
#define ETH_TYPE_ARP  0x0806
#define ETH_TYPE_IP   0x0800

/* Active Ethernet configuration, stored as big-endian uint32_t values. */
extern uint8_t  eth_our_mac[ETH_ADDR_LEN];
extern uint32_t eth_our_ip;
extern uint32_t eth_netmask;
extern uint32_t eth_gw_ip;
extern uint32_t eth_dns_ip;

void ethernet_init(void);
int  ethernet_send(const uint8_t dst_mac[ETH_ADDR_LEN], uint16_t ethertype,
                   const uint8_t *payload, uint16_t len);
void ethernet_receive(const uint8_t *frame, uint16_t len);

#endif
