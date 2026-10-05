# NexOS Worktree Snapshot

Snapshot date: 2026-10-05

Branch: `main`

HEAD: `e78284359cefa724ac2c8213bcb0b42413b868c8`

## Tracked modified paths

```text
nexos/Makefile
nexos/README.md
nexos/boot/grub/grub.cfg
nexos/kernel/arch/x86_64/boot.asm
nexos/kernel/drivers/fb.c
nexos/kernel/drivers/fb.h
nexos/kernel/drivers/font.c
nexos/kernel/drivers/font.h
nexos/kernel/drivers/mouse.c
nexos/kernel/drivers/mouse.h
nexos/kernel/gui/browser_app.c
nexos/kernel/gui/desktop.c
nexos/kernel/gui/files_app.c
nexos/kernel/gui/gui.c
nexos/kernel/gui/launcher.c
nexos/kernel/gui/notif.c
nexos/kernel/gui/settings_app.c
nexos/kernel/gui/taskbar.c
nexos/kernel/gui/taskbar.h
nexos/kernel/gui/term_app.c
nexos/kernel/gui/theme_app.c
nexos/kernel/gui/theme_app.h
nexos/kernel/gui/wm.c
nexos/kernel/gui/wm.h
nexos/kernel/net/http.c
nexos/kernel/net/http.h
nexos/kernel/net/tcp.c
nexos/ports/netsurf/compat/nexos_frontend.c
nexos/ports/netsurf/nexos_fetcher.c
```

Known Phase 4B source files are `settings_app.c` and `browser_app.c`.
Other changes predate this freeze unless a future audit proves otherwise.

## Untracked groups

```text
docs/
nexos/assets/
nexos/docs/design/
nexos/kernel/drivers/aurora_font_atlas.h
nexos/kernel/gui/aurora.c
nexos/kernel/gui/aurora.h
nexos/kernel/gui/shell.c
nexos/kernel/gui/shell.h
nexos/tools/gen_aurora_assets.py
nexos/tools/gen_nexos_icon_assets.py
```

Generated build output also exists under `nexos/build/`. Do not clean,
stage, commit, reset or revert during this freeze.

## Diff size

The tracked diff at this documentation snapshot is 29 modified files,
1,765 insertions and 783 deletions. The 29 includes the small README status
section added by this handoff pass. Recompute with:

```sh
git status --short
git diff --stat
```

There are 136 untracked files when expanded with
`git ls-files --others --exclude-standard`; `git status --short` groups many
of them as the directory entries `docs/`, `nexos/assets/` and
`nexos/docs/design/`.
