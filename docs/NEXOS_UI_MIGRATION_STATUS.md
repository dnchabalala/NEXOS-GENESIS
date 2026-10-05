# NexOS UI Migration Status

## Source of truth

`nexos/docs/design/nexos_ui_master.html` is primary and the PDF is
secondary. `AURORA_HTML_SPEC.md` extracts geometry/tokens. The old native
GUI is not the visual baseline.

| Surface | Migration | Screenshot | Interactive | Notes |
|---|---|---|---|---|
| Desktop/top bar/rail/dock/window chrome | Complete | Historical 1440x900 captures | Partial historical verification | Shell accepted historically. |
| Launcher/notifications | Complete | Historical captures | Partial | Search remains visual-only. |
| Files | Phase 4A complete | Captured | Reported accepted | Icon fallbacks remain. |
| Terminal | Phase 4A complete | Captured | Reported accepted | Monospace client retained. |
| Settings | Phase 4B implementation complete | Pending valid capture | Pending | Wi-Fi/info/power preserved. |
| Browser | Phase 4B implementation complete | Pending valid capture | Pending | Real NetSurf protected. |
| Other apps | Not migrated | Pending | Pending | Do not start until Phase 4B acceptance. |

Inter-quality antialiasing, browser blur, complete production icon
integration, narrow-layout acceptance, and future shell surfaces remain
unresolved or deferred. First accept Settings and Browser, then migrate
System Information/Themes, Calculator/Clock, and
Editor/Visualizer/Snake/System Monitor.
