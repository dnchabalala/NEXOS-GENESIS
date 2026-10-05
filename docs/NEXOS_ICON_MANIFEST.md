# NexOS Icon Asset Manifest

Read-only asset-preparation manifest for:

- `nexos/docs/design/nexos_ui_master.html`
- `docs/NEXOS_ICON_AUDIT.md`

No assets were downloaded. No GUI source, `aurora.c`, or design source was
modified. The canonical HTML contains no identifiable real icon library or
SVG artwork; the manifest therefore records the Unicode and CSS sources and
recommended future production slots without creating replacements.

## Classification summary

| Class | Meaning in this design | Count |
|---|---|---:|
| A. REAL LIBRARY ICON | No confirmed canonical instances | 0 |
| B. UNICODE/FONT GLYPH | Literal Unicode text rendered by the HTML font stack | 21 |
| C. CSS-DRAWN ICON | CSS geometry used as icon-like controls | 1 |
| D. CUSTOM/UNKNOWN ARTWORK | No confirmed canonical instances | 0 |

`•`, `•••`, password bullets, and status separators are text punctuation or
masking rather than icon assets and are not counted as production icons.

## Production asset summary

| ID | Surface/purpose | Original | Target slot | Recommended filename | Required |
|---|---|---|---|---|---|
| UNI-001 | Rail home/apps mark | `◉` U+25C9 | rail/home | `icon-home.svg` | Yes |
| UNI-002 | Rail search mark | `⌕` U+2315 | rail/search | `icon-search.svg` | Yes |
| UNI-003 | Files/folder mark | `▣` U+25A3 | files/folder, Files app | `icon-folder.svg` | Yes |
| UNI-004 | Terminal/command mark | `⌘` U+2318 | terminal app | `icon-terminal.svg` | Yes |
| UNI-005 | Visualizer/music mark | `♪` U+266A | visualizer app | `icon-visualizer.svg` | Yes |
| UNI-006 | Settings mark | `⚙` U+2699 | settings app | `icon-settings.svg` | Yes |
| UNI-007 | Browser mark | `◎` U+25CE | browser app | `icon-browser.svg` | Yes |
| UNI-008 | Editor mark | `✎` U+270E | editor app | `icon-editor.svg` | Yes |
| UNI-009 | Monitor mark | `▥` U+25A5 | monitor app | `icon-monitor.svg` | Yes |
| UNI-010 | File/document mark | `▤` U+25A4 | file row | `icon-file.svg` | Yes |
| UNI-011 | System mark | `◫` U+25EB | system app | `icon-system.svg` | Yes |
| UNI-012 | Themes mark | `◐` U+25D0 | themes app | `icon-themes.svg` | Yes |
| UNI-013 | Calculator mark | `+` U+002B | calculator app | `icon-calculator.svg` | Yes |
| UNI-014 | Clock mark | `◷` U+25F7 | clock app | `icon-clock.svg` | Yes |
| UNI-015 | Snake mark | `◆` U+25C6 | snake app | `icon-snake.svg` | Yes |
| UNI-016 | Back navigation | `‹` U+2039 | Files/Browser back | `icon-arrow-back.svg` | Yes |
| UNI-017 | Forward navigation | `›` U+203A | Files/Browser forward | `icon-arrow-forward.svg` | Yes |
| UNI-018 | Parent directory | `↑` U+2191 | Files up | `icon-arrow-up.svg` | Yes |
| UNI-019 | Reload | `↻` U+21BB | Browser reload | `icon-reload.svg` | Yes |
| UNI-020 | Address marker | `◦` U+25E6 | Browser address field | `icon-globe-or-site.svg` | Yes |
| UNI-021 | Browser overflow | `•••` U+2022 × 3 | Browser menu | `icon-more.svg` | Yes |
| CSS-001 | Window close/minimize/maximize controls | CSS circles | window chrome | native traffic-light controls | No separate asset |

The filenames are recommendations only. No files were created.

## Detailed manifest

### UNI-001 — Rail home/apps mark

ID: `UNI-001`

Surface: Top-level left rail; first `.railitem` on desktop and most screens.

Purpose: Home/apps-like navigation position; the HTML supplies no semantic
label, so this purpose is positional/inferred.

Design source type: **B. UNICODE/FONT GLYPH**

Original source: Literal text child of `<div class="railitem">` in line 1 of
`nexos/docs/design/nexos_ui_master.html`.

