#!/bin/sh
# Static/compile-level regression checks for the freestanding TCP transport.
# Wire-level tests run in QEMU; these checks prevent the old single-segment
# implementation from silently returning during ordinary host builds.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TCP="$ROOT/kernel/net/tcp.c"
HDR="$ROOT/kernel/net/tcp.h"

grep -q '#define TCP_MSS[[:space:]]\+1460' "$HDR"
grep -q '#define TCP_MAX_CONNECTIONS[[:space:]]\+8' "$HDR"
grep -q 'static tcp_conn_t \*connections\[TCP_MAX_CONNECTIONS\]' "$TCP"
grep -q 'TCP_MAX_RETRIES' "$TCP"
grep -q 'promote_ooo' "$TCP"
grep -q 'checksum(src_ip, eth_our_ip' "$TCP"
grep -q 'while (sent < len)' "$TCP"

make -C "$ROOT" check >/dev/null
echo "TCP reliability regression checks passed"
