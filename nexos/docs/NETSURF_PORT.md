# NexOS NetSurf port

This document records the first implementation baseline. It is deliberately
separate from the existing `kernel/gui/browser_app.c`, which is an HTTP-only
reference application and is not NetSurf.

Baseline date: 2026-10-03  
Upstream branch: `master`  
Source acquisition: `tools/netsurf_fetch_sources.sh`  
Source directory: `ports/netsurf/src/` (generated, not hand-written)

## Selected first-milestone graph

```text
netsurf TARGET=framebuffer
├── libnsfb                         NexOS framebuffer/input surface adapter
├── libcss                          CSS parsing and selection
├── libdom                          DOM
│   └── libhubbub                   HTML5 parser
├── libparserutils                  parser/charset support
├── libwapcaplet                    interned strings
├── libnsutils                      NetSurf utility support
├── libnsgif / libnsbmp             disabled initially; enable after base path
├── libpng                          first remote image format
├── libcurl                         HTTP and HTTPS transfer state machine
├── TLS backend                     certificate-validating backend, TBD by port test
├── zlib                            compression support
└── internal framebuffer font       avoids a freetype dependency initially
```

The following are intentionally disabled for the first build until a real
HTTPS page is visible: Duktape, video, WebGL, WebRTC, JPEG, GIF, BMP, SVG,
JPEGXL, WebP, PDF, Sprite, PSL, cookies persistence, downloads, and browser
profile caches.

`libsvgtiny` is not in the selected graph while SVG is disabled. It remains
listed in the lockfile because upstream's generic framebuffer environment
script names it as a frontend dependency; it is not fetched by the default
minimal fetch set.

## Exact upstream commits

All entries are fetched from `https://github.com/netsurf-browser/<name>.git`.
The fetch script verifies the commit after cloning.

| Component | Commit |
|---|---|
| buildsystem | `0005ae300283ff01c2e2b05e7376b3e55dea21f7` |
| libwapcaplet | `c7c128d3eb3223b216c974471f82e9337fbcf4ba` |
| libparserutils | `6b0cbf086ca8eb8fe74b69f0c9ecf274eb2397ca` |
| libhubbub | `6651b8cf87a4aa87bcdb2ff024a02659cd3f9402` |
| libdom | `f69781e1f062444b5af3f62d431d7d94018da53b` |
| libcss | `499f1c4601ad39942fd1b2204053a387bec9b989` |
| libnsutils | `0bd39060740b6163bd50875326654a722df97eb2` |
| libnsfb | `b701cdce7241c3747ccd78658a365db0983ebe24` |
| libpng | host/system dependency; NexOS port pending |
| libcurl | host/system dependency; NexOS port pending |
| TLS | selection pending portability/build probe |
| netsurf | `39da3c3a40af4566d86500ff3052dfdc7f9a0378` |

The commits above are the shallow-clone heads obtained during this port. The
lock is intentionally immutable for this implementation series; changing a
commit requires updating this document and rerunning the dependency audit.

## Required NexOS adapters

| Adapter | Upstream call surface | NexOS target | Status |
|---|---|---|---|
| memory | `malloc`, `calloc`, `realloc`, `free` | `kmalloc`/`kfree` or hosted runtime wrapper | pending |
| framebuffer | `nsfb_new`, geometry, buffer, claim/update, plotters | `fb.addr`, `fb.pitch`, `fb.width`, `fb.height` | pending |
| input | `nsfb_event`, keyboard/mouse event codes | `keyboard_*`, `mouse_*` | pending |
| timer | `gettimeofday`, monotonic scheduling | NexOS timer/PIT facilities | pending |
| filesystem | resource/config/certificate reads | VFS/RAMFS/FAT32 bridge | pending |
| transport | curl socket callbacks | `dns_resolve`, `tcp_*` through a socket shim | partial |
| TLS | curl SSL backend and certificate verification | selected portable TLS backend | pending |

