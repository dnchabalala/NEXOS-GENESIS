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
    -I"$SOURCE_DIR/netsurf" -I"$SOURCE_DIR/netsurf/include" -I"$SOURCE_DIR/netsurf/utils"
    -I"$SOURCE_DIR/libwapcaplet/include" -I"$SOURCE_DIR/libwapcaplet/src"
    -I"$SOURCE_DIR/libparserutils/include" -I"$SOURCE_DIR/libparserutils/src"
    -I"$SOURCE_DIR/libhubbub/include" -I"$SOURCE_DIR/libhubbub/src"
    -I"$SOURCE_DIR/libnsutils/include" -I"$SOURCE_DIR/libnsutils/src"
    -I"$SOURCE_DIR/libcss/include" -I"$SOURCE_DIR/libcss/src"
    -I"$SOURCE_DIR/libdom/include" -I"$SOURCE_DIR/libdom/src" -I"$SOURCE_DIR/libdom/binding"
    -I"$SOURCE_DIR/libnsfb/include" -I"$SOURCE_DIR/libnsfb/src"
    -D_DEFAULT_SOURCE -DNDEBUG -DWITH_NEXOS -DNETSURF_USE_NEXOS=YES
    -DWITHOUT_ICONV_FILTER
    '-DSIZE_MAX=((size_t)-1)'
    -Wno-unused-parameter -Wno-unused-variable -Wno-format)

compile_tree() {
    local name=$1 dir=$2 archive="$OUT/lib${1}-native.a"
    local objects=()
    local tree_includes=(-I"$dir/include" -I"$dir/src")
    while IFS= read -r src; do
        case "$src" in
            */test/*|*/tests/*|*/example/*|*/examples/*|*/demo/*|*/perf/*|*/tools/*) continue;;
            */surface/sdl.c|*/surface/vnc.c|*/surface/wld.c|*/surface/x.c) continue;;
            */libnsutils/src/unistd.c) continue;;
            */css_property_parser_gen.c) continue;;
            */libdom/bindings/xml/*) continue;;
            */libnsfb/src/dump.c) continue;;
            */libnsfb/src/plot/1bpp.c) continue;;
            */libnsfb/src/plot/16bpp.c|*/libnsfb/src/plot/24bpp.c|*/libnsfb/src/plot/8bpp.c) continue;;
            */libnsfb/src/plot/common.c|*/libnsfb/src/plot/32bpp-common.c) continue;;
        esac
        local rel=${src#"$dir/"}
        local obj="$OUT/${name}_${rel//\//_}.o"
        if [[ -f "$obj" && "$obj" -nt "$src" ]]; then
            objects+=("$obj")
            continue
        fi
        echo "[NETSURF-NATIVE CC] $name/$rel"
        gcc "${tree_includes[@]}" "${COMMON[@]}" -c "$src" -o "$obj"
        objects+=("$obj")
    done < <(find "$dir" -type f -name '*.c' | sort)
    ((${#objects[@]})) || { echo "no sources found for $name" >&2; exit 1; }
    ar rcs "$archive" "${objects[@]}"
    echo "NetSurf freestanding dependency archive: $archive"
}

compile_tree wapcaplet "$SOURCE_DIR/libwapcaplet"
compile_tree parserutils "$SOURCE_DIR/libparserutils"
compile_tree hubbub "$SOURCE_DIR/libhubbub"
compile_tree nsutils "$SOURCE_DIR/libnsutils"
compile_tree css "$SOURCE_DIR/libcss"
compile_tree dom "$SOURCE_DIR/libdom"
compile_tree nsfb "$SOURCE_DIR/libnsfb"
