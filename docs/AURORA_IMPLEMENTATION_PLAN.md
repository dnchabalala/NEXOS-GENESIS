# Aurora UI — NexOS Implementation Plan

Status: Phase 3 shell migration implemented.
Implementation status: Phase 4 application migration is NOT AUTHORIZED.
Date: 2026-10-04

## 0. Executive result

The canonical design is `nexos/docs/design/nexos_ui_master.html` and
`nexos/docs/design/nexos_ui_master.pdf`. The PDF contains 77 pages/states at
1080 x 675.12 points, corresponding to the supplied 1440 x 900 design
presentation. The HTML and PDF describe the same Aurora visual system.

Aurora is a presentation-layer migration, not a new OS architecture. The
current GUI already has a working compositor, backbuffer, window manager,
taskbar, launcher, notifications, and twelve GUI applications. The Browser is
NetSurf-backed and its networking/TLS path is proven. The migration must
replace presentation code incrementally while preserving those behaviors.

Phase 0 produced this plan. Phase 2 adds only the shared Aurora foundation;
application presentation migration remains deferred.

## 0.1 Phase 2 implementation result

Phase 2 is complete at the foundation level. The implementation adds
`nexos/kernel/gui/aurora.h` and `aurora.c`, included by the kernel build,
with an immediate-mode semantic palette and native component helpers. The
foundation owns the compatibility palette symbols previously defined by the
framebuffer module, while preserving their existing names for unmigrated
applications.

The four existing palettes remain in `theme_app.c` and are applied through
`aurora_apply_palette()`. No application paint callback, compositor phase,
input path, browser path, or backend subsystem was migrated in this phase.
The primitives render through the existing framebuffer/font APIs, do not
allocate or commit, and therefore preserve the cursor-free scene and dirty
presentation model.

The canonical Aurora Dark palette is exposed by
`aurora_default_palette()`. It is intentionally not applied to the current
desktop until Phase 3 call sites are migrated; this preserves the accepted
pre-migration appearance while making the target palette available to every
future consumer.

The component API and usage rules are documented in
`docs/AURORA_COMPONENTS.md`. Phase 3 shell migration is documented in
`docs/AURORA_PHASE3_SHELL.md`; application migration remains explicitly
blocked pending separate authorization.

## 1. Understanding of the canonical Aurora design

The design direction is:

- premium dark surfaces;
- calm hierarchy and generous spacing;
- indigo/lavender primary accent;
- green success and red danger semantics;
- restrained borders and depth;
- rounded cards and controls;
- focused/elevated surface states;
- compact status pills and badges;
- reusable buttons, fields, tabs, rows, tables, progress bars, and toasts;
- short 160–220 ms micro-transitions;
- motion that never blocks responsiveness;
- a coherent shell surrounding simple native applications.

Canonical reference colors include:

    Base       #070A12
    Surface    #0D111B
    Elevated   #111623
    Accent     #7C8CFF
    Success    #5ED69A
    Danger     #FF6868

Canonical spacing is 4 / 8 / 12 / 16 / 24 / 32 px. Canonical corner radii
range from 12 to 28 px. These are design values, not yet runtime values.

The design presents the following implemented-looking surfaces:

- boot console;
- installer selection, progress, success, and failure;
- fallback shell;
- desktop variants;
- Apps launcher and hover state;
- taskbar and network variants;
- Files states;
- Terminal states;
- Browser states;
- Settings tabs;
- System Information;
- Themes;
- Calculator;
- Clock;
- Editor;
- Visualizer;
- Snake;
- System Monitor;
- component-library specimens.

It also includes explicitly future-oriented pages. They are not current GUI
features and must not be made to appear functional without backend support:

- Control Centre;
- Universal Search;
- Notification Centre;
- Power Menu;
- Workspace Overview;
- Package Manager UI;
- Log Viewer;
- Storage Manager;
- Process Actions;
- Display and Input Settings;
- Accessibility;
- Security and Certificates.

## 2. Existing NexOS GUI architecture

### Startup and ownership