Original identifier: `◉`, U+25C9 FISHEYE.

Canonical size: 48×48 CSS rail well; glyph CSS size 20px.

Target NexOS slot: Left rail home/apps navigation.

Monochrome or full-color: Monochrome text glyph; color comes from CSS state.

Tintable: Yes, if replaced by a native mask/vector asset.

Production asset required: Yes.

Recommended asset filename: `icon-home.svg`.

Notes: Exact Unicode appearance depends on the browser/system font. Replace
with a normalized home/apps concept, not a literal Unicode outline.

### UNI-002 — Rail search mark

ID: `UNI-002`

Surface: Left rail second `.railitem`; also the Terminal-active rail position.

Purpose: Search-like navigation mark based on position and glyph shape.

Design source type: **B. UNICODE/FONT GLYPH**

Original source: Literal `.railitem` child, line 1.

Original identifier: `⌕`, U+2315 TELEPHONE RECORDER.

Canonical size: 48×48 well; glyph CSS size 20px.

Target NexOS slot: Rail search/navigation; do not infer Terminal semantics
from the glyph alone because the HTML has no label.

Monochrome or full-color: Monochrome; CSS-tinted.

Tintable: Yes.

Production asset required: Yes.

Recommended asset filename: `icon-search.svg`.

Notes: The exact source glyph is not a standard magnifying-glass asset.

### UNI-003 — Files/folder mark

ID: `UNI-003`

Surface: Files rail/dock/launcher and folder rows such as `▣ Applications`.

Purpose: Files application and folder representation.

Design source type: **B. UNICODE/FONT GLYPH**

Original source: `.railitem`, `.d`, `.appicon`, and `.row` text children,
line 1.

Original identifier: `▣`, U+25A3 WHITE SQUARE CONTAINING BLACK SMALL SQUARE.

Canonical size: 20px glyph in 48×48 rail/dock/app wells; row size follows
the row text size.

Target NexOS slot: Files app icon and folder row icon.

Monochrome or full-color: Monochrome; CSS-tinted.

Tintable: Yes.

Production asset required: Yes.

Recommended asset filename: `icon-folder.svg`.

Notes: Use one coherent folder artwork with size variants rather than the
font glyph’s square-within-square appearance.

### UNI-004 — Terminal/command mark

ID: `UNI-004`

Surface: Terminal rail/dock/launcher.

Purpose: Terminal application identity.

Design source type: **B. UNICODE/FONT GLYPH**

Original source: `.railitem`, `.d`, and `.appicon` text children, line 1.

Original identifier: `⌘`, U+2318 PLACE OF INTEREST SIGN.

Canonical size: 20px glyph; 48×48 shell/app well.

Target NexOS slot: Terminal app icon.

Monochrome or full-color: Monochrome; CSS-tinted.

Tintable: Yes.

Production asset required: Yes.

Recommended asset filename: `icon-terminal.svg`.

Notes: The production concept should communicate a terminal/command line,
not reproduce the macOS command symbol unless that is explicitly desired.

### UNI-005 — Visualizer/music mark

ID: `UNI-005`

Surface: Visualizer rail/dock/launcher and System Information-like shell
state.

Purpose: Music/visualizer identity; the HTML does not name the rail item.

Design source type: **B. UNICODE/FONT GLYPH**

Original source: `.railitem`, `.d`, `.appicon` text children, line 1.

Original identifier: `♪`, U+266A EIGHTH NOTE.

Canonical size: 20px in 48×48 wells.

Target NexOS slot: Visualizer app icon.

Monochrome or full-color: Monochrome; CSS-tinted.

Tintable: Yes.

Production asset required: Yes.

Recommended asset filename: `icon-visualizer.svg`.

Notes: System Information uses the same shell position/glyph in the design
states; do not assume it is a distinct artwork asset.

### UNI-006 — Settings mark

ID: `UNI-006`

Surface: Settings rail/dock/launcher.

Purpose: Settings application identity.

Design source type: **B. UNICODE/FONT GLYPH**

Original source: `.railitem`, `.d`, `.appicon` text children, line 1.

Original identifier: `⚙`, U+2699 GEAR.

Canonical size: 20px in 48×48 wells.

Target NexOS slot: Settings app icon.

Monochrome or full-color: Monochrome; CSS-tinted.

Tintable: Yes.

