# Aurora Phase 4A — Files and Terminal

## Scope

This phase migrates only the Files and Terminal client interiors. The shell,
window manager, compositor, framebuffer, cursor path, networking, Browser,
and other applications remain outside the phase.

The implementation uses the existing application backends and replaces the
client-area paint/layout layer with Aurora-aware native drawing.

## Files

### Design structure discovered

The canonical Files states in `docs/design/nexos_ui_master.html` are:

- `FILES-001` — root listing
- `FILES-002` — directory listing
- `FILES-003` — empty folder
- `FILES-004` — selected row
- `FILES-005` — error state
- `FILES-006` — file-operation dialog

The common Files composition is a split view: a Places sidebar, a toolbar,
breadcrumb/path and search fields, a padded content area, a rounded table of
rows, and item metadata. The HTML uses a 240px sidebar at the canonical
design size, a 56px toolbar, 26px content padding, rounded rows, and a
subtle item-count footer.

### Native structure implemented

`nexos/kernel/gui/files_app.c` now paints:

- a responsive Places sidebar with Home, Desktop, Documents, Downloads,
  System, and Trash entries;
- a 56px toolbar with back, forward, up, path, and search controls;
- a Home heading and current path subtitle;
- a padded file list with selected-row surface, folder/file glyph slots,
  names, folder labels or formatted sizes, and separators;
- an item-count footer;
- an empty-folder label when the backend returns no entries.

The sidebar, back/forward controls, search field, and Places entries are
presentation surfaces in this phase. Existing NexOS backend support does not
provide history, search, or independent Places navigation, so those controls
remain inert/disabled rather than pretending to implement unsupported
operations. The existing Up action remains connected to parent-directory
navigation.

### Preserved behavior

- VFS directory enumeration and root browsing
- directory traversal by selecting/opening a directory
- parent navigation through the existing Up action
- row selection and double-click directory opening
- existing close and window lifecycle callbacks
- resize-aware painting and clipping
- existing application memory ownership

### Files states

Implemented or represented by the current backend:

- root/directory listing
- selected row
- empty directory presentation
- resize/narrow-window layout fallback
- scrolling through the existing wheel callback path

Not connected because no corresponding backend operation exists in the
current Files implementation:

- back/forward history
- Files search
- independent sidebar location navigation
- file-operation dialog actions
- create, rename, delete, copy, paste, and permission workflows

## Terminal

### Design structure discovered

The canonical Terminal states are:

- `TERM-001` — idle prompt
- `TERM-002` — command entry
- `TERM-003` — command output
- `TERM-004` — command error
- `TERM-005` — history/completion presentation

The HTML Terminal is a dark monospace client surface with 20px padding,
native-shell heading text, a muted help line, prompt/output hierarchy, an
accent prompt, and error text. Completion is represented by a separate card
state in the design.

### Native structure implemented

`nexos/kernel/gui/term_app.c` now paints:

- a canonical dark terminal client surface (`#080b11`);
- a bounded Aurora accent edge;
- `NexOS Terminal - native shell` as the client heading;
- `Type help for commands.` as the helper line;
- monospace command/output lines with a compact 18px line rhythm;
- accent prompt rendering for `[root@nexos]$`;
- red error rendering for `nsh:` lines;
- a bounded blinking cursor inside the client region;
- dynamic visible-row calculation for resize/maximize/restore.

The Terminal backend remains the existing line buffer and shell execution
implementation. Completion/history presentation is not independently
redesigned because the current backend exposes its existing behavior through
the same line buffer and input callbacks.

### Preserved behavior

- command input and editing
- shell command execution
- command output and errors
- existing prompt and line buffer
- existing keyboard callback and shortcuts
- existing history/completion backend behavior
- resize, maximize, restore, focus, and close integration
- bounded cursor invalidation behavior

## Typography and icons

Files headings, labels, metadata, and controls use the centralized Aurora
font roles where the current renderer supports them. Terminal command content
continues to use the existing monospace bitmap path, as required by the
design and by shell readability.

The production icon asset pipeline is deferred. Files uses the existing icon
slots/glyph treatment; the current bitmap font does not render every Unicode
folder/file glyph, so some rows may show fallback glyphs until the separately
planned icon asset integration pass. No procedural icon system was added in
Phase 4A.

## Responsive behavior

Files uses a 180px sidebar at normal application widths, contracts to 132px
for narrower windows, and removes the sidebar below the smallest supported
width. Content and list widths are derived from the live client rectangle.

Terminal derives its visible rows from the live client height and keeps the
client padding and cursor inside the current bounds. Both applications
continue to render inside the existing window-manager client rectangle, so
maximize and restore do not require fixed 1440x900 coordinates.

## Performance and compositor safety

The changes are immediate-mode client painting only. They do not add timers,
per-frame allocations, continuous repainting, direct physical-framebuffer
writes, or changes to the scene/cursor ownership model. Files wheel scrolling
invalidates only its owning window through the existing WM path.

## Validation

Build and regression gates completed successfully:

- `make -C nexos check`
- `make -C nexos test-tcp`
- `make -C nexos kernel`
- `make -C nexos tls-check`
- `make -C nexos netsurf-adapter-check`
- `make -C nexos netsurf-native-link-probe`
- `make -C nexos iso`
- `git diff --check`

The generated ISO was booted under non-interactive QEMU TCG. QEMU reported
the expected 1440x900x32 framebuffer and initialized the GUI with one Files
window and one Terminal window. A monitor `screendump` captured the startup
composition:

- `/tmp/nexos-phase4a-files.png`
- `/tmp/nexos-phase4a-terminal.png`

The captures show both migrated client interiors. Physical mouse and keyboard
interaction, folder opening, terminal command execution, and resize behavior
were not manually performed in this non-interactive boot and remain manual
verification items.

## Known limitations

- The screenshot shows fallback glyphs for some Files row icons because the
  production icon asset pass is deferred.
- Files controls without backend support are intentionally inert/disabled.
- The Terminal completion card from the design is not a new standalone view;
  existing completion behavior remains in the current shell backend.
- No application interiors outside Files and Terminal were changed.
