# NetSurf on NexOS: first interface audit

Status: source audit completed 2026-10-03. No NetSurf source or dependency is
currently present in this repository. The upstream source used for this audit
was the current `netsurf-browser/netsurf` checkout and `libnsfb` checkout in
`/tmp`.

## Result in one sentence

The correct first target is NetSurf's existing framebuffer frontend
(`TARGET=framebuffer`, executable `nsfb`/`netsurf-fb`), but it cannot be linked
into the current NexOS kernel: NexOS is a freestanding, single-address-space
kernel with no hosted libc/userspace socket ABI, and its internal TCP client
does not provide TLS.

The existing `kernel/gui/browser_app.c` is therefore not a NetSurf port. It is
a bounded HTTP-only HTML stripper/renderer. In particular, its source says
`HTTPS/TLS is not implemented yet`, rejects every URL except `http://`, and
calls `http_get()`; it must not be counted as milestone progress for NetSurf.

## Upstream component matrix

| Component | Upstream evidence | First milestone | Classification |
|---|---|---:|---|
| NetSurf core browser/layout | `desktop/`, `content/`, `include/netsurf/`; `browser_window_create()`, `browser_window_navigate()` | required | `[PORT][BLOCKER]` |
| HTML5 parser | `libhubbub`, used through `utils/libdom.c` and content handlers | required | `[PORT][BLOCKER]` |
| CSS parser/selection | `libcss`; included by `desktop/print.h`, layout code and `Makefile` | required | `[PORT][BLOCKER]` |
| DOM/string support | `libdom`, `libwapcaplet`, `libparserutils` | required | `[PORT][BLOCKER]` |
| HTTP/HTTPS fetch | `content/fetch.c`, `utils/time.c`, curl fetcher; `Makefile.defaults` sets `NETSURF_USE_CURL := YES` | required | `[ADAPT][BLOCKER]` |
| Certificate validation | `NETSURF_USE_OPENSSL` in `Makefile.defaults`; curl/OpenSSL integration in root `Makefile` | required | `[PORT][BLOCKER]` |
| PNG/GIF/JPEG/BMP images | `libpng`, `libnsgif`, `libjpeg`, `libnsbmp` and `frontends/framebuffer/bitmap.c` | at least one image | `[PORT][BLOCKER]` |
| JavaScript/Duktape | `NETSURF_USE_DUKTAPE := YES` | not required | `[OPTIONAL]` |
| Video/WebGL/WebRTC/extensions | no first-milestone dependency | not required | `[OPTIONAL]` |
| Framebuffer frontend | `frontends/framebuffer/gui.c`, `framebuffer.c`, `fbtk/`, `bitmap.c` | required | `[PORT][BLOCKER]` |
| Framebuffer abstraction | `libnsfb`: `nsfb_new`, `nsfb_set_geometry`, `nsfb_get_buffer`, `nsfb_update`, plotter API | required | `[ADAPT][BLOCKER]` |
| Font rendering | frontend `font_internal.c` or `font_freetype.c`; `NETSURF_FB_FONTLIB` | required | `[ADAPT][BLOCKER]` |
| Input | `libnsfb_event.h`, `nsfb_event`; frontend sends `browser_window_mouse_*()` and `browser_window_key_press()` | required for interaction | `[ADAPT][BLOCKER]` |
| Scheduler/clock | `frontends/framebuffer/schedule.c`; `nsu_getmonotonic_ms()` and `gettimeofday`/clock APIs | required | `[ADAPT][BLOCKER]` |
| Resources/files | `frontends/framebuffer/findfile.c`, `utils/file.c`, `utils/filepath.c` | required | `[IMPLEMENT][BLOCKER]` |

The upstream building documentation lists the NetSurf libraries as
BuildSystem, libparserutils, libwapcaplet, Hubbub, libcss, libnsgif, libnsbmp,
librosprite and libnsfb. The current build defaults additionally select
libcurl, libjpeg, optional libpng, OpenSSL certificate handling, and utf8proc.
For the first image test, SVG, JPEGXL, WebP, PDF, video, and Duktape can be
disabled until the base path works.

## NexOS interface matrix

| Required interface | NexOS evidence | Classification |
|---|---|---|
| Linear 32-bit framebuffer | `kernel/drivers/fb.h`: `fb.addr`, width/height/pitch/bpp, pixel and rectangle primitives | `[EXISTS]` for storage; `[ADAPT]` for `nsfb` buffer/plot ABI |
| Mouse input | `kernel/drivers/mouse.h`: position/button polling | `[EXISTS]` for raw input; `[ADAPT]` to `nsfb_event_t` |
| Keyboard input | `kernel/drivers/keyboard.h`: character polling and extended keys | `[EXISTS]` for raw input; `[ADAPT]` to NetSurf key codes |
| Window/frame ownership | `kernel/gui/wm.h` and `browser_app.c` | `[EXISTS]` as a NexOS window; `[ADAPT]` because upstream framebuffer owns its own `fbtk` surface |
| DNS | `kernel/net/dns.h`: `dns_resolve()` | `[EXISTS]` internally; `[ADAPT]` to resolver/socket or curl backend |
| TCP | `kernel/net/tcp.h`: `tcp_connect`, `tcp_send`, `tcp_recv`, `tcp_close` | `[EXISTS]` internally; `[ADAPT]` to a file-descriptor/socket ABI |
| UDP | `kernel/net/udp.h` | `[EXISTS]` internally; not sufficient by itself for curl |
| HTTPS/TLS | `kernel/net/http.c` explicitly logs `HTTPS is not supported (TLS is unavailable)` | `[IMPLEMENT][BLOCKER]` |
| BSD sockets | `kernel/proc/syscall.c`, socket/connect/sendto/recvfrom return `ENOSYS`; `DOCUMENTATION.md` records this | `[IMPLEMENT][BLOCKER]` if retaining libcurl |
| `select`/`poll` | syscall cases exist, but there is no socket backend to poll | `[ADAPT][BLOCKER]` |
| libc/hosted runtime | only small `userspace/libc/{stdio,string,stdlib}` sources; Makefile builds them into the kernel, not a hosted libc | `[IMPLEMENT][BLOCKER]` |
| mmap/heap/thread/TLS ABI | some Linux-shaped syscall cases exist, but no complete hosted pthread/libc ABI; `arch_prctl` alone is insufficient | `[IMPLEMENT][BLOCKER]` |
| filesystem/resource paths | VFS/RAMFS/FAT32 are kernel APIs, not POSIX `open/stat/opendir/readdir` available to a NetSurf process | `[ADAPT][BLOCKER]` |

