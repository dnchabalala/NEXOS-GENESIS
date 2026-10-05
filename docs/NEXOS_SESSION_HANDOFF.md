# NexOS Session Handoff

Snapshot: 2026-10-05. Branch `main`; HEAD
`e78284359cefa724ac2c8213bcb0b42413b868c8`. The worktree is intentionally
dirty and must not be cleaned, reset, staged or reverted.

## 1. Project identity

NexOS is a custom x86_64 operating system written from scratch in C and NASM.
It is not Linux, an Arch distribution, or a Linux desktop environment. It has
its own kernel, drivers, VFS, scheduler, networking stack, graphical shell,
native applications and browser integration.

The product direction combines a small/native OS, low overhead and fast
feedback with coherent Aurora Dark desktop polish. “Arch-like” describes a
performance/minimalism philosophy, not the implementation platform. The HTML
design is visual source of truth; functionality must remain native and real.

## 2. Repository layout

| Path | Responsibility |
|---|---|
| `nexos/boot/` | Multiboot2 entry, linker script and GRUB configuration. |
| `nexos/kernel/` | Kernel entry, memory, processes, syscalls, VFS, GUI, networking, installer and packages. |
| `nexos/kernel/arch/x86_64/` | Long-mode boot, GDT/TSS, IDT/ISR, paging and ring-3 entry. |
| `nexos/kernel/drivers/` | VGA, serial, keyboard, PIT, ATA, PCI, RTL8139, RTC, framebuffer, fonts and mouse. |
| `nexos/kernel/gui/` | WM, compositor loop, Aurora primitives, shell, desktop, dock/launcher, notifications and applications. |
| `nexos/kernel/fs/` | ramfs, FAT32, procfs and VFS. |
| `nexos/kernel/net/` | Ethernet, ARP, IP, ICMP, UDP, DHCP, DNS, TCP, sockets and HTTP. |
| `nexos/kernel/proc/` | Process model, round-robin scheduler, ELF and syscalls. |
| `nexos/ports/tls/` | Mbed TLS source/configuration and NexOS TLS adapters. |
| `nexos/ports/netsurf/` | NetSurf sources/build support, frontend and fetcher adapter. |
| `nexos/tools/` | Build, QEMU, NetSurf/TLS, ISO, font and icon scripts. |
| `nexos/assets/` | Generated icon metadata and PNG alpha masks. |
| `nexos/docs/design/` | Canonical HTML/PDF design source. |
| `docs/` | Plans, specifications, phase reports, audits and handoff. |

## 3. Boot and runtime

GRUB loads `build/nexos.kernel` through Multiboot2. The x86 assembly entry
establishes long mode and requests a framebuffer. `kernel_main()` initializes
serial/VGA, GDT/TSS, IDT/PIC, paging, PMM/heap, PIT, ATA/PCI/RTC, RTL8139,
VFS, processes, scheduler and syscalls. Init populates `/etc`, `/dev`, `/proc`,
`/tmp`, attempts `/mnt`, and starts `nsh`.

The GUI enables the backbuffer, disables framebuffer-console ownership, applies
Aurora, initializes mouse/WM/dock/notifications, creates startup Terminal and
Files windows, shows the welcome notification, composes once, and enters the
main loop. Preferred graphical mode is 1440x900x32; 1280x720 and 1024x768
fallback requests exist. The actual Multiboot framebuffer tag is authoritative;
one verified 1440x900 runtime reported pitch 5760. Installer code has separate
1024x768 assumptions.

Normal QEMU uses `-machine q35`, `-vga virtio`, serial stdio, RTL8139 and
SLIRP user networking. With `/dev/kvm` it adds `-enable-kvm -cpu host`; without
it generic QEMU uses `-cpu qemu64`. Browser QEMU intentionally requires KVM
and host CPU entropy/RDRAND.

## 4. Graphics and framebuffer

The required ownership model is:

```text
cursor-free authoritative scene/backbuffer
        -> scene presentation/commit
        -> transient cursor overlay
        -> physical framebuffer
```

`fb.addr` is physical scanout; `fb.draw_addr` is the GUI draw target after
backbuffer enable. `fb_commit()` presents a completed scene. `fb_scene_dirty`
causes desktop/shell reconstruction. `wm_render_all()` paints windows
back-to-front, followed by dock, launcher and notifications as applicable.

Critical invariants:

