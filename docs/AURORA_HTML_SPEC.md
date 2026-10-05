# NexOS Aurora HTML Design Specification

## Phase 3.1 fidelity checkpoint

The native shell now targets the preferred 1440×900 framebuffer mode, with
optional 1280×720 and 1024×768 fallbacks. A generated native sans atlas
provides shell text roles at 12, 14, 16, 20, 24, and 32 pixels. Native tinted
icon primitives replace shell/launcher letter placeholders. These are
renderer-aware approximations of the HTML's Inter and icon artwork; client
interiors remain outside this phase.

Source files:

- `nexos/docs/design/nexos_ui_master.html`
- `nexos/docs/design/nexos_ui_master.pdf`

The HTML is a single self-contained document containing 77 design pages at
1440×900. Each `.page` is a 1440×900 absolute-positioned state and carries a
`data-label`. The PDF is the visual verification reference.

## Design tokens extracted from `:root`

| Token | Value |
|---|---|
| `--base` | `#070A12` |
| `--surface` | `#0D111B` |
| `--elev` | `#111623` |
| `--accent` | `#7C8CFF` |
| `--muted` | `#8A91A3` |
| `--good` | `#5ED69A` |
| `--warn` | `#FFCC66` |
| `--bad` | `#FF6868` |
| `--line` | `rgba(255,255,255,.085)` |

Additional literal colors used repeatedly:

- page background: `#04060A`
- top/deep background: `#090D17`, `#070910`
- boot/fallback background: `#05070B`
- terminal background: `#080B11`
- primary text: `#F7F8FC`
- accent text: `#DBE0FF`, `#AEB8FF`, `#CDD4FF`
- muted text: `rgba(255,255,255,.30/.38/.42/.48/.56/.62/.70)`
- success text: `#BDF8D7`, `#93E9BC`
- error text: `#FFC1C1`, `#FF9B9B`
- green window control: `#28C840`
- yellow window control: `#FFBD2E`
- red window control: `#FF5F57`

## Global CSS component geometry

| Component/class | Extracted implementation |
|---|---|
| `.page` | 1440×900, relative, overflow hidden, page break |
| `.grid` | absolute inset 0; opacity `.12`; 32×32 radial dot grid |
| `.top` | left/right 16, top 12, height 44, radius 16, 1px line, horizontal padding 16, font 13px |
| `.top b` | 14px |
| `.top .sub` | left margin 16, muted |
| `.leftbar` | left 18, top 82, width 72, height 650, radius 24, padding 16px 12px, gap 18 |
| `.railitem` | 48×48, radius 16, surface overlay, 20px icon/text |
| `.window` | left 116, top 88, width 1220, height 744, radius 28, elevated background, 1px line |
| `.titlebar` | height 54, horizontal padding 18, bottom line |
| `.dots` | flex, gap 10 |
| `.dot` | 12×12 circle |
| `.wtitle` | centered, 13px, muted, pointer-events none |
| `.client` | height 690, relative |
| `.dock` | centered, bottom 24, height 66, padding 9px 16px, gap 12, radius 24 |
| `.dock .d` | 48×48, radius 16, 20px icon/text |
| active dock indicator | 16×4, bottom -7, radius 4, accent |
| `.h1` | 40px, line-height 1.06, semibold, letter-spacing -0.035em |
| `.h2` | 24px, semibold, letter-spacing -0.02em |
| `.eyebrow` | 11px, letter spacing .22em, bold, lavender |
| `.body` | 15px, line-height 1.65, muted |
| `.tiny` | 11px, low-contrast muted |
| `.card` | 1px line, radius 22, translucent surface, padding 20 |
| `.card.focus` | accent border/background |
| `.row` | flex, top line, padding 13px 14px, 14px text |
| `.pill` | inline flex, radius 999, padding 6px 10px, 11px text |
| `.btn` | radius 12, padding 10px 14px, 13px text |
| `.input` | height 38, radius 12, padding 0 13px, 13px text |
| `.progress` | height 6, radius 6 |
| `.split` | grid columns 240px + 1fr, full height |
| `.side` | right border, padding 18, subtle surface |
| `.navitem` | padding 11px 12px, radius 12, 14px, bottom margin 4 |
| `.content` | padding 26px |
| `.toolbar` | height 56, padding 0 14px, gap 9, bottom line |
| `.kbd` | padding 3px 7px, radius 6, 10px |
| `.toast` | width 320, radius 18, padding 15, elevated surface |
| `.modal` | width 520, radius 26, padding 24, deep shadow |
| `.appgrid` | four equal columns, gap 14 |
| `.app` | radius 18, padding 15, 1px border |
| `.appicon` | 48×48, radius 16, centered, 20px icon |
| `.term` | monospace, padding 20, 14px, line-height 1.7 |
| `.table` | radius 18, overflow hidden, 1px line |
| `.thead/.trow` | columns 90px 1fr 120px 120px, padding 11px 14px |
| `.game` | height 430, radius 20, 18px grid |
| `.future` | bottom/right 22, radius 999, padding 7px 10px |

## Shell hierarchy

The intended normal desktop composition is:

```text
1440×900 page
├── static radial/linear Aurora background
├── 32px dot grid
├── top system bar: x16 y12 w1408 h44
├── left rail: x18 y82 w72 h650
├── main window: x116 y88 w1220 h744
│   ├── titlebar: h54
│   └── client: h690
└── floating dock: centered, bottom24, h66
```

The main window overlaps the rail region horizontally in the source design;
the rail is a shell navigation surface behind/alongside the primary window.
The dock is floating and is not a full-width taskbar.

## Screen labels extracted from the HTML

### Design/system

