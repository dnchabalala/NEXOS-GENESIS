#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
NEXOS_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
SOURCE_DIR=${TLS_SOURCE_DIR:-$NEXOS_DIR/ports/tls/src/mbedtls-3.6.7}
CONFIG_FILE=${TLS_CONFIG_FILE:-$NEXOS_DIR/ports/tls/mbedtls_nexos_config.h}

# Mbed TLS' upstream Makefile is reused; only its compiler environment is
# changed.  Cleaning is intentional because the same source tree is also used
# by the hosted HTTPS regression target with different flags.
make -C "$SOURCE_DIR" clean >/dev/null
TLS_CFLAGS="-std=c11 -ffreestanding -fno-stack-protector -fno-pic \
-mno-red-zone -mno-mmx -mno-sse -mno-sse2 -m64 -mcmodel=kernel \
-nostdlib -nostdinc -I$NEXOS_DIR/ports/tls/include \
-I$NEXOS_DIR/kernel/include -I$SOURCE_DIR/include -I$SOURCE_DIR/library \
-I$NEXOS_DIR/ports/tls -DMBEDTLS_CONFIG_FILE=\\\"$CONFIG_FILE\\\" \
-Wall -Wextra -O2"
make -C "$SOURCE_DIR" -j2 lib CFLAGS="$TLS_CFLAGS"