- scene/backbuffer never contains persistent cursor pixels;
- cursor restoration uses the current clean scene, never a stale underlay;
- cursor-only updates do not repaint NetSurf;
- idle has no continuous full-screen composition/commit;
- drag correctness may use full composition;
- primitives do not call `fb_commit()` or own physical presentation;
- clip/render-target state and framebuffer bounds are restored;
- no cursor trails, window trails, stale rectangles or clipping leaks.

The Phase 3.2 1440x900 corruption was caused by Aurora icon calls passing
`thickness, color` to `fb_draw_rect_outline()`, whose contract is `color,
thickness`; large colors became giant thickness values and produced black
horizontal/vertical bands. The calls were corrected. `GUI_DEBUG_STAGE` and
backbuffer guard diagnostics remain available but default to zero/off.

## 5. Input

`drivers/keyboard.c` handles PS/2 IRQ/scancodes. `drivers/mouse.c/.h` handles
IntelliMouse negotiation, complete four-byte packets, signed deltas, buttons,
wheel and clamped coordinates. GUI dispatch forwards motion/hover to the
launcher/taskbar and clicks/movement/wheel/releases to the WM. WM capture keeps
dragging attached to the window.

Do not reintroduce smoothing, interpolation or artificial acceleration. Prior
cursor incidents were presentation/restore bugs, not evidence that raw input
needed filtering. Preserve button edges, wheel and drag semantics.

## 6. Window manager

`wm.c/.h` owns up to 16 windows, linked-list order, focus/raise, minimize,
maximize, close, move, resize, drag capture, client clipping and callbacks.
`window_t` stores geometry/title/state/visibility/focus, paint/click/move/wheel/
key/resize/close callbacks and userdata.

Current Aurora design targets are a 54px titlebar, 28px outer radius, three
12px traffic-light controls with 10px visual gap, focused/inactive surfaces,
bounded shadow approximation and generous functional hit regions.

## 7. Aurora UI system

`kernel/gui/aurora.h/.c` provides semantic colors, spacing/dimensions/radii,
text roles, state flags, icon IDs, panels/cards/buttons, fields, tabs, badges,
progress, rows, separators, dialogs/toasts, window chrome, app icon wells and
indicators. Semantic roles cover background, surface/elevated/hover/pressed/
selected/inactive, text hierarchy, accent variants, borders and status colors.

Aurora is the default direction while Catppuccin Mocha, Nord, Dracula and
Gruvbox Dark remain structurally supported by the theme palette mapping.
Rendering is software-only: no GPU blur, runtime SVG parser or heavy alpha
effects. Glass uses dark layers, borders, highlights and small shadows; the
background is static. Avoid per-frame allocation, blur kernels and animation
that causes persistent invalidation.

## 8. Typography

The legacy `font.c` 8x16 bitmap remains for boot/fallback and legacy content.
`tools/gen_aurora_assets.py` produces `kernel/drivers/aurora_font_atlas.h`,
with cached GUI sizes 12, 14, 16, 20, 24 and 32 pixels and
`font_aurora_puts()`/width helpers. Terminal intentionally remains monospace.

A prior bug centered glyphs in fixed cells while advancing by actual glyph
width, causing collisions/drift. The corrected path uses consistent placement
and advance. Exact browser-quality Inter antialiasing is not implemented.

## 9. Icon/assets

`NEXOS_ICON_AUDIT.md`, `NEXOS_ICON_MANIFEST.md` and
`NEXOS_ICON_ASSET_GENERATION.md` document that the HTML primarily uses
Unicode/font glyphs and CSS traffic-light geometry, not a confirmed SVG icon
library. `tools/gen_nexos_icon_assets.py` reads
`assets/icons/source/glyphs.tsv`, writes `assets/icons/generated/generated.tsv`
and 16/24/32/48/64px grayscale alpha masks. The local pass used DejaVu Sans,
Noto Sans and Iosevka Nerd Font fallbacks; Inter provenance/license is not
established. No external font binaries were committed.

The masks are not fully integrated into every application; native Aurora icon
primitives remain in use and Files has known fallback issues. A separate Canva
set contains 22 pages for Apps, Files, Terminal, Browser, Settings, System,
Theme, Calculator, Clock, Editor, Visualizer, Snake, Monitor, WiFi, Ethernet,
Search, Power, Restart, Back, Forward, Reload and Home. Canva artwork is not
in this repository or compiled into NexOS.

## 10. Desktop shell

