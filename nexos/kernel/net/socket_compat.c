/* NexOS — kernel/net/socket_compat.c | stream socket adapter | MIT License */
#include "socket_compat.h"
#include "tcp.h"
#include "../mm/heap.h"

typedef struct {
    tcp_conn_t tcp;
    int connected;
} nexos_socket_t;

static uint32_t socket_read(vfs_node_t *node, uint64_t offset,
                            uint32_t size, uint8_t *buf) {
    nexos_socket_t *socket = (nexos_socket_t *)node->priv;
    (void)offset;
    if (!socket || !socket->connected || size == 0) return 0;
    if (size > TCP_RX_BUF_SIZE) size = TCP_RX_BUF_SIZE;
    return (uint32_t)tcp_recv(&socket->tcp, buf, (uint16_t)size, 1000);
}

static uint32_t socket_write(vfs_node_t *node, uint64_t offset,
                             uint32_t size, const uint8_t *buf) {
    nexos_socket_t *socket = (nexos_socket_t *)node->priv;
    uint32_t sent = 0;
    (void)offset;
    if (!socket || !socket->connected) return 0;

    /* The current NexOS TCP implementation accepts one segment per call.
     * Keep the adapter within a conservative Ethernet payload until TCP
     * segmentation/windowing is implemented. */
    while (sent < size) {
        uint16_t chunk = (uint16_t)(size - sent);
        if (chunk > 1400) chunk = 1400;
        if (tcp_send(&socket->tcp, buf + sent, chunk) < 0) break;
        sent += chunk;
    }
    return sent;
}

static void socket_close(vfs_node_t *node) {
    nexos_socket_t *socket = (nexos_socket_t *)node->priv;
    if (socket) {
        if (socket->connected) tcp_close(&socket->tcp);
        kfree(socket);
    }
    kfree(node);
}

vfs_node_t *nexos_socket_create(void) {
    vfs_node_t *node = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
    nexos_socket_t *socket = (nexos_socket_t *)kmalloc(sizeof(nexos_socket_t));
    if (!node || !socket) {
        if (node) kfree(node);
        if (socket) kfree(socket);
        return 0;
    }

    uint8_t *nzero = (uint8_t *)node;
    for (uint32_t i = 0; i < sizeof(vfs_node_t); i++) nzero[i] = 0;
    uint8_t *szero = (uint8_t *)socket;
    for (uint32_t i = 0; i < sizeof(nexos_socket_t); i++) szero[i] = 0;

    node->type = VFS_NODE_SOCKET;
    node->priv = socket;
    node->read = socket_read;
    node->write = socket_write;
    node->close = socket_close;
    return node;
}

int nexos_socket_connect(vfs_node_t *node, uint32_t ip, uint16_t port) {
    nexos_socket_t *socket;
    if (!node || !(node->type & VFS_NODE_SOCKET)) return -1;
    socket = (nexos_socket_t *)node->priv;
    if (!socket || socket->connected) return -1;
    if (tcp_connect(&socket->tcp, ip, port) < 0) return -1;
    socket->connected = 1;
    return 0;
}

int nexos_socket_connected(const vfs_node_t *node) {
    const nexos_socket_t *socket;
    if (!node || !(node->type & VFS_NODE_SOCKET)) return 0;
    socket = (const nexos_socket_t *)node->priv;
    return socket && socket->connected &&
           socket->tcp.state == TCP_STATE_ESTABLISHED;
}
