# NexOS Icon Audit

Read-only audit of `nexos/docs/design/nexos_ui_master.html` (the canonical
generated design HTML). No GUI or other source files were modified.

## Executive conclusion

The canonical HTML does not contain an icon package, inline SVG, SVG paths,
images, CSS masks, icon font, or reusable icon component. Most icon-like
artwork is literal Unicode text rendered by the page font. Window controls
are CSS circles. Notifications, Settings navigation, and Terminal client
content are text/CSS-only in the inspected design.

The HTML is a generated single-line document. The source locations below use
the page `data-label`, class names, and line 1 of the file.

## Package and library inspection

Search results for `nexos/docs/design/nexos_ui_master.html`:

| Item | Result |
|---|---|
| `<svg>`, `</svg>` | Not present |
| `viewBox` or SVG `path d=` | Not present |
| `<img>` or CSS `url(...)` | Not present |
| Lucide, Heroicons, Font Awesome, Phosphor, Material Icons, Iconify, Feather | Not present |
| HTML imports | None identified |

The page font stack is `Inter, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif`;
this is typography, not an icon package. Terminal-style content uses a
monospace family where specified.

Repository package manifests were also inspected. `artifacts/mockup-sandbox/package.json`
mentions `lucide-react` with the unresolved value `catalog:`. No import or
Lucide-generated markup exists in the canonical HTML, so this is package
metadata for another artifact, not proof that the design icons are Lucide.
The other inspected manifests contain no relevant icon dependency. No icon
license or provenance declaration is present in the canonical HTML.

## Exact Unicode inventory

| Glyph | Code point / Unicode name | Uses in the design |
|---|---|---|
| `◉` | U+25C9 FISHEYE | first rail item; positional home/apps-like mark |
| `⌕` | U+2315 TELEPHONE RECORDER | search-like rail item; Terminal active rail position |
| `▣` | U+25A3 WHITE SQUARE CONTAINING BLACK SMALL SQUARE | Files rail/dock/launcher and folder rows |
| `⌘` | U+2318 PLACE OF INTEREST SIGN | Terminal rail/dock/launcher |
| `♪` | U+266A EIGHTH NOTE | Visualizer/System Information rail/dock/launcher |
| `⚙` | U+2699 GEAR | Settings rail/dock/launcher |
| `◎` | U+25CE BULLSEYE | Browser dock/launcher |
| `✎` | U+270E LOWER RIGHT PENCIL | Editor dock/launcher |
| `▥` | U+25A5 SQUARE WITH VERTICAL FILL | Monitor dock/launcher |
| `▤` | U+25A4 SQUARE WITH HORIZONTAL FILL | file/document rows |
| `◫` | U+25EB WHITE SQUARE WITH VERTICAL BISECTING LINE | System launcher |
| `◐` | U+25D0 CIRCLE WITH LEFT HALF BLACK | Themes launcher |
| `+` | U+002B PLUS SIGN | Calculator launcher |
| `◷` | U+25F7 WHITE CIRCLE WITH UPPER RIGHT QUADRANT | Clock launcher |
| `◆` | U+25C6 BLACK DIAMOND | Snake launcher |
| `‹` | U+2039 SINGLE LEFT-POINTING ANGLE QUOTATION MARK | Files/Browser back |
| `›` | U+203A SINGLE RIGHT-POINTING ANGLE QUOTATION MARK | Files/Browser forward |
| `↑` | U+2191 UPWARDS ARROW | Files parent directory |
| `↻` | U+21BB CLOCKWISE OPEN CIRCLE ARROW | Browser reload |
| `◦` | U+25E6 WHITE BULLET | Browser address-field marker |

The bullet `•` (U+2022), `•••`, and password bullets are ordinary text
punctuation/masking, not separate icon artwork.

## Shell surfaces

### Top bar

**Purpose:** product identity, context, and system status.

**HTML:** `.top` containing `<b>NexOS</b>`, `.sub`, and `.muted` status text.
Example status is `Wi‑Fi • 94% • Sat 21:31`.

**Icon implementation:** none. The bullet is a text separator; no network,
battery, clock, SVG, image, or library icon is present.

**Source:** line 1, repeated in `DESK-*`, `FILES-*`, `TERM-*`, `BROWSER-*`,
Settings, System Information, and Theme pages.

### Left rail

**HTML:** `.leftbar` with six `.railitem` divs. Desktop order is:

```html
<div class="railitem">◉</div>
<div class="railitem">⌕</div>
<div class="railitem">▣</div>
<div class="railitem">⌘</div>
<div class="railitem">♪</div>
<div class="railitem">⚙</div>
```

The active item adds class `active`; Files uses the first item, Terminal the
second, Browser the third, System Information the fifth, and Settings the
sixth. The HTML does not provide explicit semantic names for these glyphs.

**Implementation:** Unicode text inside ordinary `div`s. CSS `.railitem`
provides a 48×48 grid-centered well and 20px glyph size; active appearance
is CSS surface/color treatment. No package/SVG/image is used.

**Source:** line 1, `.leftbar .railitem`, repeated across all screens.

### Dock

**HTML:** `.dock` containing the exact six-item sequence:

```html
<div class="dock">
  <div class="d">▣</div><div class="d">⌘</div><div class="d">◎</div>
  <div class="d">✎</div><div class="d">▥</div><div class="d">⚙</div>
</div>
```

`active` is added to the relevant `.d`. The active indicator is CSS
`.dock .d.active:after`, a 16×4 rounded bar, not icon artwork. Each dock
item is a 48×48 CSS well with centered Unicode text.

Files, Terminal, Browser, and Settings active positions are respectively
the `▣`, `⌘`, `◎`, and `⚙` entries. Source: line 1, `.dock .d`, repeated
on every major page.