Primary source: `nexos/docs/design/nexos_ui_master.html`; secondary reference:
the PDF. The old GUI is not a design baseline. The shell has a top bar, left
rail, central overlapping windows, floating bottom dock, launcher and toasts.
Canonical geometry includes top bar `(16,12,1408,44)`, rail `(18,82,72,650)`,
main design window `(116,88,1220,744)`, 54px titlebar, 28px radius, and dock
bottom 24px/high 66px. Native fallback layouts use actual dimensions.

The desktop background targets `#070A12`, static indigo/violet illumination
and a 32px dot grid. A tiled 16px illumination experiment was removed because
it was visibly blocky.

`shell.c` draws branding/context, network/memory/clock grouping and the rail.
`taskbar.c/.h` is historical naming for the current floating dock: it draws
Apps, running window pills, focus/restore behavior, clock/memory/network state
and bounds. The old full-width taskbar visual model must not return.

`launcher.c` provides modal/scrim, search-field presentation, app grid,
hover/pressed states and power controls. Current inventory: Terminal, Files,
System, Theme, Browser, Calc, Clock, Editor, Visualizer, Snake, Monitor,
Settings. Search is visual-only/placeholder; Apps toggle, Escape, outside-click,
real app launch, Restart and Shutdown are preserved.

`notif.c` preserves queue/lifetime and welcome/Wi-Fi-compatible notifications,
with Aurora toast, accent and lifetime/progress treatment. Do not restore
continuous slide/fade repaint.

## 11. Files and Terminal (Phase 4A)

Files is reported implemented and visually accepted. Its Aurora split view has
Places/sidebar, back/forward/up toolbar, path/search controls, heading/subtitle,
rows, selected row, metadata, separators, footer count and empty-folder state.
VFS traversal, opening, selection, parent navigation, mouse/keyboard/wheel,
resize and callbacks remain real. Search, independent Places navigation,
history and file operations are inert/unsupported unless source proves them.

Terminal is reported implemented and visually accepted. It has native-shell
heading/helper text, dark terminal surface, monospace command/output, prompt
accent, error coloring, bounded cursor and responsive rows. Command input,
execution, output/errors, history/completion where supported, scrolling and
WM integration are preserved. Do not rewrite shell execution for UI polish.

## 12. Settings (Phase 4B)

Settings is implemented/build-verified but not visually or physically accepted.
`settings_app.c` maps the HTML split navigation/content model to Wi-Fi,
Display, System and About with Aurora cards, rows, badges and fields. Wi-Fi
states include disconnected, AP list, selected AP, password, connecting,
connected and failure. Scan/select/password/connect/disconnect remain real.
Display/System/About expose existing information and power behavior. Unsupported
keyboard/mouse/display configuration was not fabricated.

The attempted Settings screenshot is invalid acceptance evidence: QEMU monitor
input left the launcher visible. Capture and physically verify Settings.

## 13. Browser (Phase 4B)

Browser is implemented/build-verified but not visually or physically accepted.
Chrome is a 56px toolbar with Back/Forward/Reload, address field/focus state,
ready/loading/error badges, 32px status footer, NetSurf/network status and an
error card. The real viewport is the live client rectangle below toolbar and
above status; resize/maximize/reformat uses that geometry. No Browser screenshot
was captured after Phase 4B.

The protected path is:

```text
Browser UI -> upstream NetSurf -> nexos_frontend.c -> nexos_fetcher.c
           -> http_get/http_free -> DNS/TCP/Mbed TLS -> network
```

Do not replace NetSurf, fake webpages or hardcode the landing page. The primary
URL is `https://nexos.dnchabalala.site/`. JavaScript is not in current scope;
image support is not established as implemented and must be verified before
claiming it. Source provides address navigation, Back, Forward, Reload,
keyboard scrolling, wheel, pointer hover/link interaction, status/history and
resize/reformat, but post-Phase-4B physical verification is pending.

## 14. Networking and fixes

The stack is RTL8139 Ethernet -> ARP/IP -> UDP/DHCP/DNS and TCP -> HTTP and
Mbed TLS HTTPS. Browser QEMU uses RTL8139/SLIRP and requires KVM/`-cpu host`.

`http.c` now completes Content-Length responses at declared body length and
recognizes terminal chunked transfer without waiting for close/timeout.
TCP receive draining updates ACK/window state promptly. Historical traces
reduced tiny-response/page loading from minutes to seconds. Preserve TLS
certificate verification and these framing/window fixes.

