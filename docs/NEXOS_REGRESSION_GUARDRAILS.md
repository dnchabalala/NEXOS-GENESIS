# NexOS Regression Guardrails

Future agents must preserve these solved behaviors and ownership rules.

## Framebuffer/compositor

- Keep the authoritative scene cursor-free and commit centrally.
- Restore old cursor pixels from the current clean scene; never use stale
  underlays.
- No cursor trails, window trails, stale rectangles or clipping leaks.
- No continuous idle full-screen repaint; cursor-only movement must not
  repaint NetSurf.
- Active drag may use correctness-first full composition.
- `fb_draw_rect_outline()` call order is `color, thickness`; reversing it
  caused the 1440x900 black bands and must not return.

## Input

- Preserve complete IntelliMouse packet decoding, wheel and button edges.
- Do not add smoothing/interpolation/artificial acceleration.
- Preserve WM drag capture, click, wheel and hit-test boundaries.

## Network/browser

- Complete Content-Length at declared body length.
- Complete chunked transfer at the terminal zero-size chunk.
- Preserve immediate TCP receive-window/ACK progression.
- Preserve TLS certificate verification and Mbed TLS configuration.
- Do not replace upstream NetSurf, fake webpages or hardcode the landing page.

## UI/design

- The canonical HTML is the source of truth; do not restore legacy taskbar
  structure or use the old GUI as the design baseline.
- Use Aurora semantic primitives and keep future surfaces clearly future.
- Do not spend migration work on procedural icon recreation when the asset
  pipeline or external artwork is the actual concern.

## Validation

Builds and serial boot are not visual/physical acceptance. Use actual QEMU
framebuffer screenshots and real input tests before claiming acceptance.
