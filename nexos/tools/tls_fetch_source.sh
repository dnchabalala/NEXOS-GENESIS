#!/bin/sh
set -eu
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
NEXOS_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
LOCK_FILE="$NEXOS_DIR/ports/tls/sources.lock"
SOURCE_DIR="${TLS_SOURCE_DIR:-$NEXOS_DIR/ports/tls/src}"
TMP_DIR="${TMPDIR:-/tmp}/nexos-tls-fetch"
mkdir -p "$SOURCE_DIR" "$TMP_DIR"
while IFS='|' read -r name url sha version; do
    case "$name" in ''|'#'*) continue ;; esac
    archive="$TMP_DIR/$name-$version.tar.bz2"
    curl -fL --retry 3 -o "$archive" "$url"
    printf '%s  %s\n' "$sha" "$archive" | sha256sum -c -
    rm -rf "$SOURCE_DIR/$name-$version"
    mkdir -p "$SOURCE_DIR/$name-$version"
    tar -xjf "$archive" -C "$SOURCE_DIR/$name-$version" --strip-components=1
    echo "verified $name $version $sha"
done < "$LOCK_FILE"