`NEXOS_HTTP_TRACE` and `NEXOS_TCP_TRACE` default to 0. Historical baseline:
main HTML about 3.1s, CSS about 3.0s, first paint 3.263s, content ready
16.855s, Loading cleared 19.394s. These are not current guarantees.

`nexos_frontend.c` supplies native viewport/plotter/bitmap/window integration;
`nexos_fetcher.c` bridges NetSurf fetches to the existing HTTP client. The
Makefile provides adapter and native link probes.

## 15. Application status

| Application | Backend | Aurora migration | Visual/interactive state | Next work |
|---|---|---|---|---|
| Files | Implemented | Phase 4A | Accepted per report | Preserve; icon fallback only |
| Terminal | Implemented | Phase 4A | Accepted per report | Preserve |
| Settings | Implemented Wi-Fi/info/power | Phase 4B | Build verified; acceptance pending | Capture/physical test |
| Browser | Real NetSurf | Phase 4B | Build verified; acceptance pending | Capture/physical HTTPS test |
| System Information | Implemented | Not migrated | Pending | Later phase |
| Themes | Implemented | Not migrated | Pending | Later phase |
| Calculator | Implemented | Not migrated | Pending | Later phase |
| Clock | Implemented | Not migrated | Pending | Later phase |
| Editor | Implemented | Not migrated | Pending | Later phase |
| Visualizer | Implemented | Not migrated | Pending | Later phase |
| Snake | Implemented | Not migrated | Pending | Later phase |
| System Monitor | Implemented | Not migrated | Pending | Later phase |

Future Control Centre, Universal Search, Notification Centre, Workspace
Overview, Lock screen, Package Manager, Storage Manager, Accessibility and
Security Center are design concepts, not functional features.

## 16. Design documents and migration history

Authoritative design: `nexos/docs/design/nexos_ui_master.html`; secondary
visual reference: `nexos/docs/design/nexos_ui_master.pdf`. Supporting docs are
`AURORA_HTML_SPEC.md`, `AURORA_COMPONENTS.md` and
`AURORA_IMPLEMENTATION_PLAN.md`. Historical evidence is in
`AURORA_PHASE3_SHELL.md`, `AURORA_PHASE3_RECONSTRUCTION.md`,
`AURORA_PHASE3_1_FIDELITY.md`, `AURORA_PHASE3_3_VISUAL_CHECKLIST.md`,
`AURORA_PHASE4A_FILES_TERMINAL.md` and
`AURORA_PHASE4B_SETTINGS_BROWSER.md`. Icon evidence is in
`NEXOS_ICON_AUDIT.md`, `NEXOS_ICON_MANIFEST.md` and
`NEXOS_ICON_ASSET_GENERATION.md`. Historical reports retain old failures;
current status belongs in this handoff.

Chronology: Phase 0 audit/spec; Phase 2 Aurora foundation; initial Phase 3
shell migration (technically successful but visually rejected); structural
HTML-based Phase 3 reconstruction; Phase 3.1 1440x900/font/icon/depth work;
Phase 3.2 framebuffer corruption diagnosis and font correction; Phase 3.3/3.4
visual convergence/literal shell passes; Phase 4A Files/Terminal; Phase 4B
Settings/Browser implementation. The important lesson is that source
constants are not acceptance: actual framebuffer screenshots and physical
interaction are required.

## 17. Acceptance matrix

| Component | Implemented | Build | Screenshot | Interactive | Accepted |
|---|---:|---:|---:|---:|---:|
| Boot | Yes | Yes | Boot evidence | Generic boot only | Boot accepted |
| Framebuffer | Yes | Yes | 1440x900 evidence | N/A | Correctness accepted |
| Desktop/top bar/rail/dock | Yes | Yes | Historical captures | Partial | Historical shell acceptance |
| Window chrome | Yes | Yes | Historical captures | Partial | Historical shell acceptance |
| Launcher/notifications | Yes | Yes | Historical captures | Partial | Historical shell acceptance |
| Files | Yes | Yes | Yes | Reported accepted | Phase 4A accepted |
| Terminal | Yes | Yes | Yes | Reported accepted | Phase 4A accepted |
| Settings | Yes | Yes | Invalid final capture | No | Pending |
| Browser | Yes | Yes | None after Phase 4B | No | Pending |
| Networking/TLS/NetSurf | Yes | Yes | Runtime history | Browser recheck pending | Architecture/performance accepted historically |

## 18. Build/run commands

Verified command set:

```sh
make -C nexos check
make -C nexos test-tcp
make -C nexos kernel
make -C nexos tls-check
make -C nexos netsurf-adapter-check
make -C nexos netsurf-native-link-probe
make -C nexos iso
git diff --check
```

