# Aurora Phase 3.3 visual checklist

This checklist records the native shell against the canonical shell CSS in
`nexos/docs/design/nexos_ui_master.html`. Status is based on QEMU framebuffer
captures at 1440x900, not source geometry alone.

## Captures

| State | Capture |
|---|---|
| Desktop with Files + Terminal | `/tmp/nexos-phase33-desktop-final.png` |
| Launcher open | `/tmp/nexos-phase33-launcher.png` |
| Notification visible | `/tmp/nexos-phase33-notification.png` |

Launcher and notification captures use the existing compile-time debug state
renderer only to make interactive states deterministic for inspection. That
mode is not the default build or runtime path.

## Desktop

**HTML target:** `DESK-001`/`DESK-002` uses a continuous Aurora page with a
32px dot grid, top glass bar at `(16,12,1408,44)`, left rail at
`(18,82,72,650)`, intentional overlapping windows, and a centered dock at
`bottom:24px; height:66px`.

**Native implementation:** `desktop.c`, `shell.c`, `wm.c`, `taskbar.c`, and
`gui.c`. Startup now follows the reference composition: Terminal
`(186,218,540,320)` behind Files `(606,262,610,380)`.

**Status:** CLOSE. Global hierarchy and overlap read as the same composition;
client interiors are intentionally still legacy and blur/antialiasing remain
renderer-limited.

## Top Bar

**HTML target:** `.top` is a 16px-radius surface with a low-contrast 1px
line, 16px horizontal padding, NexOS/context hierarchy, grouped right status,
and broad soft shadow.

**Native implementation:** `shell_draw_topbar()` now uses layered surfaces, a
leading accent mark, NexOS/Desktop hierarchy, grouped network/memory/clock
cards, semantic colors, and framebuffer-safe width calculations.

**Status:** CLOSE. Surface weight and grouping are substantially closer; exact
font rendering and CSS shadow softness remain approximate.

## Left Rail

**HTML target:** `.leftbar` is `(18,82,72,650)`, 24px radius, 16px/12px
padding, 48px items, 18px item rhythm, subtle surface, and selected accent.

**Native implementation:** `shell_draw_left_rail()` uses six supported shell
icons, shared 48px cards, dark tinted icon wells, consistent borders/highlights,
and active framebuffer bounds.

**Status:** CLOSE. Geometry and rhythm match at 1440x900; icon path fidelity
and optical stroke consistency remain below the HTML glyph intent.

## Dock

**HTML target:** centered floating `.dock`, bottom 24px, 66px high, 9px/16px
padding, 12px gap, 48px items, 16px item radius, 16x4 active mark, and soft
depth.

**Native implementation:** `taskbar.c` provides centered dynamic layout,
running-window pills, Apps toggle, focus/minimize/restore behavior, layered
shadow, top highlight, active marks, and no legacy full-width taskbar.

**Status:** CLOSE. Outer geometry/depth and behavior match. Native shows Apps
plus running windows; the HTML's broader illustrative app set is not copied as
inert controls.

## Window Chrome

**HTML target:** `.window` 28px radius/deep shadow; `.titlebar` 54px; three
12px leading traffic lights with 10px visual gap; centered 13px title; clear
focused/inactive depth.

**Native implementation:** `aurora_window_chrome()` and `wm_draw_window()`.
Controls are leading-edge traffic lights, hit regions remain generous,
focused state receives accent treatment, and bounded shadow bands replace blur.

**Status:** CLOSE. Geometry, control side, hierarchy, and overlap align;
client content and exact shadow softness remain approximate.

## Files and Terminal outer windows

**HTML target:** Terminal recedes at left and Files sits forward at right in
`DESK-002`/`DESK-003`, with the same chrome and intentional overlap.

**Native implementation:** startup placement/resizing in `gui.c`, outer frame
in `wm.c`; client implementations are unchanged.

**Status:** CLOSE. Outer composition is aligned; interiors are Phase 4.

## Launcher

**HTML target:** `.scrim` plus centered `.modal` around 520px wide, 26px
radius, 4-column `.appgrid`, 14px gap, 18px app cards, 48px icon wells,
search field, and power controls.

**Native implementation:** `launcher.c` preserves the real app table, search
placeholder, hover/pressed behavior, Escape/outside close, and restart/shutdown.

**Status:** CLOSE. Captured launcher structure and geometry align; icon paths,
text rasterization, and alpha/glass softness are approximations.

## Notification

**HTML target:** `.toast` 320px wide, 18px radius, 15px padding, elevated
surface, accent treatment, title/body hierarchy, and progress/lifetime cue.

**Native implementation:** `notif.c` preserves queue/lifetime behavior with
layered panel depth, accent rail, progress underline, and dirty invalidation.

**Status:** CLOSE. Captured toast has correct bounds, placement, accent
hierarchy, and lifetime treatment; exact alpha/shadow softness is approximate.

## Deliberately not matched in this pass

- Files, Terminal, Browser, Settings, and other client interiors (Phase 4).
- Exact Inter antialiasing and CSS blur/shadows (unsupported by the current
  native renderer).
- HTML illustrative dock population (not copied as inert controls).
- Physical interactive acceptance: this environment has no `/dev/kvm`, so
  captures prove presentation only, not pointer actions.