Production asset required: Yes.

Recommended asset filename: `icon-settings.svg`.

Notes: This is a Unicode gear, not a package-provided gear icon.

### UNI-007 — Browser mark

ID: `UNI-007`

Surface: Browser dock and launcher; browser app state uses `◎` in the dock.

Purpose: Browser application identity.

Design source type: **B. UNICODE/FONT GLYPH**

Original source: `.d` and `.appicon` text children, line 1.

Original identifier: `◎`, U+25CE BULLSEYE.

Canonical size: 20px in a 48×48 well.

Target NexOS slot: Browser app icon.

Monochrome or full-color: Monochrome; CSS-tinted.

Tintable: Yes.

Production asset required: Yes.

Recommended asset filename: `icon-browser.svg`.

Notes: The HTML does not use a globe or browser-window artwork; the bullseye
is the exact original mark.

### UNI-008 — Editor mark

ID: `UNI-008`

Surface: Dock and launcher.

Purpose: Text Editor application identity.

Design source type: **B. UNICODE/FONT GLYPH**

Original source: `.d` and `.appicon` text children, line 1.

Original identifier: `✎`, U+270E LOWER RIGHT PENCIL.

Canonical size: 20px in a 48×48 well.

Target NexOS slot: Editor app icon.

Monochrome or full-color: Monochrome; CSS-tinted.

Tintable: Yes.

Production asset required: Yes.

Recommended asset filename: `icon-editor.svg`.

Notes: Replace with a normalized pencil/document concept.

### UNI-009 — Monitor mark

ID: `UNI-009`

Surface: Dock and launcher.

Purpose: System Monitor application identity.

Design source type: **B. UNICODE/FONT GLYPH**

Original source: `.d` and `.appicon` text children, line 1.

Original identifier: `▥`, U+25A5 SQUARE WITH VERTICAL FILL.

Canonical size: 20px in a 48×48 well.

Target NexOS slot: Monitor app icon.

Monochrome or full-color: Monochrome; CSS-tinted.

Tintable: Yes.

Production asset required: Yes.

Recommended asset filename: `icon-monitor.svg`.

Notes: Replace with a monitor/chart concept; the font glyph is not a clear
monitor silhouette.

### UNI-010 — File/document mark

ID: `UNI-010`

Surface: Files table rows.

Purpose: Document/file type marker, e.g. `kernel.log` and `notes.txt`.

Design source type: **B. UNICODE/FONT GLYPH**

Original source: `.row` text spans, line 1.

Original identifier: `▤`, U+25A4 SQUARE WITH HORIZONTAL FILL.

Canonical size: Inherited row text size; no dedicated icon box.

Target NexOS slot: Files list file icon.

Monochrome or full-color: Monochrome; CSS-tinted.

Tintable: Yes.

Production asset required: Yes.

Recommended asset filename: `icon-file.svg`.

Notes: The design does not distinguish file extensions with separate glyphs.

### UNI-011 — System mark

ID: `UNI-011`

Surface: Launcher app grid.

Purpose: System application identity.

Design source type: **B. UNICODE/FONT GLYPH**

Original source: `<div class="appicon">◫</div>`, line 1,
`LAUNCH-002 — Apps Launcher Open`.

Original identifier: `◫`, U+25EB WHITE SQUARE WITH VERTICAL BISECTING LINE.

Canonical size: 20px in 48×48 `.appicon`.

Target NexOS slot: System app icon.

Monochrome or full-color: Monochrome; CSS-tinted.

Tintable: Yes.

Production asset required: Yes.

Recommended asset filename: `icon-system.svg`.

Notes: No System icon appears in the six-item dock sequence.

### UNI-012 — Themes mark

ID: `UNI-012`

Surface: Launcher app grid.

Purpose: Themes application identity.

Design source type: **B. UNICODE/FONT GLYPH**

Original source: `<div class="appicon">◐</div>`, line 1, `LAUNCH-002`.

Original identifier: `◐`, U+25D0 CIRCLE WITH LEFT HALF BLACK.

Canonical size: 20px in 48×48 `.appicon`.

Target NexOS slot: Themes app icon.

Monochrome or full-color: Monochrome; CSS-tinted.

Tintable: Yes.

Production asset required: Yes.

Recommended asset filename: `icon-themes.svg`.

Notes: Replace with a theme/palette concept; no palette artwork is embedded.

