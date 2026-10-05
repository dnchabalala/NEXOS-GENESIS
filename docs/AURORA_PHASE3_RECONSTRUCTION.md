# Aurora Phase 3 — Structural Shell Reconstruction

This checkpoint replaces the legacy shell presentation with the structural
shell described by `docs/AURORA_HTML_SPEC.md`. Application interiors remain
unchanged and are explicitly deferred to Phase 4.

## Implemented shell

- static Aurora desktop background across the complete framebuffer;
- top system bar at the HTML geometry (`16px` side margins, `12px` top,
  `44px` height, `16px` radius) with NexOS context, network, memory, and
  clock information;
- left navigation rail at the HTML geometry (`18px` left, `82px` top,
  `72px` width, rounded surface, six supported navigation affordances);
- floating centered dock (`66px` high, `24px` bottom, `48px` items,
  `12px` gaps) replacing the full-width legacy taskbar surface;
- 54px window titlebars with 12px traffic-light controls and 28px frame
  geometry;
- 520px launcher modal with four-column, 104px app cards, scrim, search
  presentation, and existing launch/restart/shutdown actions;
- 320px notification surfaces positioned above the dock.

## Preserved behavior

Window focus, z-order, drag, minimize/restore, maximize, close, startup
Files and Terminal windows, Apps launcher actions, existing themes, and the
cursor-free scene/backbuffer compositor remain in use. Minimized windows stay
represented in the dock so they can be restored.

## Responsive policy

The canonical 1440×900 values are used directly where space permits. Current
QEMU exposes 1024×768, so the shell clamps the rail and centers/clamps the
dock and launcher without assuming 1440×900. The current framebuffer mode is
still supplied by the existing boot path; no video-mode change was made in
this checkpoint.

## Known approximation/deferments

The software renderer still uses its 8×16 bitmap font and glyph-based icon
primitive. True Inter typography, a runtime SVG/icon pipeline, blur, and
full translucent glass are not introduced here. They remain renderer-aware
approximations. Files, Terminal, Settings, Browser, and all other app
interiors remain legacy until Phase 4.

## Verification

`make -C nexos kernel`, `make -C nexos iso`, and `git diff --check` passed.
Generic serial QEMU boot reached the GUI at 1024×768, initialized the new
Dock, composed two startup windows, and entered the GUI loop. KVM/SDL
interactive verification was not performed in this environment.
