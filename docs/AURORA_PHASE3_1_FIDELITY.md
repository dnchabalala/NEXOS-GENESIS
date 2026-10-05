# Aurora Phase 3.1 — Visual Fidelity Hardening

This checkpoint hardens the accepted Aurora shell without migrating
application interiors. The HTML specification remains the source of truth:
`nexos/docs/design/nexos_ui_master.html` and `docs/AURORA_HTML_SPEC.md`.

## Canonical framebuffer

The Multiboot2 framebuffer request and GRUB preference now request
1440×900×32 first, followed by 1280×720×32 and 1024×768×32 fallback modes.
Runtime QEMU selected `FB: 1440x900x32bpp ... pitch=5760`. The request remains
optional; the kernel consumes the actual Multiboot framebuffer tag.

## Typography

`tools/gen_aurora_assets.py` converts a redistributable DejaVu Sans file into
a fixed-cell transparent atlas for 12, 14, 16, 20, 24, and 32 pixel roles.
The generated header is `kernel/drivers/aurora_font_atlas.h`; no TTF parser or
runtime allocation is used. `font_aurora_puts()` and
`font_aurora_str_width()` select the nearest cached size. The legacy 8×16 IBM
renderer remains unchanged for boot, Terminal, and application compatibility
content.

## Icons

`aurora_icon_id_t` and `aurora_icon_draw()` provide native tinted shapes for
shell, launcher, dock, navigation, network, power, and toolbar semantics.
`aurora_app_icon_id()` supplies the app-icon container. There is no runtime
SVG parser or external asset dependency. Implemented IDs include Apps, Files,
Terminal, Browser, Settings, System, Theme, Calculator, Clock, Editor,
Visualizer, Snake, Monitor, WiFi, Ethernet, Search, Power, Restart, Back,
Forward, Reload, and Home.

## Surfaces and depth

Shared panel and window primitives add small offset shadow bands and a one
pixel top highlight. This approximates the HTML glass hierarchy using
layered colors and bounded blending. No blur kernel, animation, or persistent
repaint was introduced.

## Screenshot verification

QEMU HMP `screendump` captured `/tmp/nexos-aurora.ppm` at 1440×900 and it was
converted to `/tmp/nexos-aurora.png` for inspection. The images are temporary
and not repository assets. Chromium, Firefox, wkhtmltoimage, Playwright, and
other local HTML renderers were unavailable or crashed, so no independent
HTML screenshot was generated. Geometry comparison is based on extracted HTML
values and native capture dimensions.

## Deliberate differences and constraints

- Font antialiasing is a compact monochrome approximation, not browser-grade
  Inter rasterization.
- Shell icons are native line-drawn masks rather than SVG artwork.
- Glass blur is represented by layered dark surfaces, borders, highlights,
  and small shadows.
- Files and Terminal client interiors remain legacy by phase boundary.
- Atlas data is static, icons decode nothing per frame, and depth bands are
  painted only during existing dirty scene composition.
