#!/usr/bin/env bash
set -euo pipefail

NEXOS_DIR=$(cd "$(dirname "$0")/.." && pwd)
cd "$NEXOS_DIR"
SOURCE_DIR=${NETSURF_SOURCE_DIR:-ports/netsurf/src}
TLS=ports/tls/src/mbedtls-3.6.7
OUT=${NETSURF_NATIVE_BUILD:-build/netsurf-native}
PREFIX=${NETSURF_HOST_PREFIX:-ports/netsurf/host-prefix}
mkdir -p "$OUT"

COMMON=(-std=c11 -ffreestanding -fno-stack-protector -fno-pic -mstackrealign -mno-red-zone
    -mno-mmx -msse -msse2 -mfpmath=sse -O2 -m64 -mcmodel=kernel -nostdlib -nostdinc
    -ffunction-sections -fdata-sections -fcommon -I. -Ikernel -Ikernel/include
    -Iports/tls -Iports/tls/include -I"$TLS/include" -I"$PREFIX/include"
    -I"$SOURCE_DIR/netsurf" -I"$SOURCE_DIR/netsurf/include"
    -I"$SOURCE_DIR/netsurf/content/handlers" -I"$SOURCE_DIR/netsurf/content"
    -I"$SOURCE_DIR/libwapcaplet/include" -I"$SOURCE_DIR/libparserutils/include"
    -I"$SOURCE_DIR/libhubbub/include" -I"$SOURCE_DIR/libnsutils/include"
    -I"$SOURCE_DIR/libcss/include" -I"$SOURCE_DIR/libdom/include"
    -I"$SOURCE_DIR/libnsfb/include"
    -D_DEFAULT_SOURCE -DWITH_NEXOS -DNETSURF_USE_NEXOS=YES
    -Wno-unused-parameter -Wno-unused-variable -Wno-format)

gcc "${COMMON[@]}" -c ports/netsurf/compat/nexos_frontend.c \
    -o "$OUT/nexos_frontend.o"
ar rcs "$OUT/libnetsurf-native-frontend.a" "$OUT/nexos_frontend.o"
echo "NetSurf NexOS frontend archive: $OUT/libnetsurf-native-frontend.a"