`check` syntax-checks kernel C; `test-tcp` runs reliability tests; `kernel`
builds/links; `tls-check` checks the TLS adapter; adapter-check validates the
fetcher ABI; native link probe builds native NetSurf objects; `iso` packages
the bootable image. Runtime targets are `make -C nexos run`, `run-debug`,
`run-browser` and `run-browser-interactive`.

## 19. Screenshot/debug workflow

QEMU monitor `screendump` may output PPM/Netpbm despite a `.png`-looking
destination. Convert it before inspection. Historical names include
`/tmp/nexos-phase33-desktop-final.png`, `nexos-phase33-launcher.png`,
`nexos-phase33-notification.png` and `nexos-aurora.png`; `/tmp` is ephemeral.
The Phase 4B Settings-named image showed the launcher and is invalid evidence;
Wi-Fi and Browser Phase 4B captures were not produced.

## 20. Known issues and limitations

**High:** Settings visual/physical acceptance pending; Browser visual,
physical and post-change production HTTPS acceptance pending.

**Medium:** exact Inter rendering unavailable; browser-quality blur/heavy alpha
glass unavailable; production icon masks not uniformly integrated; Unicode
fallback artifacts remain; narrow layouts lack complete physical verification.

**Deferred:** JavaScript and image support require separate audit; remaining
app interiors are not migrated; future-only surfaces are not implemented;
Canva artwork is external.

## 21. Regression-prevention checklist

Never reintroduce continuous desktop/notification/browser repaint, cursor or
window trails, stale underlays, unbounded idle full-screen commits, the old
full-width taskbar design, fake Browser rendering, hardcoded webpages,
weakened TLS, HTTP Content-Length/chunked completion regressions, TCP ACK/window
regressions, mouse smoothing/interpolation, or expensive launcher blur.

The framebuffer bug guard is also permanent: `fb_draw_rect_outline()` takes
`color, thickness`, not the reverse. Keep actual-width/pitch arithmetic and
centralized clip/target ownership.

## 22. Worktree and generated assets

At snapshot, 29 tracked paths were modified and 136 untracked files were
present; untracked groups included
`docs/`, `nexos/assets/`, `nexos/docs/design/`, the Aurora source files,
generated font atlas and asset scripts. Exact paths are in
`NEXOS_WORKTREE_SNAPSHOT.md`. Known Phase 4B files are only
`nexos/kernel/gui/settings_app.c` and `nexos/kernel/gui/browser_app.c`; all
other accumulated edits are pre-existing unless proven otherwise.

`kernel/drivers/aurora_font_atlas.h` is generated by
`tools/gen_aurora_assets.py`. Icon metadata/masks are generated by
`tools/gen_nexos_icon_assets.py` from `assets/icons/source/glyphs.tsv`.
Icon PNGs are prepared assets, not a universal runtime loader.

## 23. Exact next-session procedure

1. Read this handoff completely.
2. Run `git status --short`; do not clean/reset.
3. Run `make -C nexos check`, `kernel` and `iso`.
4. Boot with interactive KVM/input if available.
5. Open Settings; test sections, Wi-Fi list/selection/password/connection,
   resize and maximize/restore.
6. Capture valid Settings and Wi-Fi framebuffer screenshots.
7. Open Browser and load `https://nexos.dnchabalala.site/`; verify real
   NetSurf rendering, chrome/viewport separation, scroll, Back, Forward,
   Reload, address navigation, hover/click and resize/maximize/restore.
8. Capture Browser ready/loading/error screenshots.
9. Fix only Phase 4B regressions discovered during acceptance.
10. Only after Phase 4B acceptance, migrate the remaining app group.

Recommended subsequent order from the existing plan: System Information and
Themes, Calculator and Clock, then Editor, Visualizer, Snake and System
Monitor. This is not authorization during the freeze.

## 24. Development rules

- Read the handoff first and inspect source before changing it.
- Use the canonical HTML as UI source of truth.
- Preserve backend behavior and never fake unsupported functionality.
- Use framebuffer screenshots for visual acceptance.
- Never claim physical interaction without performing it.
- Do not modify stable subsystems without evidence.
- Do not clean, revert or stage unrelated worktree changes.
- Do not replace NetSurf or weaken TLS.
- Keep dirty-driven rendering and centralized compositor ownership.
- Mark future concepts as future and stop at requested phase boundaries.