`nexos/kernel/gui/gui.c` starts the graphical phase. It enables the GUI
backbuffer, disables framebuffer-console drawing, initializes mouse, window
manager, taskbar, and notifications, creates one Terminal and one Files
window, composes the initial desktop, and enters the event loop.

After GUI ownership begins, the graphical display is composed through the GUI
backbuffer and committed to the physical framebuffer. Kernel/debug output is
kept on serial rather than being allowed to scribble over the desktop.

### Event path

    PS/2 keyboard/mouse
        -> keyboard/mouse driver
        -> gui_main event loop
        -> launcher/taskbar/window-manager dispatch
        -> application callbacks
        -> scene invalidation
        -> ordered composition
        -> framebuffer commit

### Scene composition

Current intended order:

    desktop
        -> visible windows back-to-front
        -> taskbar
        -> launcher/notifications
        -> cursor
        -> physical framebuffer commit

The authoritative scene is cursor-free. Cursor-only presentation is separate
from scene composition. This invariant must remain intact during Aurora work.

### Window manager

Source: `nexos/kernel/gui/wm.c` and `wm.h`.

Current capabilities:

- maximum of 16 windows;
- focus and z-order;
- normal, minimized, and maximized states;
- titlebar dragging with mouse capture;
- close, minimize, maximize/restore controls;
- application paint/click/key/mouse/wheel/resize/close callbacks;
- client-area clipping;
- taskbar window-pill integration.

Current shared geometry:

    titlebar height:        32 px
    window control radius:   7 px
    window control gap:     22 px
    shadow offset:           4 px

### Framebuffer and renderer

Sources: `nexos/kernel/drivers/fb.c`, `fb.h`, and `font.c`.

Capabilities:

- linear framebuffer;
- draw backbuffer;
- full commit;
- bounded rectangle commit;
- clipping;
- pixels, rectangles, lines, circles;
- rounded rectangles;
- borders;
- alpha blending;
- rectangle blending;
- in-place blur;
- rectangle copy;
- bitmap glyph rendering;
- 8x16 bitmap font and approximately 16x32 enlarged text.

Current constraints:

- software rasterization;
- no general font loading;
- no anti-aliased font renderer;
- no general SVG/icon asset pipeline;
- Browser image decoder integration is incomplete;
- large alpha, blur, gradient, and full-frame operations are expensive.

### Input

Sources: `keyboard.c/.h` and `mouse.c/.h`.

- PS/2 set-1 keyboard;
- PS/2 IntelliMouse-compatible four-byte mouse packets;
- left/right/middle buttons;
- wheel delta;
- software cursor;
- pointer clamping to framebuffer bounds;
- titlebar drag capture;
- window and Browser pointer forwarding.

There is no general widget toolkit, clipboard abstraction, double-click
abstraction, or keyboard focus-navigation framework.

### Existing GUI modules

- `desktop.c/.h` — procedural dark aurora desktop;
- `taskbar.c/.h` — Apps, window pills, clock, memory, network;
- `launcher.c/.h` — Apps panel, app grid, power actions;
- `notif.c/.h` — bounded toast notifications;
- `browser_app.c/.h` — Browser chrome and NetSurf bridge;
- `files_app.c/.h` — VFS browser;
- `term_app.c/.h` — shell terminal;
- `settings_app.c/.h` — WiFi, Display, System, About;
- `sysinfo_app.c/.h` — system/memory information;
- `theme_app.c/.h` — four runtime themes;
- `calc_app.c/.h` — integer calculator;
- `clock_app.c/.h` — RTC clock;
- `edit_app.c/.h` — text editor;
- `viz_app.c/.h` — animated visualizer;
- `snake_app.c/.h` — keyboard game;
- `sysmon_app.c/.h` — live monitor.

## 3. Renderer capability assessment

### Cheap

- opaque fills;
- small borders and separators;
- bitmap text;
- small opaque icons;
- compact progress bars;
- small cursor updates;
- simple list and table rows.

### Moderate

- rounded rectangles;
- circles;
- limited alpha blending;
- window shadows;
- focus rings;
- small animated controls;
- compact status indicators.

### Expensive