No adapter is marked complete until it is compiled against the actual selected
upstream headers and exercised by a test.

The first adapter source is now present at
`ports/netsurf/compat/libnsfb/nexos_surface.c`. It uses the existing
`fb.addr/fb.pitch` surface and `keyboard_*`/`mouse_*` APIs and registers the
upstream surface name `nexos`. It is intentionally not included in the normal
NexOS kernel build yet: its correct compile target is the NetSurf/libnsfb
port, where libnsfb's internal surface ABI is available.

The first narrow socket ABI is also present in
`kernel/net/socket_compat.{h,c}` and the syscall dispatcher now implements
only AF_INET/SOCK_STREAM `socket`, `connect`, `sendto`, `recvfrom`, and
`close` over the existing `tcp_*` API. It deliberately leaves server sockets,
`sendmsg`, and `recvmsg` unsupported.

This adapter is not yet TLS-capable. The underlying TCP evidence is concrete:
`kernel/net/tcp.c::tcp_send()` emits a single segment per call,
`tcp_receive()` drops out-of-order data, and the connection has one global
`active_conn`. The socket layer chunks writes to 1400 bytes, but reliable
segmentation/retransmission and multiple connections are still required
before curl/OpenSSL can be considered usable for arbitrary HTTPS traffic.

The upstream fetcher also confirms that NetSurf's OpenSSL-enabled path uses
OpenSSL X509 types directly in `content/fetchers/curl.c` for certificate
information. A portable curl TLS backend such as mbedTLS may be possible with
`NETSURF_USE_OPENSSL=NO`, but that must be proven by a real curl/NetSurf build;
certificate validation must not be disabled as a shortcut.

## Implementation order

1. Fetch and verify this source graph.
2. Build the unmodified framebuffer target on the host as a dependency probe;
   capture the first missing API/library rather than guessing.
3. Define the NexOS hosted execution boundary and minimal allocator/string/
   compiler-runtime ABI needed by the selected source set.
4. Port `libnsfb`'s RAM/plot surface to the existing NexOS framebuffer and
   translate existing keyboard/mouse events.
5. Adapt the event scheduler and resource filesystem calls.
6. Select and port a TLS backend with hostname and certificate validation.
7. Adapt curl's required socket operations to existing DNS/TCP primitives.
8. Cross-build the selected NetSurf libraries and link a NexOS application.
9. Validate DNS → TCP → TLS → certificate → HTTP → HTML/CSS/layout → text /
   PNG rendering in QEMU.

The current repository does not yet satisfy steps 3–9. This is an
implementation status document, not a success claim.

The first NexOS runtime change is now present: `kernel/arch/x86_64/fpu.c`
enables the architectural FPU/SSE state before interrupts. This is required
because the audited NetSurf core uses `float`/`double` layout values, while
the original NexOS kernel was compiled with `-mno-sse -mno-sse2`. A direct
compiler probe confirmed that returning a `double` with those flags fails with
`error: SSE register return with SSE disabled`. XSAVE/FPU state must be added
to process context before NexOS schedules more than one floating-point-using
thread; the current boot path has one runnable init thread.

## Probe evidence

On the first clean host probe, `libwapcaplet`, `libparserutils`, `libcss`,
`libnsutils`, and `libnsfb` compiled with the locked commits. `libhubbub`
stopped before compilation because its upstream Makefile invokes the external
generator `gperf` for `src/treebuilder/element-type.gperf`:

```text
GPERF: src/treebuilder/element-type.gperf
make: gperf: No such file or directory
make: *** ... autogenerated-element-type.c ... Error 127
```

`libdom` then failed only because `libhubbub.pc` and its headers had not been
installed. This is a reproducible build-tool prerequisite, not evidence that
NexOS needs a `gperf` runtime interface. The host has no package manager in
the current environment, so the NexOS port does not vendor or replace this
build generator.
