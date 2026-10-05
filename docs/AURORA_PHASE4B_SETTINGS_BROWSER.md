# Aurora Phase 4B — Settings and Browser

## Scope

This phase migrates only Settings and Browser presentation. The desktop
shell, top bar, rail, dock, window chrome, Files, Terminal, compositor,
framebuffer, cursor system, and Browser networking/backend remain protected.

## Settings

### HTML structure discovered

The canonical HTML contains four Settings states:

- `Settings — Wi‑Fi`
- `Settings — Display`
- `Settings — System`
- `Settings — About`

All use a split layout with a Settings navigation sidebar and a padded
content region. The sidebar contains the `SETTINGS` eyebrow and Wi‑Fi,
Display, System, and About navigation rows. The content region uses a page
heading, muted `NexOS system preferences` subtitle, cards, rows, status pills,
buttons, and fields.

The Wi‑Fi design includes disconnected, network-selection, password, and
connected visual concepts. Display is information-oriented in the current
backend: resolution, colour depth, software rendering, and brightness. The
System design includes memory, network, uptime, and power cards. About uses a
large NexOS identity card and system metadata.

### Native structure implemented

`nexos/kernel/gui/settings_app.c` now uses Aurora primitives for:

- responsive split navigation sidebar;
- Settings eyebrow and active navigation row;
- page heading and subtitle;
- Wi‑Fi connection card, signal indicator, status badge, AP rows, password
  field, Connect and Re-scan actions, Disconnect action, and result messages;
- Display information cards for resolution, colour depth, rendering, and
  brightness;
- System memory/network/uptime cards and Restart/Shutdown controls;
- About identity card and architecture/kernel/boot/graphics/network/syscall
  information.

The sidebar is 180px at the normal application width, contracts at narrower
widths, and disappears at the smallest fallback width. Content padding and
card widths are derived from the live client rectangle.

### Supported states and preserved behavior

Preserved from the existing Settings backend:

- Wi‑Fi scan
- access-point list
- signal display
- AP selection
- encrypted-network password entry
- connect and disconnect
- connected/disconnected status
- connection success/failure messages
- Wi‑Fi notifications
- display information
- system information
- reboot and shutdown actions
- About information
- Settings resize, focus, maximize, restore, and close integration

Unsupported or intentionally non-functional presentation:

- Display brightness is informational and not an adjustable setting.
- Colour theme remains informational; theme switching remains in the existing
  Theme application.
- Sidebar entries navigate only among the existing four Settings tabs.
- No keyboard/mouse configuration UI was invented.
- No Bluetooth, security center, accessibility, or control-center UI was
  added.

## Browser

### HTML structure discovered

The canonical Browser states are:

- `BROWSER-001 — Ready`
- `BROWSER-002 — Address Focus`
- `BROWSER-003 — Loading`
- `BROWSER-004 — Loaded HTML`
- `BROWSER-005 — Error`

The HTML Browser client is a vertical composition with a 56px toolbar,
navigation controls, a flexible address field, a real page viewport framed by
an 18px content inset/card treatment, and a 32px status footer. The status
footer shows a semantic state and `NetSurf • NexOS networking`. Loading uses a
progress treatment; error uses a danger pill and explanatory card.

### Native chrome implemented

`nexos/kernel/gui/browser_app.c` now paints:

- a 56px Aurora toolbar;
- reusable Back, Forward, and Reload icon-button slots;
- an Aurora address field with focus border and cursor;
- a 32px status footer with semantic Ready/Loading/Error badge;
- NetSurf/NexOS networking status text;
- Aurora error-card presentation while preserving the existing error state;
- the real NetSurf viewport beneath the toolbar and above the footer.

The existing toolbar dispatch remains separate for Back, Forward, and Reload.
The existing address input remains the navigation input. The actual page
content is still drawn by upstream NetSurf through the NexOS frontend.

### NetSurf viewport integration

The viewport remains bound through:

`nexos_netsurf_bind_window()` and `nexos_netsurf_set_viewport()`

The new viewport is calculated as:

- x: 0
- y: `TOOLBAR_H` (56)
- width: live Browser client width
- height: live client height minus 56px toolbar and 32px status footer

Mouse tracking, clicks, wheel scrolling, keyboard scrolling, resize,
maximize, restore, reformat, invalidation, painting, HTTPS, and history
continue through the existing frontend/NetSurf APIs.

### Browser states

- Ready: success status badge and live viewport
- Address focus: focused field and editing cursor
- Loading: warning status badge while the real fetch is active
- Loaded HTML: real NetSurf page remains the viewport content
- Error: danger status badge and error card
- Link hover/status: preserved through existing NetSurf mouse tracking and
  status callback

No custom HTML renderer, fake page, hardcoded production page, or networking
change was introduced.

## Icons

The Browser uses existing Aurora icon IDs for Back, Forward, and Reload.
Settings uses the existing Aurora signal indicator and shared controls. No
new icon asset pipeline or procedural icon family was added in this phase.

## Performance and rendering safety

- No continuous repaint loop was added.
- Settings remains event-driven.
- Browser chrome is repainted only as part of the existing window invalidation
  path; NetSurf still controls page invalidation.
- No changes were made to TCP, DNS, HTTP, TLS, Mbed TLS, NetSurf core,
  framebuffer stride, backbuffer allocation, PS/2 decoding, or cursor
  presentation.
- All new application drawing uses the existing scene/render-target path and
  Aurora primitives.

## Screenshots

QEMU monitor screendumps were used during a non-interactive TCG boot:

- `/tmp/nexos-phase4b-settings.png` — generated from a QEMU PPM screendump;
  the final capture remained on the launcher because monitor relative mouse
  injection was not reliable enough to leave Settings visible.
- `/tmp/nexos-phase4b-settings-wifi.png` — not captured.
- `/tmp/nexos-phase4b-browser.png` — not captured.
- `/tmp/nexos-phase4b-browser-loading.png` — not captured.
- `/tmp/nexos-phase4b-browser-error.png` — not captured.

The serial log did confirm that Settings was opened during QEMU automation.
The Browser was not opened in the available non-interactive session, so no
claim is made for a rendered HTTPS screenshot or physical Browser behavior.

## Validation

Completed successfully:

- `make -C nexos check`
- `make -C nexos test-tcp`
- `make -C nexos kernel`
- `make -C nexos tls-check`
- `make -C nexos netsurf-adapter-check`
- `make -C nexos netsurf-native-link-probe`
- `make -C nexos iso`
- `git diff --check`

QEMU TCG boot succeeded at 1440×900. Startup shell and existing Files and
Terminal windows initialized. Physical Settings Wi‑Fi interactions and
Browser interactions were not manually verified.

## Known limitations

- The QEMU monitor provides relative PS/2 mouse events, which made reliable
  automated navigation through the launcher impossible for final screenshots.
- Browser HTTPS runtime validation was not performed in this TCG session;
  the protected NetSurf/fetcher/network architecture was not changed.
- Existing bitmap/icon limitations remain outside this phase.
- No other application was migrated.