- full-screen gradients;
- large translucent surfaces;
- large blur regions;
- many overlapping translucent windows;
- repeated full-scene composition;
- large text-operation counts;
- continuously animated windows;
- full-frame commits.

### Currently unsupported or incomplete

- GPU acceleration;
- external font loading;
- anti-aliased typography;
- general SVG icons;
- complete freestanding Browser image decoding;
- JavaScript;
- video;
- modern browser application behavior dependent on JavaScript.

Visual fidelity must be achieved primarily with opaque/mostly opaque native
primitives, restrained effects, and bounded animation.

## 4. Aurora token specification

These are the proposed implementation tokens derived from the canonical
design. They are not yet runtime definitions.

### Semantic colors

    color.base              #070A12
    color.surface           #0D111B
    color.elevated          #111623
    color.overlay           rgba(0,0,0,0.28)
    color.border            derived low-contrast line
    color.border_focus      #7C8CFF
    color.text_primary      light neutral
    color.text_secondary    muted neutral
    color.text_tertiary     subdued neutral
    color.accent            #7C8CFF
    color.success           #5ED69A
    color.warning           existing yellow semantic
    color.danger            #FF6868

The current Catppuccin, Nord, Dracula, and Gruvbox palettes should initially
be represented as compatibility palettes behind semantic roles rather than
deleted or replaced blindly.

### Spacing

    space_1:  4 px
    space_2:  8 px
    space_3: 12 px
    space_4: 16 px
    space_5: 24 px
    space_6: 32 px

### Radii

    radius_sm:  8–12 px
    radius_md: 12–16 px
    radius_lg: 20–28 px

### Core dimensions

    titlebar height:       existing 32 px; Aurora target requires review
    taskbar height:        existing 40 px; preserve initially
    compact control:       approximately 28–32 px
    standard control:      approximately 36 px
    launcher card:         existing 110x112 px; migrate after shell review
    window controls:       existing 7 px radius; preserve hit targets

### Typography roles

    display:       40–48 px design reference; bitmap fallback required
    section:       20–24 px design reference; bitmap fallback required
    body/control:  13–16 px design reference; 8x16 runtime baseline
    metadata:      11–13 px design reference; bitmap-compatible fallback
    monospace:     Terminal/editor/network diagnostics

### State roles

- normal;
- hover;
- pressed;
- focused;
- selected;
- active;
- inactive;
- disabled;
- loading;
- success;
- warning;
- error;
- offline;
- connected;
- scanning;
- password-entry;
- progress;
- empty.

## 5. Native UI primitive architecture

Do not create a web-style DOM or CSS layer. Use native C drawing helpers and
existing framebuffer primitives.

Candidate primitives, subject to repository naming review:

- `ui_panel` — bounded opaque/elevated surface;
- `ui_card` — card with radius, border, and state;
- `ui_button` — standard semantic button;
- `ui_icon_button` — compact toolbar/window action;
- `ui_text_field` — text entry and focus state;
- `ui_password_field` — masked text entry;
- `ui_search_field` — visual/search input, only functional where backed;
- `ui_tab` — active/inactive tab;
- `ui_badge` — compact status label;
- `ui_progress` — progress track and fill;
- `ui_list_row` — normal/hover/selected row;
- `ui_table_row` — table-specific row;
- `ui_separator` — divider/rule;
- `ui_dialog` — only when a native modal surface is actually needed;
- `ui_toast` — notification surface;
- `ui_window_chrome` — titlebar, title, controls, focus state;
- `ui_app_icon` — letter/icon treatment with semantic color;
- `ui_signal_indicator` — WiFi bars/status;
- `ui_memory_indicator` — memory status;
- `ui_scrollbar` — only when an application has real scroll behavior.

Each primitive must draw through the active GUI render target and obey the
current clip. No primitive may write directly to the physical framebuffer.
Application callbacks must not leak clip or render-target state.

## 6. Design-to-code mapping and migration plan

