#!/bin/sh
set -eu

NEXOS_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SOURCE_DIR=${NETSURF_SOURCE_DIR:-"$NEXOS_DIR/ports/netsurf/src"}
NETSURF_DIR="$SOURCE_DIR/netsurf"

NETSURF_SOURCE_DIR="$SOURCE_DIR" \
    "$NEXOS_DIR/tools/netsurf_apply_nexos_fetcher.sh"

INCLUDES="-I$NETSURF_DIR -I$NETSURF_DIR/include \
-I$NETSURF_DIR/content/handlers \
-I$SOURCE_DIR/libwapcaplet/include -I$SOURCE_DIR/libnsutils/include \
-I$SOURCE_DIR/libdom/include -I$SOURCE_DIR/libcss/include \
-I$SOURCE_DIR/libparserutils/include -I$SOURCE_DIR/libhubbub/include \
-I$SOURCE_DIR/libnsfb/include -I$NEXOS_DIR"

# This validates the actual NetSurf fetcher ABI and registration path without
# pretending that host libcurl or host sockets are a NexOS runtime test.
# shellcheck disable=SC2086
gcc -std=c11 -D_DEFAULT_SOURCE -DWITH_NEXOS -fsyntax-only $INCLUDES \
    "$NETSURF_DIR/content/fetchers/nexos.c" \
    "$NETSURF_DIR/content/fetch.c"

echo "NetSurf NexOS fetcher ABI check passed"
