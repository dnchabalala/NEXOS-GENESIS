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
├── NexOS fetcher boundary          NetSurf fetch callbacks over existing http_get()
├── TLS backend                     existing freestanding Mbed TLS adapter
├── zlib                            compression support (future response decoding)
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
| libcurl | intentionally disabled for NexOS first milestone; its multi/socket ABI is host-oriented |
| TLS | freestanding Mbed TLS is verified independently in native NexOS |
| netsurf | `39da3c3a40af4566d86500ff3052dfdc7f9a0378` |

The commits above are the shallow-clone heads obtained during this port. The
lock is intentionally immutable for this implementation series; changing a
commit requires updating this document and rerunning the dependency audit.

## Required NexOS adapters

| Adapter | Upstream call surface | NexOS target | Status |
|---|---|---|---|
| memory | `malloc`, `calloc`, `realloc`, `free` | `kmalloc`/`kfree` or hosted runtime wrapper | pending |
| framebuffer | `nsfb_new`, geometry, buffer, claim/update, plotters | `fb.addr`, `fb.pitch`, `fb.width`, `fb.height` | adapter present; linked NetSurf use pending |
| input | `nsfb_event`, keyboard/mouse event codes | `keyboard_*`, `mouse_*` | adapter present; linked NetSurf use pending |
| timer | `gettimeofday`, monotonic scheduling | NexOS timer/PIT facilities | pending |
| filesystem | resource/config/certificate reads | VFS/RAMFS/FAT32 bridge | pending |
| transport | NetSurf `fetcher_operation_table` | existing `http_get()` → DNS/TCP/Mbed TLS | adapter ABI compiled; runtime browser path pending |
| TLS | NexOS HTTP boundary | existing freestanding Mbed TLS and embedded trust store | native HTTPS verified |

No adapter is marked complete until it is compiled against the actual selected
upstream headers and exercised by a test.

The first adapter source is now present at
`ports/netsurf/compat/libnsfb/nexos_surface.c`. It uses the existing
`fb.addr/fb.pitch` surface and `keyboard_*`/`mouse_*` APIs and registers the
upstream surface name `nexos`. It is intentionally not included in the normal
NexOS kernel build yet: its correct compile target is the NetSurf/libnsfb
port, where libnsfb's internal surface ABI is available.

The existing narrow socket ABI remains in
`kernel/net/socket_compat.{h,c}` and implements the supported
AF_INET/SOCK_STREAM operations over the current TCP stack. Native HTTPS
does not use host sockets: `kernel/net/http.c` drives NexOS DNS/TCP and the
freestanding Mbed TLS adapter directly, with certificate validation against
the embedded trust bundle.

The upstream fetcher audit confirms that `content/fetchers/curl.c` uses
libcurl's multi interface, fd-set polling, socket callbacks, and host libcurl
TLS configuration. Porting that whole ABI would duplicate the already-proven
NexOS HTTP/TLS boundary. The first NexOS configuration therefore disables
libcurl and registers `content/fetchers/nexos.c` through NetSurf's existing
`fetcher_operation_table`. The adapter emits normal NetSurf header/data/
finished messages and calls the existing `http_get()` implementation; it does
not implement a second browser transport.

## Implementation order

1. Fetch and verify this source graph.
2. Build the unmodified framebuffer target on the host as a dependency probe;
   capture the first missing API/library rather than guessing.
3. Define the NexOS hosted execution boundary and minimal allocator/string/
   compiler-runtime ABI needed by the selected source set.
4. Port `libnsfb`'s RAM/plot surface to the existing NexOS framebuffer and
   translate existing keyboard/mouse events.
5. Adapt the event scheduler and resource filesystem calls.
6. Build the selected host dependency libraries with
   `make -C nexos netsurf-deps` and cross-build the selected NetSurf sources.
7. Exercise a real NetSurf HTTPS fetch in QEMU.
8. Validate DNS → TCP → TLS → certificate → HTTP → HTML/CSS/layout → text /
   PNG rendering in QEMU.

The repository now satisfies the locked dependency and fetcher ABI steps
through `make -C nexos netsurf-deps` and
`make -C nexos netsurf-adapter-check`. The upstream framebuffer sources also
compile, including `content/fetchers/nexos.c`, but the hosted `nsfb`
executable cannot be the NexOS runtime artifact: its link fails because the
NexOS fetcher intentionally references kernel-only `http_get()` and
`http_free()` symbols. The remaining work is to embed the selected NetSurf
core/frontend objects into the freestanding NexOS image and provide the
existing NexOS window/framebuffer/runtime entry points at that boundary.

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

The generator is a host build prerequisite, not evidence that NexOS needs a
`gperf` runtime interface. With `gperf` installed, libhubbub now generates
`autogenerated-element-type.c` normally and the locked dependency graph
builds successfully.

The next concrete link failure is from the standard hosted framebuffer
target:

```text
/usr/bin/ld: content_fetchers_nexos.o: undefined reference to `http_get'
/usr/bin/ld: content_fetchers_nexos.o: undefined reference to `http_free'
```

This occurs because `TARGET=framebuffer` produces a Linux host executable,
while those symbols intentionally belong to the freestanding NexOS kernel.
Adding host shims would conceal the real runtime boundary and duplicate the
transport, so the port must link NetSurf as part of the NexOS image instead.