### Launcher

**HTML:** `LAUNCH-002 — Apps Launcher Open` and `LAUNCH-003 — App Hover State`.
The `.appgrid` contains `.app` cards and `.appicon` wells. Exact mapping:

| App label | `.appicon` text |
|---|---|
| Terminal | `⌘` |
| Files | `▣` |
| System | `◫` |
| Themes | `◐` |
| Browser | `◎` |
| Calc | `+` |
| Clock | `◷` |
| Editor | `✎` |
| Visualizer | `♪` |
| Snake | `◆` |
| Monitor | `▥` |
| Settings | `⚙` |

The Browser hover state changes `.app` to `.app hover`; it does not swap
artwork. `.appicon` is a CSS 48×48, radius-16 tinted well with a 20px
Unicode glyph. No library or asset exists.

### Window controls

Every window uses:

```html
<div class="dots">
  <i class="dot red"></i><i class="dot yellow"></i><i class="dot green"></i>
</div>
```

These are CSS circles, not icons. `.dot` is 12×12 with `border-radius:50%`;
red is `#FF5F57`, yellow `#FFBD2E`, and green `#28C840`. The order visually
implies close/minimize/maximize, but the HTML does not add accessible names
or icon package metadata. Source: line 1, `.dots`/`.dot`, every window page.

### Notifications

`DESK-002 — Startup Notification` and related notification states use
`.toast`, text content, status colors, and `.progress` where applicable.
There is no icon element, glyph, SVG, image, or icon library component in
the toast markup. The icon-like treatment is CSS surface, border, text, and
progress geometry only.

## Files icon audit

Source pages: `FILES-001` through `FILES-006`, line 1.

| Purpose | Exact HTML | Implementation |
|---|---|---|
| Back | `<span class="btn">‹</span>` | Unicode text in CSS button |
| Forward | `<span class="btn">›</span>` | Unicode text in CSS button |
| Parent | `<span class="btn">↑</span>` | Unicode text in CSS button |
| Folder row | `▣ Applications`, `▣ Documents`, etc. | Unicode text in `.row` span |
| File row | `▤ kernel.log`, `▤ notes.txt`, etc. | Unicode text in `.row` span |

The Places sidebar (`Home`, `Desktop`, `Documents`, `Downloads`, `System`,
`Trash`) is text-only. No search icon is placed in the `Search files` field.
No SVG/image/library component is present.

## Terminal icon audit

Source pages: `TERM-001 — Idle Prompt` through `TERM-005 — History &
Completion`, line 1. Terminal client content is text-only: prompt, command,
output, errors, and help text. The only Terminal icon-like marks are the
shell `⌘` rail, dock, and launcher glyphs. Window controls are the shared CSS
traffic lights. No Terminal-specific vector or image asset exists.

## Browser icon audit

Source pages: `BROWSER-001 — Ready` through `BROWSER-010 — Security Detail`,
line 1.

| Purpose | Exact markup/text | Type |
|---|---|---|
| Back | `.btn` containing `‹` | Unicode text |
| Forward | `.btn` containing `›` | Unicode text |
| Reload | `.btn` containing `↻` | Unicode text |
| Address marker | `.input` containing `◦` | Unicode text |
| More/options | `.btn` containing `•••` | repeated bullet text |
| Browser dock | `.d` containing `◎` | Unicode text |
| Browser rail | third `.railitem`, `▣` | Unicode text |

No lock/certificate icon is embedded in the security page markup. Security
details are text/content. No SVG/image/library component is present.

## Settings icon audit

Source pages: `Settings — Wi‑Fi`, `Settings — Display`, `Settings — System`,
and `Settings — About`, line 1.

Settings is represented by the sixth shell glyph `⚙` in the rail and dock and
the `⚙` launcher card. Its client sidebar is text-only (`Wi‑Fi`, `Display`,
`System`, `About`). Buttons (`Scan networks`, `Join`, `Cancel`, `Restart`,
`Shut down`) are text in `.btn` elements. Password masking uses repeated
`•`; there is no eye/lock icon. No SVG/image/library component is present.

## CSS/component inventory

| Class/component | Icon status |
|---|---|
| `.railitem` | Unicode text child; CSS well/state |
| `.dock .d` | Unicode text child; CSS well/state/active bar |
| `.appicon` | Unicode text child; CSS well |
| `.dot` | CSS circle geometry, not an asset |
| `.btn` | Generic text button; glyph appears only where markup supplies it |
| `.input` | Text field; `◦`, bullets, or no icon supplied by content |
| `.toast` | No icon; CSS/text surface |
| `.progress` | CSS progress bar, not icon |

There are no `<defs>`, symbols, sprite sheets, icon components, or exported
artwork definitions in the HTML.

## Exact extraction and licensing conclusion

Extractable exactly from the repository:

- literal glyphs and Unicode code points;
- launcher glyph-to-label mapping and order;
- repeated rail/dock sequences;
- classes, page labels, and CSS glyph/container sizes;
- traffic-light dimensions and colors;
- Files and Browser toolbar glyphs.

Not extractable because it is not present:

- SVG path data or viewBoxes;
- raster icon files;
- icon-font identity;
- reusable icon exports/sprites;
- package component names for the design glyphs;
- per-icon accessibility names;
- icon-library license/provenance.

The `lucide-react` catalog entry in `artifacts/mockup-sandbox/package.json`
does not alter this conclusion: no canonical HTML import or Lucide output was
found, and no resolved version/license is recorded there for this audit.

## Final finding

The current canonical NexOS design’s icon system is a set of font-rendered
Unicode marks plus CSS geometry, not a recoverable vector icon library. A
future native icon pipeline may choose to replace these marks, but that would
be a new implementation/design decision rather than extraction of artwork
from the supplied HTML.
