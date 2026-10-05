# NexOS Icon Asset Generation

Standalone asset-preparation pass for the canonical design in
`nexos/docs/design/nexos_ui_master.html`. The source audit and slot mapping
are in `docs/NEXOS_ICON_AUDIT.md` and `docs/NEXOS_ICON_MANIFEST.md`.

No GUI source was changed. The generated assets are not wired into
`aurora.c`, the dock, rail, launcher, taskbar, or window manager.

## Fonts identified

The HTML declares this primary stack:

```css
Inter, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif
```

Inter, Apple system fonts, and Segoe UI were not available as matching local
font files. Fontconfig resolution using `Inter:charset=...` produced these
local source decisions:

| Resolution | Local font | Glyphs |
|---|---|---|
| Fallback | `/usr/share/fonts/TTF/DejaVuSans.ttf` | `◉ ▣ ⌘ ♪ ⚙ ◎ ✎ ▥ ▤ ◫ ◐ ◷ ◆ ↑ ↻ ◦` |
| Fallback | `/usr/share/fonts/noto/NotoSans-Regular.ttf` | `+ ‹ › •` |
| Fallback | `/home/dnchabalala/.local/share/fonts/IosevkaNerdFont/IosevkaNerdFont-Regular.ttf` | `⌕` |

The fontconfig result is the best available local approximation to browser
fallback for this asset-only pass. It is not a claim that every browser on
every host renders the canonical HTML identically.

### Primary-font coverage

The declared primary family `Inter` is unavailable locally, so its glyph
coverage cannot be tested from this workspace. The generated masks therefore
use explicit local fallback files. All 21 manifest glyph slots were
successfully rasterized; none was marked `NEEDS_CUSTOM_ASSET`.

The appearance of `⌕` is especially fallback-dependent: fontconfig selected
the local Iosevka Nerd Font. The remaining Unicode symbols were primarily
resolved to DejaVu Sans, with the basic punctuation/navigation glyphs
resolved to Noto Sans.

### Licensing / redistribution

The local font files did not expose license metadata through `fc-query`, and
corresponding local license files were not found in the inspected system
documentation paths. Their licenses are therefore recorded as:

`NOT DETERMINABLE FROM LOCAL FILES — do not commit these font binaries or
redistribute generated assets as product assets until provenance/licensing is
verified.`

No font binary was copied into the repository.

## Generated assets

Source metadata:

- `nexos/assets/icons/source/glyphs.tsv`

Build-time generator:

- `nexos/tools/gen_nexos_icon_assets.py`

Generated alpha-mask PNGs:

- `nexos/assets/icons/generated/`
- 21 stable icon IDs × 5 sizes
- sizes: 16×16, 24×24, 32×32, 48×48, and 64×64
- format: 8-bit grayscale PNG (`L` mode), where black is transparent mask
  coverage and white is fully covered glyph
- no color is baked into the masks; runtime tinting remains possible

Generated metadata:

- `nexos/assets/icons/generated/generated.tsv`

The metadata records ID, stable name, glyph, code point, purpose, target slot,
selected font, license status, and generated sizes.

Stable IDs include:

`NEXOS_ICON_APPS`, `NEXOS_ICON_SEARCH`, `NEXOS_ICON_FILES`,
`NEXOS_ICON_TERMINAL`, `NEXOS_ICON_VISUALIZER`, `NEXOS_ICON_SETTINGS`,
`NEXOS_ICON_BROWSER`, `NEXOS_ICON_EDITOR`, `NEXOS_ICON_MONITOR`,
`NEXOS_ICON_FILE`, `NEXOS_ICON_SYSTEM`, `NEXOS_ICON_THEMES`,
`NEXOS_ICON_CALCULATOR`, `NEXOS_ICON_CLOCK`, `NEXOS_ICON_SNAKE`,
`NEXOS_ICON_BACK`, `NEXOS_ICON_FORWARD`, `NEXOS_ICON_UP`,
`NEXOS_ICON_RELOAD`, `NEXOS_ICON_ADDRESS`, and `NEXOS_ICON_MORE`.

## Reproduced glyphs

All 21 entries in `NEXOS_ICON_MANIFEST.md` were rendered from the exact
Unicode character recorded by the manifest:

| Stable ID | Character | Code point | Selected font |
|---|---|---|---|
| `NEXOS_ICON_APPS` | `◉` | U+25C9 | DejaVu Sans |
| `NEXOS_ICON_SEARCH` | `⌕` | U+2315 | Iosevka Nerd Font |
| `NEXOS_ICON_FILES` | `▣` | U+25A3 | DejaVu Sans |
| `NEXOS_ICON_TERMINAL` | `⌘` | U+2318 | DejaVu Sans |
| `NEXOS_ICON_VISUALIZER` | `♪` | U+266A | DejaVu Sans |
| `NEXOS_ICON_SETTINGS` | `⚙` | U+2699 | DejaVu Sans |
| `NEXOS_ICON_BROWSER` | `◎` | U+25CE | DejaVu Sans |
| `NEXOS_ICON_EDITOR` | `✎` | U+270E | DejaVu Sans |
| `NEXOS_ICON_MONITOR` | `▥` | U+25A5 | DejaVu Sans |
| `NEXOS_ICON_FILE` | `▤` | U+25A4 | DejaVu Sans |
| `NEXOS_ICON_SYSTEM` | `◫` | U+25EB | DejaVu Sans |
| `NEXOS_ICON_THEMES` | `◐` | U+25D0 | DejaVu Sans |
| `NEXOS_ICON_CALCULATOR` | `+` | U+002B | Noto Sans |
| `NEXOS_ICON_CLOCK` | `◷` | U+25F7 | DejaVu Sans |
| `NEXOS_ICON_SNAKE` | `◆` | U+25C6 | DejaVu Sans |
| `NEXOS_ICON_BACK` | `‹` | U+2039 | Noto Sans |
| `NEXOS_ICON_FORWARD` | `›` | U+203A | Noto Sans |
| `NEXOS_ICON_UP` | `↑` | U+2191 | DejaVu Sans |
| `NEXOS_ICON_RELOAD` | `↻` | U+21BB | DejaVu Sans |
| `NEXOS_ICON_ADDRESS` | `◦` | U+25E6 | DejaVu Sans |
| `NEXOS_ICON_MORE` | `•••` | U+2022 × 3 | Noto Sans |

No custom artwork is required for the current glyph set in this local
environment. The output is still font-dependent and must not be treated as a
final cross-platform product icon set until the licensed production font
source is selected.

## Contact sheet

Generated verification artifact:

`/tmp/nexos-icon-contact-sheet.png`

The sheet is 1040×672 RGB PNG and shows every generated icon at 48px with its
stable ID, source glyph/code point, target purpose, and selected font.

## CSS traffic lights

No image assets were generated for window controls. The canonical design
implements them as CSS geometry:

- 12×12 circles;
- 10px gap;
- red `#FF5F57`;
- yellow `#FFBD2E`;
- green `#28C840`;
- `.dot { border-radius: 50%; }`.

They remain native controls and are intentionally excluded from the alpha
mask output.

## Asset memory footprint

The generated PNG files occupy approximately 33,425 bytes on disk after PNG
compression. Uncompressed alpha-mask storage, if loaded as one byte per pixel,
is:

```text
21 × (16² + 24² + 32² + 48² + 64²)
= 21 × 7,936
= 166,656 bytes
```

This is an estimate of raw mask storage, not a runtime allocation. No runtime
loader or cache was added in this pass.

## Reproduction command

From the repository root:

```sh
python3 nexos/tools/gen_nexos_icon_assets.py /tmp/nexos-icon-contact-sheet.png
```

The generator uses Python Pillow, `fc-match`, and the source metadata TSV.
It emits the five requested sizes and rewrites the generated metadata and
contact sheet. It does not modify GUI source or wire assets into the build.

## Unresolved/custom-artwork status

- `NEEDS_CUSTOM_ASSET`: none for the 21 current Unicode slots.
- `CUSTOM/UNKNOWN ARTWORK`: none identified in the canonical HTML.
- Window traffic lights: CSS-only and intentionally not rasterized.
- Future replacement artwork: still required if the product needs a
  consistent icon family independent of host font fallback.

## Validation performed

- Read `docs/NEXOS_ICON_AUDIT.md`.
- Read `docs/NEXOS_ICON_MANIFEST.md`.
- Read the canonical HTML icon markup and CSS.
- Resolved each code point with local fontconfig.
- Rasterized all 21 glyphs at five sizes.
- Verified PNG type/dimensions and inspected the contact sheet.
- `git diff --check` passes for the created text/script files.

No GUI/runtime tests were run because the assets are intentionally not wired
into NexOS yet.