| Surface | Current source | Reusable Aurora work | Backend preserved | Risk | Order |
|---|---|---|---|---|---|
| Desktop | `desktop.c` | background, watermark, static shell | desktop phase | Medium | 1 |
| Window chrome | `wm.c` | titlebar, controls, focus, shadow | WM behavior | High | 1 |
| Taskbar | `taskbar.c` | Apps, pills, indicators | RTC/memory/network | Medium | 1 |
| Launcher | `launcher.c` | panel, cards, search placeholder, power | launch/reset/shutdown | Medium | 1 |
| Notifications | `notif.c` | toast primitive and states | queue/timing | Medium | 1 |
| Files | `files_app.c` | toolbar, rows, empty/selected | VFS | Medium | 2 |
| Terminal | `term_app.c` | terminal shell/status surfaces | shell | Medium | 2 |
| Settings | `settings_app.c` | tabs, cards, fields, status | WiFi/RTC/memory/netif | Medium | 2 |
| Browser | `browser_app.c`, NetSurf frontend | toolbar/status/error chrome | NetSurf/fetch/TLS | High | 2 |
| System Information | `sysinfo_app.c` | cards/meters | PMM/timer | Low | 3 |
| Themes | `theme_app.c` | theme cards and token mapping | palette switching | High | 3 |
| Calculator | `calc_app.c` | keypad/button states | calculator logic | Low | 3 |
| Clock | `clock_app.c` | time surface | RTC | Low | 3 |
| Editor | `edit_app.c` | toolbar/status/prompt | editor buffer/VFS | Medium | 3 |
| Visualizer | `viz_app.c` | panel/status treatment | visualizer model | Low | 3 |
| Snake | `snake_app.c` | game header/status | game logic | Low | 3 |
| System Monitor | `sysmon_app.c` | cards/table/charts | process/heap/PMM | Medium | 3 |
| Boot | `vga.c`, `console.c`, `kernel.c` | Aurora boot presentation where safe | boot flow | Medium | 4 |
| Installer | `installer.c` | installer visual treatment | installer backend | High | 4 |

## 7. Exact migration order

### Checkpoint aurora-01-foundation

Implemented in Phase 2:

- token structure and semantic palette;
- compatibility mapping for current themes;
- typography roles;
- spacing/radius/dimension constants;
- primitive drawing API;
- clip/render-target guard policy;
- primitive-level host/build checks where practical.

Acceptance gate: passed for compile-time/build validation. Existing GUI
behavior and compositor remain unchanged because application call sites were
not migrated.

### Checkpoint aurora-02-shell

Migrate in this order:

1. Desktop;
2. Window chrome;
3. Taskbar;
4. Apps launcher;
5. Notifications.

Verify before continuing:

- clean startup;
- one Files and one Terminal window;
- focus, drag, minimize, maximize, restore, close;
- Apps open/dismiss;
- taskbar pills;
- notifications;
- no cursor trails;
- no window trails;
- no idle repaint storm;
- no clipping leaks.

### Checkpoint aurora-03-files-terminal

- migrate Files chrome and states;
- preserve VFS traversal and selection;
- migrate Terminal chrome;
- preserve shell commands, history, completion, pipes, redirection, and GUI
  launch aliases.

### Checkpoint aurora-04-settings-browser

- migrate Settings tabs/cards/fields/status;
- migrate Browser toolbar, URL field, status, loading/error states;
- preserve NetSurf, HTTPS, navigation, scrolling, hover, history, reload,
  resize/reformat, clipping, and frontend callbacks.

### Checkpoint aurora-05-apps

Migrate System Information, Themes, Calculator, Clock, Editor, Visualizer,
Snake, and System Monitor using the same primitives.

### Checkpoint aurora-06-system-states

Complete supported variants:

- empty;
- loading;
- error;
- disabled;
- selected;
- focused;
- inactive;
- offline;
- connected;
- scanning;
- password entry;
- success;
- warning;
- failure;
- progress;
- installer complete/failure where the backend exposes those states.

### Checkpoint aurora-07-polish

Only after correctness and performance verification:

- spacing refinements;
- restrained effect tuning;
- animation duration tuning;
- typography alignment;
- icon consistency;
- visual comparison against canonical design.

