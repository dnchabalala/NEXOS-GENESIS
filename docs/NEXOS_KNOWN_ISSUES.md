# NexOS Known Issues

## HIGH

- Settings has no valid isolated framebuffer acceptance capture or physical
  interaction acceptance.
- Browser has no post-migration screenshot or physical acceptance; production
  HTTPS must be reverified after the new chrome.

## MEDIUM

- Exact browser-quality Inter rendering is not implemented; Aurora uses a
  static native raster atlas.
- True blur/heavy alpha glass is unavailable; surfaces use approximation.
- Prepared icon masks are not integrated uniformly; glyph fallback artifacts
  remain, including known Files concerns.
- Narrow responsive layouts lack complete physical verification.

## LOW

- QEMU monitor relative mouse injection is unreliable for precise automation.
- Historical `/tmp` screenshots are ephemeral.

## DEFERRED / PRODUCT ROADMAP

- JavaScript and image support require a separate source/runtime audit.
- Remaining app interiors are not migrated.
- Control Centre, Universal Search, Notification Centre, Workspace Overview,
  Lock screen, Package Manager, Storage Manager, Accessibility and Security
  Center are design concepts, not implemented product surfaces.
- Canva icon artwork is external and not compiled into this repository.
