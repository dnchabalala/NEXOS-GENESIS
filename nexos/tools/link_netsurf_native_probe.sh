#!/usr/bin/env bash
set -euo pipefail

NEXOS_DIR=$(cd "$(dirname "$0")/.." && pwd)
cd "$NEXOS_DIR"
OUT=${NETSURF_NATIVE_BUILD:-build/netsurf-native}

ld -r -o "$OUT/netsurf-native-link.o" --whole-archive \
    "$OUT/libnetsurf-native.a" \
    "$OUT/libnetsurf-native-frontend.a" \
    "$OUT/libwapcaplet-native.a" \
    "$OUT/libparserutils-native.a" \
    "$OUT/libhubbub-native.a" \
    "$OUT/libnsutils-native.a" \
    "$OUT/libcss-native.a" \
    "$OUT/libdom-native.a" \
    "$OUT/libnsfb-native.a" \
    --no-whole-archive

echo "NetSurf freestanding archive link probe: $OUT/netsurf-native-link.o"