## 8. Unsupported and future surfaces

### Implement now

- visual migration of all existing GUI applications;
- existing desktop, taskbar, launcher, window, and notification states;
- existing Browser chrome and frontend presentation;
- existing Settings controls and read-only panels;
- existing installer/boot presentation where safe.

### UI-only / backend currently available but no GUI surface

These may be considered only after the existing migration:

- package-manager surface around `npkg`;
- log viewer around kernel/serial logs;
- storage surface around ATA/VFS/mount state;
- process actions around existing `kill` support;
- network diagnostics around DNS/ping/interface/TLS data;
- file operation dialogs around existing VFS commands;
- security/certificate detail around verified TLS state.

### Future / backend not available as a complete product surface

- Control Centre;
- Universal Search;
- Notification Centre;
- Workspace Overview;
- Accessibility settings;
- audio controls;
- Bluetooth controls;
- full display/input settings;
- lock screen/login;
- workspace management.

Do not wire buttons that only exist in the design reference. A dormant visual
component is acceptable only if it is genuinely reusable and cannot imply that
the feature works.

## 9. Performance budget

### Keep cheap

- opaque panels;
- small borders and separators;
- bitmap text;
- small status badges;
- small icon shapes;
- compact progress bars;
- bounded cursor updates.

### Use selectively

- rounded rectangles;
- alpha overlays;
- focus glows;
- window shadows;
- animated hover states;
- small cards and signal indicators.

### Approximate or simplify

- large blur regions;
- large translucent surfaces;
- full-screen glass;
- complex shadow stacks;
- continuous gradients;
- simultaneous application animation;
- large decorative background effects.

### Preserve performance invariants

- no unconditional idle full-screen repaint;
- cursor-only movement must not repaint NetSurf content;
- active drag may use correctness-first full scene composition;
- one ordered scene composition;
- one controlled physical framebuffer presentation;
- bounded animation and scheduler work;
- no Browser-specific compositor path.

## 10. Highest-risk areas

1. Window chrome migration: affects every application and pointer target.
2. Render-target and clipping state: prior cursor/window trails show that
   state leakage can corrupt presentation.
3. Taskbar/window z-order: taskbar must remain above ordinary windows and
   below cursor/overlays as currently defined.
4. Browser frontend: NetSurf must continue drawing only inside its viewport.
5. Theme migration: current applications use global palette variables and
   hardcoded literals; conversion must preserve all four runtime themes.
6. Typography: canonical design assumes richer type than the current bitmap
   font can provide.
7. Animation: blur and decorative effects can reintroduce repaint storms.
8. Installer/boot: these use different presentation assumptions from the GUI.
9. Fixed-size application layouts: Aurora spacing must not silently break
   existing windows at 1024x768.

## 11. Likely files to change after authorization

Foundation:

- new GUI token/component files, location to be decided after review;
- `nexos/kernel/drivers/fb.h` and possibly `fb.c` only if primitive support
  is genuinely required;
- `nexos/kernel/gui/theme_app.c/.h`;
- `nexos/kernel/gui/anim.h`.

Shell:

- `nexos/kernel/gui/desktop.c/.h`;
- `nexos/kernel/gui/wm.c/.h`;
- `nexos/kernel/gui/taskbar.c/.h`;
- `nexos/kernel/gui/launcher.c/.h`;
- `nexos/kernel/gui/notif.c/.h`;
- `nexos/kernel/gui/gui.c` only for integration/invalidation if required.

Applications:

- `files_app.c/.h`;
- `term_app.c/.h`;
- `settings_app.c/.h`;
- `browser_app.c/.h`;
- `sysinfo_app.c/.h`;
- `theme_app.c/.h`;
- `calc_app.c/.h`;
- `clock_app.c/.h`;
- `edit_app.c/.h`;
- `viz_app.c/.h`;
- `snake_app.c/.h`;
- `sysmon_app.c/.h`.

Boot/installer only after GUI migration is stable:

- `nexos/kernel/drivers/vga.c/.h`;
- `nexos/kernel/drivers/console.c/.h`;
- `nexos/kernel/installer/installer.c/.h`;
- `nexos/kernel/kernel.c` only if presentation handoff requires it.