### UNI-013 — Calculator mark

ID: `UNI-013`

Surface: Launcher app grid.

Purpose: Calculator application identity.

Design source type: **B. UNICODE/FONT GLYPH**

Original source: `<div class="appicon">+</div>`, line 1, `LAUNCH-002`.

Original identifier: `+`, U+002B PLUS SIGN.

Canonical size: 20px in 48×48 `.appicon`.

Target NexOS slot: Calculator app icon.

Monochrome or full-color: Monochrome; CSS-tinted.

Tintable: Yes.

Production asset required: Yes.

Recommended asset filename: `icon-calculator.svg`.

Notes: The plus sign alone is the entire original icon.

### UNI-014 — Clock mark

ID: `UNI-014`

Surface: Launcher app grid.

Purpose: Clock application identity.

Design source type: **B. UNICODE/FONT GLYPH**

Original source: `<div class="appicon">◷</div>`, line 1, `LAUNCH-002`.

Original identifier: `◷`, U+25F7 WHITE CIRCLE WITH UPPER RIGHT QUADRANT.

Canonical size: 20px in 48×48 `.appicon`.

Target NexOS slot: Clock app icon.

Monochrome or full-color: Monochrome; CSS-tinted.

Tintable: Yes.

Production asset required: Yes.

Recommended asset filename: `icon-clock.svg`.

Notes: Replace with a simple clock face, not a font-dependent quadrant glyph.

### UNI-015 — Snake mark

ID: `UNI-015`

Surface: Launcher app grid.

Purpose: Snake application identity.

Design source type: **B. UNICODE/FONT GLYPH**

Original source: `<div class="appicon">◆</div>`, line 1, `LAUNCH-002`.

Original identifier: `◆`, U+25C6 BLACK DIAMOND.

Canonical size: 20px in 48×48 `.appicon`.

Target NexOS slot: Snake app icon.

Monochrome or full-color: Monochrome; CSS-tinted.

Tintable: Yes.

Production asset required: Yes.

Recommended asset filename: `icon-snake.svg`.

Notes: No snake artwork exists in the HTML; the diamond is only a placeholder
concept.

### UNI-016 — Files/Browser back

ID: `UNI-016`

Surface: Files and Browser toolbars.

Purpose: Navigate backward.

Design source type: **B. UNICODE/FONT GLYPH**

Original source: `.btn` text child, line 1, `FILES-*` and `BROWSER-*`.

Original identifier: `‹`, U+2039 SINGLE LEFT-POINTING ANGLE QUOTATION MARK.

Canonical size: Generic `.btn` content; no icon-specific CSS box or size.

Target NexOS slot: 24px-class toolbar back icon.

Monochrome or full-color: Monochrome; CSS-tinted.

Tintable: Yes.

Production asset required: Yes.

Recommended asset filename: `icon-arrow-back.svg`.

Notes: Preserve the action semantics, not the typography-dependent glyph.

### UNI-017 — Files/Browser forward

ID: `UNI-017`

Surface: Files and Browser toolbars.

Purpose: Navigate forward.

Design source type: **B. UNICODE/FONT GLYPH**

Original source: `.btn` text child, line 1, `FILES-*` and `BROWSER-*`.

Original identifier: `›`, U+203A SINGLE RIGHT-POINTING ANGLE QUOTATION MARK.

Canonical size: Generic `.btn` content; no icon-specific CSS box or size.

Target NexOS slot: 24px-class toolbar forward icon.

Monochrome or full-color: Monochrome; CSS-tinted.

Tintable: Yes.

Production asset required: Yes.

Recommended asset filename: `icon-arrow-forward.svg`.

Notes: Pair geometry with `UNI-016`.

### UNI-018 — Files parent

ID: `UNI-018`

Surface: Files toolbar.

Purpose: Navigate to parent directory.

Design source type: **B. UNICODE/FONT GLYPH**

Original source: `.btn` text child, line 1, `FILES-*`.

Original identifier: `↑`, U+2191 UPWARDS ARROW.

Canonical size: Generic `.btn` content; no icon-specific CSS box or size.

Target NexOS slot: 24px-class toolbar parent/up icon.

Monochrome or full-color: Monochrome; CSS-tinted.

Tintable: Yes.

Production asset required: Yes.

Recommended asset filename: `icon-arrow-up.svg`.

Notes: The HTML uses a vertical arrow without a folder/container motif.