## Exact POSIX/Linux surface actually requested

This is not a request to make NexOS Linux-compatible. These are the concrete
interfaces present in the audited framebuffer target and its enabled core
features:

* C hosted runtime: `malloc/calloc/realloc/free`, `mem*`, `str*`, `stdio`,
  `errno`, `assert`, `ctype`, `math`, `qsort`, and `setjmp`/compiler runtime
  support as selected by the compiler.
* Files/resources: `open`, `close`, `read`, `write`, `stat`, `access`,
  `mkdir`, `opendir`, `readdir`, `realpath`, `getenv`, and `PATH_MAX`-style
  path handling (`utils/file.c`, `utils/filepath.c`).
* Time/event loop: `gettimeofday`, monotonic time, `clock_gettime`, and
  blocking/polling support for scheduled fetches.
* Networking when using the existing upstream curl fetcher: `socket`,
  `connect`, `close`, `fcntl`/nonblocking state, `select` or `poll`, DNS
  resolution, and socket error/status APIs.
* TLS: the libcurl TLS backend and OpenSSL certificate store/API. A working
  TCP implementation is not TLS.

NexOS currently supplies none of these as a coherent hosted process ABI. The
Linux-numbered syscall dispatcher is not enough: `SYS_SOCKET` and
`SYS_CONNECT` return `ENOSYS`, and `fork`/`execve` are also unavailable.

## Ordered implementation plan

1. **[IMPLEMENT][BLOCKER] Hosted execution boundary.** Choose one explicit
   model: (a) a real NexOS userspace process + libc, or (b) a deliberately
   freestanding static NetSurf environment. The smallest auditable path is a
   static single-address-space port first, but it still needs a complete libc
   ABI and must not be confused with Linux compatibility.
2. **[ADAPT][BLOCKER] Replace libnsfb surfaces.** Add a NexOS `nsfb` surface
   whose buffer is `fb.addr`, geometry is the existing framebuffer metadata,
   and update/claim are no-ops or dirty-rectangle hooks. Translate PS/2 input
   to `nsfb_event_t`.
3. **[IMPLEMENT][BLOCKER] NetSurf scheduler/resources.** Map monotonic time,
   callbacks, resource lookup, and the framebuffer frontend's minimal file
   operations to NexOS APIs.
4. **[ADAPT][BLOCKER] Fetch transport.** First determine whether a minimal
   curl build can use a NexOS socket shim. If not, preserve NetSurf's fetch
   interface and write a small adapter over `dns_resolve`/`tcp_*`; do not
   silently substitute the existing HTTP parser.
5. **[PORT][BLOCKER] TLS and certificate validation.** Port one existing
   implementation (prefer a small TLS library or a suitably isolated OpenSSL
   subset) and connect it to the transport. Certificate roots and hostname
   validation must be explicit.
6. **[PORT][BLOCKER] NetSurf libraries.** Cross-compile only the required
   parser/layout/image libraries with optional JavaScript, video, WebP, PDF,
   and other features disabled.
7. **[OPTIONAL]** Add Duktape, cookies, redirects, forms, JavaScript, and
   broader image/video support incrementally after a simple HTTPS page works.

## Current milestone verdict

The repository passes its existing C syntax check, and its framebuffer, raw
keyboard/mouse, DNS, and internal TCP primitives are useful starting points.
It does **not** yet meet any NetSurf milestone beyond “NexOS boots and has a
framebuffer.” There is no evidence of NetSurf startup, HTML5 parsing, CSS
selection/layout, TLS negotiation, certificate validation, remote HTTPS
fetching, image decode, or NetSurf interaction. Those claims must remain
unmade until a real NetSurf binary is integrated and exercised in QEMU.

## Sources audited

* Upstream NetSurf `frontends/framebuffer/Makefile`, `gui.c`, `framebuffer.c`,
  `fetch.c`, `schedule.c`, `font_internal.c`, and `bitmap.c`.
* Upstream NetSurf `Makefile.defaults`, root `Makefile`, `content/fetch.c`,
  `utils/time.c`, `utils/file.c`, and `utils/filepath.c`.
* Upstream libnsfb public headers `libnsfb.h`, `libnsfb_event.h`,
  `libnsfb_plot.h`, and the RAM surface implementation.
* NexOS `kernel/drivers/fb.h`, `kernel/drivers/{keyboard,mouse}.h`,
  `kernel/net/{dns,tcp,http}.{h,c}`, `kernel/proc/syscall.c`, `Makefile`,
  and `DOCUMENTATION.md`.
