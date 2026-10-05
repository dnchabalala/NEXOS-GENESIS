# NexOS Build and Run

Run from the repository root; Make targets use `nexos/`.

## Prerequisites

The repository expects `make`, `gcc` or `x86_64-elf-gcc`, `ld` or a
cross linker, `nasm`, `grub-mkrescue`, `xorriso`,
`qemu-system-x86_64`, Python 3, and host tools used by the NetSurf/TLS
scripts.

## Verified build/check commands

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

`check` syntax-checks kernel C; `test-tcp` runs reliability tests;
`kernel` builds/links; `tls-check` checks the TLS adapter;
`netsurf-adapter-check` validates the fetcher ABI; native link probe builds
native NetSurf objects; `iso` packages `nexos/build/nexos.iso`.

## QEMU

```sh
make -C nexos run
make -C nexos run-debug
make -C nexos run-browser
make -C nexos run-browser-interactive
```

`tools/run_qemu.sh` uses KVM/`-cpu host` when `/dev/kvm` exists and
otherwise uses `-cpu qemu64`; it boots ISO with virtio video, serial stdio,
RTL8139 and SLIRP user networking. `tools/run_browser_qemu.sh` intentionally
requires KVM, `-cpu host`, RTL8139 and SLIRP because the browser runtime
historically requires host entropy/RDRAND.

## Diagnostics and screenshots

`NEXOS_HTTP_TRACE` and `NEXOS_TCP_TRACE` default to 0.
`GUI_DEBUG_STAGE` defaults to 0; nonzero values render one diagnostic
framebuffer and halt. Use QEMU monitor `screendump`; output may be
PPM/Netpbm despite a `.png` filename and must be converted before
inspection. Source constants and serial logs are not visual acceptance
evidence.

## Runtime limitations

Without KVM, browser-capable runtime is unavailable. QEMU relative mouse
injection can be unreliable for precise app-state screenshots. Never claim
physical interaction from a boot-only run.
