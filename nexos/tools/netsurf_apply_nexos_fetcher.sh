#!/bin/sh
set -eu

NEXOS_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SOURCE_DIR=${NETSURF_SOURCE_DIR:-"$NEXOS_DIR/ports/netsurf/src"}

test -d "$SOURCE_DIR/netsurf" || {
    echo "NetSurf source tree is missing: $SOURCE_DIR" >&2
    exit 1
}

# Keep the NexOS transport adapter in the NexOS tree. The fetched upstream
# checkout is generated state and receives this file as part of netsurf-config.
cp "$NEXOS_DIR/ports/netsurf/nexos_fetcher.c" \
   "$SOURCE_DIR/netsurf/content/fetchers/nexos.c"

NETSURF_DIR="$SOURCE_DIR/netsurf"

if ! grep -q 'feature_enabled,NEXOS' "$NETSURF_DIR/Makefile"; then
    perl -0pi -e \
        's#\$\(eval \$\(call feature_switch,DUKTAPE,Javascript \(Duktape\),,,,,\)\)#$&\n\$\(eval \$\(call feature_enabled,NEXOS,-DWITH_NEXOS,,NexOS HTTP fetcher\)\)#' \
        "$NETSURF_DIR/Makefile"
fi

if ! grep -q 'S_FETCHERS_\$(NETSURF_USE_NEXOS)' \
        "$NETSURF_DIR/content/fetchers/Makefile"; then
    perl -0pi -e \
        's#(S_FETCHERS_\$\(NETSURF_USE_CURL\) += curl\.c\n)#$1S_FETCHERS_\$\(NETSURF_USE_NEXOS\) += nexos.c\n#' \
        "$NETSURF_DIR/content/fetchers/Makefile"
fi

if ! grep -q 'fetch_nexos_register' "$NETSURF_DIR/content/fetch.c"; then
    perl -0pi -e \
        's/#include "content\/fetchers\/curl\.h"/#ifdef WITH_CURL\n#include "content\/fetchers\/curl.h"\n#endif\n#ifdef WITH_NEXOS\nextern nserror fetch_nexos_register\(void\);\n#endif/' \
        "$NETSURF_DIR/content/fetch.c"
    # Insert the NexOS registration immediately before the first data
    # fetcher registration. This is kept separate from the curl block so the
    # upstream curl implementation remains untouched and selectable.
    perl -0pi -e \
        's/\n\tret = fetch_data_register\(\);/\n#ifdef WITH_NEXOS\n\tret = fetch_nexos_register\(\);\n\tif (ret != NSERROR_OK) {\n\t\treturn ret;\n\t}\n#endif\n\n\tret = fetch_data_register\(\);/' \
        "$NETSURF_DIR/content/fetch.c"
fi

echo "installed NexOS NetSurf fetcher adapter"
