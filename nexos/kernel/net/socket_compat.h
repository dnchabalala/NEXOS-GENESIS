/* NexOS — narrow BSD stream-socket adapter over the existing TCP client */
#ifndef NEXOS_SOCKET_COMPAT_H
#define NEXOS_SOCKET_COMPAT_H

#include "../fs/vfs.h"
#include <stdint.h>

vfs_node_t *nexos_socket_create(void);
int nexos_socket_connect(vfs_node_t *node, uint32_t ip, uint16_t port);
int nexos_socket_connected(const vfs_node_t *node);

#endif