### UNI-019 — Browser reload

ID: `UNI-019`

Surface: Browser toolbar in `BROWSER-001` through `BROWSER-010`.

Purpose: Reload the current page.

Design source type: **B. UNICODE/FONT GLYPH**

Original source: `.btn` text child, line 1.

Original identifier: `↻`, U+21BB CLOCKWISE OPEN CIRCLE ARROW.

Canonical size: Generic `.btn` content; no icon-specific box.

Target NexOS slot: 24px-class reload icon.

Monochrome or full-color: Monochrome; CSS-tinted.

Tintable: Yes.

Production asset required: Yes.

Recommended asset filename: `icon-reload.svg`.

Notes: Keep distinct from history back/forward icons.

### UNI-020 — Browser address marker

ID: `UNI-020`

Surface: Browser address field.

Purpose: Leading decorative/site marker before the hostname.

Design source type: **B. UNICODE/FONT GLYPH**

Original source: `.input` text child, line 1, `BROWSER-001`, loading, and
loaded states.

Original identifier: `◦`, U+25E6 WHITE BULLET.

Canonical size: Inherited address-field text size; no separate icon well.

Target NexOS slot: Browser address/site identity icon.

Monochrome or full-color: Monochrome; CSS-tinted.

Tintable: Yes.

Production asset required: Yes.

Recommended asset filename: `icon-globe-or-site.svg`.

Notes: The HTML does not identify it as a globe, lock, or certificate mark;
this recommendation is a functional browser address slot, not an extracted
artwork claim.

### UNI-021 — Browser overflow

ID: `UNI-021`

Surface: Browser toolbar.

Purpose: More/options control.

Design source type: **B. UNICODE/FONT GLYPH**

Original source: `.btn` text child containing `•••`, line 1.

Original identifier: U+2022 BULLET repeated three times.

Canonical size: Generic `.btn` content; no icon-specific box.

Target NexOS slot: 24px-class overflow/more icon.

Monochrome or full-color: Monochrome; CSS-tinted.

Tintable: Yes.

Production asset required: Yes.

Recommended asset filename: `icon-more.svg`.

Notes: The bullets are text, not a three-dot vector asset.

### CSS-001 — Window traffic lights

ID: `CSS-001`

Surface: Every window title bar.

Purpose: Close, minimize, and maximize visual controls.

Design source type: **C. CSS-DRAWN ICON**

Original source: `.dots` containing `.dot.red`, `.dot.yellow`, and
`.dot.green`, line 1.

Original identifier: CSS classes, no Unicode/SVG identifier.

Canonical size: Each circle 12×12px, `border-radius:50%`, with 10px gap in
`.dots`.

Target NexOS slot: Native window chrome traffic-light controls.

Monochrome or full-color: Full-color; red `#FF5F57`, yellow `#FFBD2E`, green
`#28C840`.

Tintable: No for canonical colors; state themes may provide semantic variants.

Production asset required: No separate asset; retain as a native CSS/native
renderer primitive.

Recommended asset filename: `window-controls-traffic-lights` (conceptual
slot only; do not create an image).

Notes: The HTML contains no glyph or path. The control behavior is implied by
color/order; the source provides no accessible icon names.

## D. CUSTOM/UNKNOWN ARTWORK review

No confirmed D-category artwork was found. Every icon-like mark visible in
the inspected top bar, rail, dock, launcher, window chrome, notification,
Files, Terminal, Browser, and Settings surfaces is either B (Unicode) or C
(CSS circles). Notifications and Settings text-only elements do not require
asset entries.

## A. REAL LIBRARY ICON review

No confirmed A-category icon was found. `lucide-react` appears only in
`artifacts/mockup-sandbox/package.json` as `catalog:` metadata and is not
referenced by the canonical HTML. There is no exact library icon name,
upstream URL, or package version to record for the design source.

## Production preparation rules

1. Do not treat the current Unicode glyphs as stable artwork; their shape
   varies with the rendering font and platform.
2. Generate monochrome, tintable assets for all `UNI-*` production slots.
3. Keep the window traffic lights as native CSS/renderer geometry rather than
   raster assets.
4. Preserve semantic slots and behavior while replacing appearance.
5. Do not add runtime SVG parsing based on this manifest.
6. Any future library adoption must record its exact package/version and
   license separately; none is established by the current HTML.