- NexOS Design System — Aurora
- NexOS Component Library
- SYS-001 — Boot Console
- SYS-002 — Installer Device Selection
- SYS-003 — Installer Progress
- SYS-004A — Installation Complete
- SYS-004B — Installation Failed
- SYS-005 — Fallback Shell

### Desktop/shell

- DESK-001 — Desktop Idle
- DESK-002 — Startup Notification
- DESK-003 — Multiple Windows
- DESK-004 — Empty Desktop
- LAUNCH-002 — Apps Launcher Open
- LAUNCH-003 — App Hover State
- TASK-001 — Normal Taskbar
- TASK-002 — Active Window
- TASK-003 — Minimized Window
- TASK-004 — Network Variants

### Files

- FILES-001 — Root Listing
- FILES-002 — Directory Listing
- FILES-003 — Empty Folder
- FILES-004 — Selected Row
- FILES-005 — Error State
- FILES-006 — File Operation Dialog

### Terminal

- TERM-001 — Idle Prompt
- TERM-002 — Command Entry
- TERM-003 — Command Output
- TERM-004 — Command Error
- TERM-005 — History & Completion

### Browser

- BROWSER-001 — Ready
- BROWSER-002 — Address Focus
- BROWSER-003 — Loading
- BROWSER-004 — Loaded HTML
- BROWSER-005 — Error
- BROWSER-006 — Link Hover / Status
- BROWSER-007 — History Navigation
- BROWSER-008 — Maximized / Reflow
- BROWSER-009 — Unsupported Content
- BROWSER-010 — Security Detail

### Settings and existing applications

- Settings — Wi-Fi
- Settings — Display
- Settings — System
- Settings — About
- SYSINFO-001 — System Information
- SYSINFO-002 — Memory Detail
- THEME-001 — Theme Selector
- THEME-002 — Active Theme Card
- CALC-001 — Calculator
- CALC-002 — Calculator Error
- CLOCK-001 — Clock
- EDITOR-001 — Empty Editor
- EDITOR-002 — Modified Document
- EDITOR-003 — Save Prompt
- EDITOR-004 — Open Prompt
- EDITOR-005 — Cursor / Navigation
- VIZ-001 — Visualizer
- SNAKE-001 — Active
- SNAKE-002 — Paused
- SNAKE-003 — Game Over
- SYSMON-001 — Live Monitor
- SYSMON-002 — Process Table

### Future-only labels

- FUTURE — Control Centre
- FUTURE — Universal Search
- FUTURE — Notification Centre
- FUTURE — Power Menu
- FUTURE — Workspace Overview
- FUTURE — Package Manager
- FUTURE — Log Viewer
- FUTURE — Storage Manager
- FUTURE — Process Actions
- FUTURE — Display & Input Settings
- FUTURE — Accessibility
- FUTURE — Security & Certificates

## Shell interaction states

- Desktop: idle, startup notification, multiple windows, empty desktop.
- Rail: normal and active item.
- Window: normal, focused, inactive, overlapped, maximized, minimized.
- Dock: normal, active running app, active indicator.
- Launcher: open, hover card, scrim, search placeholder, power controls.
- Notification: visible toast, progress/lifetime, success/warning/error pills.
- Controls: primary, secondary, accent, danger, focused input, selected row.

## HTML-to-native mapping

| HTML component | Native NexOS primitive/API | Source | Backend behavior |
|---|---|---|---|
| `.page`/background/grid | framebuffer background renderer | `desktop.c` | static desktop repaint |
| `.top` | new shell top-bar paint surface | `gui.c`/`taskbar.c` | RTC/network status reads |
| `.leftbar`/`.railitem` | shell rail paint/hit regions | `gui.c`/`taskbar.c` | existing supported app launch/focus only |
| `.window`/`.titlebar`/`.dots` | WM frame and `aurora_window_chrome` | `wm.c`, `aurora.c` | focus, drag, min/max/close/resize |
| `.dock`/`.d` | floating dock renderer | `taskbar.c` | Apps, running-window focus/restore |
| `.card` | `aurora_card` | `aurora.c` | visual surface only |
| `.app`/`.appicon` | `aurora_card` + `aurora_app_icon` | `launcher.c` | existing app launch callbacks |
| `.btn` | `aurora_button` | `aurora.c` | existing callback/hit logic |
| `.input` | `aurora_text_field`/`aurora_search_field` | `aurora.c` | only existing fields are functional |
| `.pill`/`.badge` | `aurora_badge` | `aurora.c` | status display |
| `.toast` | `aurora_panel`/`aurora_toast` | `notif.c` | existing notification queue/lifetime |
| `.toolbar` | existing app toolbar bounds | app modules | app-specific, Phase 4 interior migration |
| `.split`/`.side`/`.navitem` | existing app layouts | app modules | app-specific, Phase 4 |
| `.table`/`.row` | existing app list/table paint | app modules | app-specific, Phase 4 |

## Current native renderer gaps

- No native Inter font; current text remains bitmap-based.
- No runtime SVG parser or icon asset pipeline.
- No GPU compositing or true backdrop blur.
- Alpha blending and rounded rectangles are available but software-rendered.
- Static precomputed/background bands are preferred over animation.
- The current framebuffer target is usually 1024×768; 1440×900 selection is
  not currently proven by the boot/framebuffer path.

## Required structural reconstruction

The old full-width bottom taskbar must not remain as the visual shell model.
The reconstruction must add:

1. top system bar;
2. left navigation rail;
3. 54px titlebar and 28px-radius main/window frames;
4. centered floating bottom dock;
5. launcher panel using the HTML app-grid geometry;
6. 320px notification toasts.

At smaller resolutions, canonical positions should be scaled or clamped as a
single layout policy rather than independently preserving legacy coordinates.
