# NexOS Architecture

## System overview

```
GRUB/Multiboot2 -> x86_64 kernel -> memory/drivers/VFS/processes/scheduler
                                  -> Ethernet/DNS/TCP/HTTP/Mbed TLS
                                  -> framebuffer GUI compositor
                                       -> Aurora shell + native applications
                                       -> NetSurf frontend/fetcher
```

NexOS is freestanding C/NASM code with its own kernel; it is not a Linux
userspace or desktop environment.

## Boot and kernel

`kernel/kernel.c` initializes architecture, memory, drivers, filesystems,
networking, processes and GUI prerequisites. GRUB loads the kernel ELF. The
Multiboot framebuffer tag supplies actual address, dimensions, pitch and bpp.
The preferred framebuffer is 1440x900x32, with 1280x720 and 1024x768 fallback
requests. The actual tag is authoritative.

## GUI/compositor

`gui.c` owns ordered presentation and the event loop. `fb.draw_addr` is the
scene target after backbuffer enable; `fb.addr` is physical scanout. The
scene is cursor-free. `desktop.c`, `shell.c`, `wm.c`, `taskbar.c`,
`launcher.c` and `notif.c` compose the shell.

Dirty scene changes trigger composition; idle must settle without continuous
full-frame commits. All drawing respects clip bounds and active target; commit
remains centralized. Window drag may use correctness-first full composition.

## Input and WM

PS/2 keyboard IRQ/scancode handling and IntelliMouse packet decoding feed the
GUI. Motion, buttons and wheel go through taskbar/launcher/WM dispatch.
`window_t` stores geometry, focus/visibility/state, callbacks and userdata.
The WM handles titlebar/client conversion, z-order, focus, minimize, maximize,
resize, close and drag capture.

## Aurora

`aurora.c/.h` centralize semantic colors, states, dimensions, typography
roles and native primitives. Software rendering approximates glass with
surfaces, borders, highlights and bounded shadows. No runtime SVG parser or
GPU blur is present.

## VFS/app model

ramfs is root, FAT32 is `/mnt` when available, and procfs is `/proc`.
Files uses VFS APIs; Terminal uses the shell backend; Settings uses
Wi-Fi/RTC/memory/network/power APIs. Other GUI apps are present but not yet
Aurora-migrated.

## Network/Browser

RTL8139 -> Ethernet/ARP/IP -> UDP/DHCP/DNS or TCP -> HTTP/Mbed TLS. Browser
uses upstream NetSurf through `nexos_frontend.c` and `nexos_fetcher.c`,
which bridge viewport, plotters, scheduled work and `http_get()`. This
architecture is protected.

## Assets

`gen_aurora_assets.py` creates the static GUI font atlas. The icon generator
creates prepared Unicode glyph alpha masks and manifests. Icon PNGs are not a
universal runtime loader yet.
