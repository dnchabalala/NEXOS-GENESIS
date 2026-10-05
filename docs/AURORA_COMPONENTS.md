# Aurora Native Components

Status: Phase 2 foundation. Application migration is not included.

Phase 3 has since adopted these primitives for the desktop, window chrome,
taskbar, Apps launcher, and notification surfaces. Application interiors remain
on their pre-migration paint paths.

## Purpose

Aurora is an immediate-mode native C drawing layer for NexOS. It provides
semantic colors, geometry, typography roles, interaction states, and reusable
draw helpers without introducing HTML, CSS, a retained-mode widget tree, or a
second compositor.

Implementation:

- `nexos/kernel/gui/aurora.h`
- `nexos/kernel/gui/aurora.c`

The source is included by the normal kernel build through `nexos/Makefile`.

## Theme architecture

`aurora_palette_t` retains the existing theme shape: name, legacy surface and
text colors, semantic accents, and five showcase dots. The four existing theme
definitions remain in `theme_app.c` and are now applied through:

    aurora_apply_palette(const aurora_palette_t *palette)

The function updates the legacy `COL_*` symbols used by current applications
and stores the active palette for future semantic consumers. No application
has been migrated to the primitives yet, so existing rendering remains stable.

The current palettes remain supported:

- Catppuccin Mocha;
- Nord;
- Dracula;
- Gruvbox Dark.

The canonical, not-yet-applied migration palette is available through
`aurora_default_palette()`. It contains the Aurora Dark design values from
the supplied specification. Keeping it separate from the active legacy
palette is intentional: Phase 2 must not visually rewrite unmigrated
applications.

Semantic roles are obtained through `aurora_color(role)`. The initial mapping
uses the current palette values so the foundation does not visually rewrite
the desktop before later migration phases.

## Tokens

### Spacing

`AURORA_SPACE_1` through `AURORA_SPACE_6` are 4, 8, 12, 16, 24, and 32 px.

### Radii

Small, medium, large, panel, and dialog radii are provided as compile-time
constants. They are currently 8, 12, 16, 20, and 24 px respectively.

### Dimensions

The foundation defines compact and standard control heights, the current 32 px
titlebar, the current 40 px taskbar, panel/dialog padding, row height, icon
sizes, window control geometry, and scrollbar width.

### Typography roles

The API exposes caption, body, body emphasis, label, section, title, and display
roles. With the current renderer, title/display use the existing 2x bitmap
font and the remaining roles use the existing 8x16 font. This is an explicit
capability mapping, not a claim of variable font support.

## Primitive API

The current native primitives are:

- `aurora_panel`
- `aurora_card`
- `aurora_button`
- `aurora_icon_button`
- `aurora_text_field`
- `aurora_search_field`
- `aurora_tab`
- `aurora_badge`
- `aurora_progress`
- `aurora_list_row`
- `aurora_table_row`
- `aurora_separator`
- `aurora_dialog`
- `aurora_toast`
- `aurora_window_chrome`
- `aurora_app_icon`
- `aurora_signal_indicator`
- `aurora_memory_indicator`
- `aurora_scrollbar`

Example usage:

    aurora_button((aurora_rect_t){ x, y, w, h },
                  "Connect", AURORA_BUTTON_PRIMARY,
                  AURORA_STATE_FOCUSED);

Applications provide geometry, content, variant, and state. They do not need
to provide raw component colors.

## Interaction states

State flags are centralized:

- normal;
- hover;
- pressed;
- focused;
- selected;
- disabled;
- inactive.

Components use these flags to select semantic surfaces and borders. Input
dispatch is intentionally not part of this foundation; existing application
callbacks and the existing WM remain responsible for deciding state.

## Rendering safety

Every primitive uses the existing framebuffer drawing API. Consequently:

- current clipping is respected;
- the active scene render target is respected;
- no primitive calls `fb_commit()`;
- no primitive writes directly to the physical framebuffer;
- no primitive allocates memory during painting;
- no primitive starts timers or invalidates the scene;
- no primitive changes the compositor order;
- no primitive owns cursor pixels.

The caller remains responsible for establishing the correct clip and scene
phase. The primitives do not save or mutate global clip state.

## Renderer approximations

The current implementation deliberately approximates Aurora using native
software primitives:

- glass is represented by opaque/layered dark surfaces and borders;
- focus is represented by semantic borders rather than expensive glow stacks;
- typography uses the existing bitmap font;
- status colors use existing palette roles;
- progress and indicators use solid fills;
- no large blur or full-screen translucent effect was added;
- no SVG, image, or GPU dependency was added.

## Performance rules

- Do not construct component objects every frame.
- Do not allocate during painting.
- Do not perform text measurement repeatedly when callers can cache geometry.
- Do not add continuous animation to static controls.
- Do not call full-screen composition or framebuffer commit from a primitive.
- Do not use blur for ordinary panels.
- Keep large alpha and rounded surfaces bounded.
- Continue using the existing dirty-scene policy.

## Deliberately deferred

This foundation does not migrate:

- desktop appearance;
- window chrome call sites;
- taskbar;
- Apps launcher;
- notifications;
- Files;
- Terminal;
- Browser;
- Settings;
- any other application.

It also does not implement future UI surfaces, dialogs with new behavior,
search, workspaces, accessibility, image support, JavaScript, or networking
changes.

## Phase 3.1 renderer assets

The shell may use the generated `font_aurora_puts()` API with semantic sizes
12, 14, 16, 20, 24, and 32 pixels. The atlas is generated by
`tools/gen_aurora_assets.py` and compiled into
`kernel/drivers/aurora_font_atlas.h`; the legacy 8×16 API remains the
compatibility and Terminal path.

Icon-bearing shell components should use `aurora_icon_id_t`,
`aurora_icon_draw()`, or `aurora_app_icon_id()`. These are native tinted
primitives and do not parse SVG at runtime.

`aurora_panel()` and `aurora_window_chrome()` provide bounded layered depth:
small offset bands, dark surfaces, a one-pixel border, and a restrained top
highlight. They must still be called only during dirty scene composition and
must not own the physical framebuffer.
