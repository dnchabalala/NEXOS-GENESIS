#!/bin/bash
# NexOS browser-capable QEMU run: host CPU entropy + SLIRP DNS.
set -e
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DISPLAY_MODE="${DISPLAY_MODE:-sdl}"
MEMORY="${MEMORY:-2048}"

if [ ! -e /dev/kvm ]; then
    echo "[QEMU] browser run requires /dev/kvm and host CPU features (RDRAND)" >&2
    exit 2
fi

exec qemu-system-x86_64 \
    -machine q35 -enable-kvm -cpu host -m "${MEMORY}" \
    -serial stdio -display "${DISPLAY_MODE}" -no-reboot -vga virtio \
    -cdrom "${ROOT_DIR}/build/nexos.iso" -boot d \
    -netdev user,id=net0 -device rtl8139,netdev=net0 \
    "$@"
