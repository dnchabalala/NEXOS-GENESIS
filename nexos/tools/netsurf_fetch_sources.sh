#!/bin/sh
# Fetch the exact upstream sources listed by ports/netsurf/sources.lock.
# This only acquires source; it does not claim that NetSurf builds for NexOS.
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
NEXOS_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
LOCK_FILE="$NEXOS_DIR/ports/netsurf/sources.lock"
SOURCE_DIR="${NETSURF_SOURCE_DIR:-$NEXOS_DIR/ports/netsurf/src}"

mkdir -p "$SOURCE_DIR"

while IFS='|' read -r name url commit required; do
    case "$name" in ''|'#'*) continue ;; esac
    case "$required" in no-*) continue ;; esac

    destination="$SOURCE_DIR/$name"
    if [ ! -d "$destination/.git" ]; then
        git clone --no-tags "$url" "$destination"
    fi
    git -C "$destination" fetch --no-tags origin "$commit"
    git -C "$destination" checkout --detach "$commit"
    actual=$(git -C "$destination" rev-parse HEAD)
    if [ "$actual" != "$commit" ]; then
        echo "netsurf source verification failed: $name" >&2
        exit 1
    fi
    echo "verified $name $actual"
done < "$LOCK_FILE"

echo "NetSurf source graph is available at $SOURCE_DIR"
