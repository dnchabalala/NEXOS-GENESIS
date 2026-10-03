#!/usr/bin/env bash
set -euo pipefail

NEXOS_DIR=$(cd "$(dirname "$0")/.." && pwd)
cd "$NEXOS_DIR"
SOURCE_DIR=${NETSURF_SOURCE_DIR:-ports/netsurf/src}
ROOT="$SOURCE_DIR/netsurf"
TLS=ports/tls/src/mbedtls-3.6.7
OUT=${NETSURF_NATIVE_BUILD:-build/netsurf-native}
PREFIX=${NETSURF_HOST_PREFIX:-ports/netsurf/host-prefix}
mkdir -p "$OUT"

mapfile -t sources < <(PKG_CONFIG_PATH="$PWD/$PREFIX/lib/pkgconfig" \
    make -s -C "$ROOT" TARGET=framebuffer -pn 2>/dev/null |
    awk '$1=="S_COMMON" && $2==":=" {sub(/^S_COMMON := /,"",$0); print}' |
    tr ' ' '\n' | while read -r src; do
        case "$src" in *.c) ;; *) continue;; esac
        case "$src" in content/fetchers/about/*|content/fetchers/file/*|utils/file.c|utils/filepath.c|desktop/hotlist.c|desktop/global_history.c|desktop/save_*|desktop/version.c|content/handlers/image/*) continue;; esac
        printf '%s\n' "$src"
    done)

# S_COMMON is not the complete freestanding browser engine.  The desktop
# browser sources are kept in S_BROWSER by upstream and are normally pulled
# in by a platform frontend executable.  The NexOS frontend is a native
# embedding, so add the engine-side portion explicitly; hosted frontend
# sources remain excluded below.
browser_sources=(
    desktop/bitmap.c desktop/browser.c desktop/browser_window.c
    desktop/browser_history.c desktop/frames.c desktop/cw_helper.c
    desktop/netsurf.c desktop/gui_factory.c desktop/selection.c
    desktop/textinput.c desktop/download.c desktop/save_text.c
    content/handlers/image/image.c content/handlers/image/image_cache.c
)
for src in "${browser_sources[@]}"; do
    case " ${sources[*]} " in
        *" $src "*) ;;
        *) sources+=("$src") ;;
    esac
done

COMMON=(-std=c11 -ffreestanding -fno-stack-protector -fno-pic -mstackrealign -mno-red-zone
    -mno-mmx -msse -msse2 -mfpmath=sse -O2 -m64 -mcmodel=kernel -nostdlib -nostdinc
    -ffunction-sections -fdata-sections -fcommon -I. -Ikernel -Ikernel/include
    -Iports/tls -Iports/tls/include -I"$TLS/include" -I"$PREFIX/include"
    -I"$ROOT" -I"$ROOT/include" -I"$ROOT/content/handlers" -I"$ROOT/content"
    -I"$SOURCE_DIR/libwapcaplet/include" -I"$SOURCE_DIR/libparserutils/include"
    -I"$SOURCE_DIR/libhubbub/include" -I"$SOURCE_DIR/libnsutils/include"
    -I"$SOURCE_DIR/libcss/include" -I"$SOURCE_DIR/libdom/include" -I"$SOURCE_DIR/libnsfb/include"
    -D_DEFAULT_SOURCE -DWITH_NEXOS -DNETSURF_USE_NEXOS=YES
    '-DNETSURF_BUILTIN_LOG_FILTER="level:WARNING"'
    '-DNETSURF_BUILTIN_VERBOSE_FILTER="level:VERBOSE"'
    -DMBEDTLS_CONFIG_FILE="$PWD/$TLS/../mbedtls_nexos_config.h" -Wno-unused-parameter
    -Wno-unused-variable -Wno-format)

objects=()
for src in "${sources[@]}"; do
    obj="$OUT/${src//\//_}.o"
    echo "[NETSURF-NATIVE CC] $src"
    gcc "${COMMON[@]}" -c "$ROOT/$src" -o "$obj"
    objects+=("$obj")
done
ar rcs "$OUT/libnetsurf-native.a" "${objects[@]}"
echo "NetSurf freestanding object archive: $OUT/libnetsurf-native.a"
