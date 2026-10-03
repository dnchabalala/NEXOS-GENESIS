#!/bin/bash
# NexOS — tools/build_iso.sh | Assemble bootable ISO | MIT License

set -e

if command -v grub-mkrescue >/dev/null 2>&1; then
    GRUB_MKRESCUE=grub-mkrescue
elif command -v grub2-mkrescue >/dev/null 2>&1; then
    GRUB_MKRESCUE=grub2-mkrescue
else
    echo "[build_iso.sh] Error: grub-mkrescue/grub2-mkrescue is required." >&2
    echo "[build_iso.sh] Install GRUB PC BIOS modules and xorriso, then retry." >&2
    exit 127
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$SCRIPT_DIR/.."
BUILD_DIR="$ROOT_DIR/build"
ISO_DIR="$BUILD_DIR/iso"
KERNEL="$BUILD_DIR/nexos.kernel"
ISO="$BUILD_DIR/nexos.iso"

echo "[build_iso.sh] Creating ISO structure..."
mkdir -p "$ISO_DIR/boot/grub"

# Copy kernel
cp "$KERNEL" "$ISO_DIR/boot/nexos.kernel"

# Copy GRUB config
cp "$ROOT_DIR/boot/grub/grub.cfg" "$ISO_DIR/boot/grub/grub.cfg"

# Build ISO with grub-mkrescue
echo "[build_iso.sh] Running grub-mkrescue..."
"$GRUB_MKRESCUE" -o "$ISO" "$ISO_DIR"

echo "[build_iso.sh] ISO created: $ISO"
ls -lh "$ISO"