Must remain untouched unless direct regression evidence requires otherwise:

- DNS;
- TCP;
- HTTP;
- Mbed TLS;
- entropy;
- NetSurf engine;
- NetSurf fetcher;
- VFS implementation;
- shell command implementation;
- process scheduler/task infrastructure;
- PS/2 packet decoding;
- mouse motion/button semantics;
- keyboard driver.

## 12. Files that must be preserved as design references

- `nexos/docs/design/nexos_ui_master.html`;
- `nexos/docs/design/nexos_ui_master.pdf`.

These files are visual specifications. They are not a runtime widget library
and must not be copied into the kernel as HTML/CSS.

## 13. Testing and migration gates

After every implementation checkpoint, run:

    make -C nexos check
    make -C nexos test-tcp
    make -C nexos kernel
    make -C nexos tls-check
    make -C nexos netsurf-adapter-check
    make -C nexos netsurf-native-link-probe
    make -C nexos iso
    git diff --check

Runtime acceptance must include generic QEMU and the browser-capable
interactive QEMU target.

Shell acceptance:

- boot;
- clean desktop;
- cursor movement;
- titlebar drag;
- click/focus;
- minimize/restore;
- maximize/restore;
- close;
- Apps open/dismiss;
- startup Files and Terminal;
- no stale pixels or cursor/window trails;
- idle scene remains quiet.

Application acceptance:

- Files browse/select/parent navigation;
- Terminal commands/history/completion and GUI launch aliases;
- Settings WiFi interaction and read-only states;
- Browser HTTPS navigation, scroll, links, Back, Forward, Reload, and
  maximize/reformat;
- each remaining app opens, paints, receives its existing controls, and closes.

## 14. Blockers discovered in Phase 0

1. The canonical design assumes a type hierarchy richer than the current
   bitmap font. A runtime-compatible typography mapping is required.
2. The canonical Aurora palette differs from the current Catppuccin runtime
   palette. Semantic token mapping must be defined before migration.
3. The current renderer has no general icon asset pipeline.
4. The current GUI has no general widget/focus/clipboard/dialog framework.
5. Several design pages represent future functionality rather than existing
   functionality. They cannot be wired without backend/API work.
6. Many applications have fixed dimensions and application-specific drawing;
   the shell must be migrated before aggressive app resizing.
7. Blur, alpha, gradients, and animation have measurable software-rendering
   cost and must remain bounded.
8. Browser rendering must remain inside the existing NetSurf viewport and
   compositor path.

These blockers do not prevent implementation of the existing-surface migration.
They prevent treating the supplied design as a direct HTML/CSS port or making
future screens functional without backend support.

## 15. Is the supplied design sufficient to begin implementation?

Yes, for an incremental migration of existing surfaces.

The canonical HTML/PDF specifies the visual direction, core component examples,
screen/state inventory, colors, spacing, radii, typography intent, and motion
intent. The repository supplies the behavioral implementation and renderer
constraints.

It is not sufficient to implement unsupported future products without additional
product/API decisions. Those surfaces must remain explicitly nonfunctional or
be deferred.

Phase 2 foundation implementation is complete. The structural Phase 3 shell
reconstruction is recorded in `docs/AURORA_PHASE3_RECONSTRUCTION.md`; app
interior migration remains explicitly deferred to Phase 4.

## 16. Phase 3 reconstruction checkpoint

The shell presentation now uses the HTML-derived top bar, left rail, floating
dock, 54px titlebars, 28px window frame radius, 520px launcher modal, and
320px notification surfaces. No application interior migration or future
backend feature was started. The compositor remains cursor-free-scene based.

## 17. Phase 3.1 fidelity checkpoint

The shell fidelity pass adds the preferred 1440×900 Multiboot/GRUB mode with
safe fallbacks, a generated native sans atlas, procedural tinted icons, and
shared layered surface depth. These changes are constrained to the shell and
Aurora foundation; application client interiors remain legacy until Phase 4
is explicitly authorized.
