# Aurora Phase 3 — Native Shell Migration

Status: Phase 3 shell migration complete. Application interiors remain
legacy and Phase 4 is not included.

## Scope

The migrated surfaces are:

- desktop background;
- native window frame/titlebar presentation;
- taskbar;
- Apps launcher;
- notification toasts.

Files, Terminal, Settings, Browser, and the other application interiors keep
their existing paint and interaction callbacks.

## Shell implementation

`gui.c` activates `aurora_default_palette()` before the first composed frame.
The existing Catppuccin, Nord, Dracula, and Gruvbox palettes remain available
through the existing Theme application and continue to use
`aurora_apply_palette()`.

### Desktop

`desktop.c` now draws a static two-layer Aurora background with a restrained
accent band, sparse grid points, and the NexOS watermark. The previous moving
aurora animation was removed because it required periodic scene invalidation.
The desktop still exposes the existing rectangular repaint API for scene
restoration.

### Window chrome

`wm.c` keeps the existing geometry, hit regions, z-order, focus, drag,
minimize, maximize, restore, close, resize, and client clipping behavior.
The frame and titlebar now use semantic Aurora surfaces and the shared
`aurora_window_chrome()` primitive. The existing three circular controls retain
their proven locations and behavior. Depth is represented with a bounded dark
offset and focused/inactive semantic surfaces rather than a large blur/glow.

### Taskbar

`taskbar.c` keeps the Apps button, window pills, focus indication, clock,
memory display, and WiFi/Ethernet/no-network states. Apps and window pills now
use shared Aurora buttons. Ethernet uses an Aurora badge and all text/status
colors come from semantic roles. Existing width guards remain in place so
window pills stop before the right-side status area.

### Apps launcher

`launcher.c` keeps the existing twelve applications, launch callbacks, search
placeholder, hover behavior, Escape/outside dismissal, Restart, and Shutdown.
The panel, cards, icons, search field, separator, and power controls now use
Aurora primitives. The search field remains intentionally nonfunctional, as it
was before this migration. The expensive blur pass was removed in favor of an
opaque elevated panel and scrim.

### Notifications

`notif.c` keeps the existing four-entry queue, eviction policy, lifetime,
progress indication, and welcome/WiFi notification callers. Toasts now use an
Aurora panel surface and semantic accent/text roles. Toasts enter in their
parked position rather than running a full-scene slide animation; expiry is
event-driven so the desktop is not continuously recomposed.

## Invalidation behavior

| Event | Invalidation |
|---|---|
| Desktop startup | One complete scene composition |
| Window focus/raise | Scene dirty; one ordered composition |
| Window drag | Scene dirty; correctness-first full composition |
| Window resize/maximize | Scene dirty; application reformat preserved |
| Taskbar hover transition | Scene dirty only when target changes |
| Launcher open/close/hover | Existing launcher-driven scene invalidation |
| Notification show/expiry | Scene dirty on state transition |
| Cursor-only movement | Existing bounded cursor presentation path |
| Idle desktop | No periodic desktop animation invalidation |

The cursor-free scene/backbuffer, ordered composition, clipping, and single
commit architecture were not redesigned.

## Resolution behavior

Shell geometry continues to derive taskbar placement and desktop height from
the actual framebuffer. Window chrome uses the existing window rectangle.
Launcher placement is centered and clamped to the screen edges. The current
launcher card grid is designed around the existing 1024x768 runtime and fits
the 1280x720 and 1440x900 design proportions; very narrow displays below the
fixed card-grid width remain a known limitation.

## Design comparison

Matched:

- dark premium shell hierarchy;
- elevated rounded panels and cards;
- semantic indigo/lavender accent treatment;
- focused/inactive window distinction;
- compact status/taskbar controls;
- launcher card grid and power area;
- toast hierarchy and accent rail.

Approximated:

- glass and blur use opaque/layered surfaces;
- large soft shadows use bounded dark offsets;
- typography uses the existing bitmap font roles;
- app icons remain glyph-based;
- toast motion is simplified to preserve event-driven rendering.

Deferred:

- application interior migration;
- Control Centre, Universal Search, Notification Centre, Workspaces, and
  other future surfaces;
- image/SVG icon assets;
- new animation or GPU effects.

## Verification constraints

The ISO and generic qemu64 boot were verified in the current environment.
Interactive SDL/KVM verification was unavailable because `/dev/kvm` is not
present. The serial boot reached mouse initialization, WM initialization,
exactly one Files window, exactly one Terminal window, taskbar paint, initial
composition, and the GUI main loop.
