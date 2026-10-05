/* NexOS — kernel/gui/launcher.c | Liquid-glass app launcher with animations
 *
 * Open:  panel slides up from below + scrim fades in  (~350 ms, ease-out-cubic)
 * Close: panel slides back down + scrim fades out      (~220 ms, ease-in-quad)
 * MIT License */
#include "launcher.h"
#include "anim.h"
#include "aurora.h"
#include "wm.h"
#include "../drivers/fb.h"
#include "../drivers/font.h"
#include "../kernel.h"
#include <stdint.h>
#include <stddef.h>

/* ── App launchers (defined elsewhere) ──────────────────────────────────── */
void launch_terminal(void);
void launch_filemanager(void);
void launch_sysinfo(void);
void launch_themewin(void);
void launch_browser(void);
void launch_calc(void);
void launch_clock(void);
void launch_editor(void);
void launch_visualizer(void);
void launch_snake(void);
void launch_sysmon(void);
void launch_settings(void);

static void action_restart(void) {
    __asm__ volatile("mov $0xFE, %%al\n out %%al, $0x64\n" ::: "eax");
}
static void action_shutdown(void) {
    __asm__ volatile("mov $0x2000, %%ax\n mov $0x604, %%dx\n outw %%ax, %%dx\n"
                     ::: "eax", "edx");
}

/* ── Layout ──────────────────────────────────────────────────────────────── */
#define PANEL_W     520
#define PANEL_H     620
#define PANEL_R      26
#define CARD_COLS     4
#define CARD_W      104
#define CARD_H       96
#define CARD_R       18
#define CARD_PAD     14
#define GRID_TOP    100
#define ICON_R       24

/* ── App table ───────────────────────────────────────────────────────────── */
typedef struct {
    const char *label;
    aurora_icon_id_t icon;
    uint32_t    icon_color;
    void      (*action)(void);
} app_item_t;

static const app_item_t apps[] = {
    { "Terminal",   AURORA_ICON_TERMINAL, 0xA6E3A1, launch_terminal    },
    { "Files",      AURORA_ICON_FILES, 0x89B4FA, launch_filemanager },
    { "System",     AURORA_ICON_SYSTEM, 0xCBA6F7, launch_sysinfo     },
    { "Theme",      AURORA_ICON_THEME, 0xFAB387, launch_themewin    },
    { "Browser",    AURORA_ICON_BROWSER, 0x74C7EC, launch_browser     },
    { "Calc",       AURORA_ICON_CALCULATOR, 0xF9E2AF, launch_calc        },
    { "Clock",      AURORA_ICON_CLOCK, 0x94E2D5, launch_clock       },
    { "Editor",     AURORA_ICON_EDITOR, 0x89DCEB, launch_editor      },
    { "Visualizer", AURORA_ICON_VISUALIZER, 0xF38BA8, launch_visualizer  },
    { "Snake",      AURORA_ICON_SNAKE, 0xA6E3A1, launch_snake       },
    { "Monitor",    AURORA_ICON_MONITOR, 0xCBA6F7, launch_sysmon      },
    { "Settings",   AURORA_ICON_SETTINGS, 0x89B4FA, launch_settings    },
};
#define APP_COUNT  ((int)(sizeof(apps)/sizeof(apps[0])))

/* ── State ───────────────────────────────────────────────────────────────── */
static int launcher_visible = 0;  /* 1 after show(), before real hide */
static int launcher_closing = 0;  /* 1 while close animation runs */
static int laun_anim        = 0;  /* 0-256 animation progress */
static int panel_x, panel_y;
static int launcher_hover = -1;
static int hover_restart  = 0;
static int hover_shutdown = 0;

/* Per-card hover glow (0-256 smooth transition) */
static int card_glow[APP_COUNT > 0 ? APP_COUNT : 1];

/* ── Helpers ─────────────────────────────────────────────────────────────── */
static void draw_centered(int rx, int rw, int y,
                           const char *s, uint32_t fg, uint32_t bg) {
    int tx = rx + (rw - font_aurora_str_width(s, 14)) / 2;
    font_aurora_puts(tx, y, s, 14, fg, bg);
}

static void draw_card(int cx, int cy, int idx, int hovered) {
    int cx0 = cx - CARD_W / 2;
    uint32_t state = hovered ? AURORA_STATE_HOVER : AURORA_STATE_NORMAL;
    uint32_t card_bg = aurora_state_color(AURORA_COLOR_SURFACE,
                                           AURORA_COLOR_HOVER,
                                           AURORA_COLOR_PRESSED, state);
    fb_fill_rounded_rect(cx0, cy, CARD_W, CARD_H, CARD_R, card_bg);
    fb_draw_rect_outline(cx0, cy, CARD_W, CARD_H,
                         (state & AURORA_STATE_HOVER) ?
                         aurora_color(AURORA_COLOR_BORDER_FOCUSED) :
                         aurora_color(AURORA_COLOR_BORDER_SUBTLE), 1);

    /* Icon */
    int icon_cy = cy + ICON_R + 14;
    aurora_app_icon_id(cx, icon_cy, ICON_R, apps[idx].icon,
                       apps[idx].icon_color);

    draw_centered(cx0, CARD_W, cy + CARD_H - 24,
                  apps[idx].label,
                  aurora_color(AURORA_COLOR_TEXT_PRIMARY),
                  aurora_color(hovered ? AURORA_COLOR_HOVER :
                               AURORA_COLOR_SURFACE));
}

/* ── Animation tick ──────────────────────────────────────────────────────── */
void launcher_tick(uint32_t delta_ms) {
    /* Update card hover glow */
    for (int i = 0; i < APP_COUNT; i++) {
        int target = (launcher_hover == i) ? 256 : 0;
        int step   = (int)(delta_ms * 256 / 120);   /* ~120 ms full transition */
        if (card_glow[i] < target)
            card_glow[i] = anim_clamp(card_glow[i] + step, 0, target);
        else if (card_glow[i] > target)
            card_glow[i] = anim_clamp(card_glow[i] - step, target, 256);
    }

    if (!launcher_visible && !launcher_closing) return;

    if (launcher_closing) {
        /* Close: ease-in-quad — starts slow, then snaps away */
        int step = (int)(delta_ms * 256 / 200);  /* ~200 ms close */
        laun_anim -= step;
        if (laun_anim <= 0) {
            laun_anim       = 0;
            launcher_closing = 0;
            launcher_visible = 0;
            fb_scene_dirty   = 1;
        }
    } else {
        /* Open: ease-out-cubic — shoots up then decelerates */
        int step = (int)(delta_ms * 256 / 350);  /* ~350 ms open */
        laun_anim = anim_clamp(laun_anim + step, 0, 256);
    }
}

/* ── Public API ──────────────────────────────────────────────────────────── */
void launcher_show(int x, int y) {
    (void)x; (void)y;
    klog(LOG_DEBUG, "APPS ACTION toggle requested visible=%d closing=%d",
         launcher_visible, launcher_closing);
    panel_x = ((int)fb.width  - PANEL_W) / 2;
    panel_y = ((int)fb.height - PANEL_H) / 2;
    if (panel_x < AURORA_SPACE_2) panel_x = AURORA_SPACE_2;
    if (panel_x + PANEL_W > (int)fb.width - AURORA_SPACE_2)
        panel_x = (int)fb.width - PANEL_W - AURORA_SPACE_2;
    if (panel_y < 4) panel_y = 4;
    launcher_visible = 1;
    launcher_closing = 0;
    laun_anim        = 0;   /* start animation from 0 */
    launcher_hover   = -1;
    hover_restart    = 0;
    hover_shutdown   = 0;
    for (int i = 0; i < APP_COUNT; i++) card_glow[i] = 0;
    fb_scene_dirty   = 1;
    klog(LOG_DEBUG, "APPS STATE visible=1 closing=0");
}

void launcher_hide(void) {
    if (launcher_visible && !launcher_closing) {
        launcher_closing = 1;  /* start close animation — tick will finish */
    }
}

int launcher_is_visible(void) {
    return launcher_visible || launcher_closing;
}

void launcher_draw(void) {
    if (!launcher_visible && !launcher_closing) return;

    static int paint_diag_budget = 2;
    if (paint_diag_budget > 0) {
        klog(LOG_DEBUG, "APPS PAINT visible=%d closing=%d anim=%d",
             launcher_visible, launcher_closing, laun_anim);
        paint_diag_budget--;
    }

    /* ── Animation values ── */
    int   p_open  = anim_ease_out_cubic(laun_anim);
    int   p_close = anim_ease_in_quad(laun_anim);
    int   p       = launcher_closing ? p_close : p_open;

    /* Scrim alpha: 0 → 190 on open, 190 → 0 on close */
    uint8_t scrim_a = (uint8_t)(190 * p / 256);

    /* Panel vertical offset: slides up from below */
    int y_lift = (PANEL_H / 3) * (256 - p) / 256;
    int py     = panel_y + y_lift;

    /* Panel contents alpha */
    uint8_t panel_a = (uint8_t)(215 * p / 256);
    if (panel_a < 4) return;

    /* ── Step 1: scrim ── */
    int desktop_h = (int)fb.height;
    if (scrim_a > 2)
        fb_fill_rect_blend(0, 0, (int)fb.width, desktop_h,
                           aurora_color(AURORA_COLOR_BACKGROUND), scrim_a);

    /* ── Step 2: frosted panel ── */
    if (panel_a > 8) {
        (void)panel_a;
        fb_fill_rounded_rect(panel_x, py, PANEL_W, PANEL_H, PANEL_R,
                             aurora_color(AURORA_COLOR_ELEVATED));
        fb_draw_rect_outline(panel_x, py, PANEL_W, PANEL_H,
                             aurora_color(AURORA_COLOR_BORDER_SUBTLE), 1);
    }

    /* ── Step 3: rim + specular ── */
    fb_draw_rect_outline(panel_x, py, PANEL_W, PANEL_H,
                         aurora_color(AURORA_COLOR_BORDER_FOCUSED), 1);
    fb_fill_rect(panel_x + PANEL_R, py + 1,
                 PANEL_W - 2 * PANEL_R, 1,
                 aurora_color(AURORA_COLOR_ACCENT));

    /* ── Step 4: title ── */
    draw_centered(panel_x, PANEL_W, py + 18,
                  "NexOS  Apps",
                  aurora_color(AURORA_COLOR_TEXT_PRIMARY),
                  aurora_color(AURORA_COLOR_ELEVATED));

    /* ── Step 5: search bar ── */
    int sb_x = panel_x + 20, sb_y = py + 46, sb_w = PANEL_W - 40, sb_h = 28;
    aurora_search_field((aurora_rect_t){sb_x, sb_y, sb_w, sb_h},
                        "Search apps...", AURORA_STATE_INACTIVE);

    /* ── Step 6: app grid ── */
    int total_w = CARD_COLS * CARD_W + (CARD_COLS - 1) * CARD_PAD;
    int grid_x0 = panel_x + (PANEL_W - total_w) / 2;
    int grid_y  = py + GRID_TOP;

    for (int i = 0; i < APP_COUNT; i++) {
        int row     = i / CARD_COLS, col = i % CARD_COLS;
        int card_cx = grid_x0 + col * (CARD_W + CARD_PAD) + CARD_W / 2;
        int card_cy = grid_y  + row * (CARD_H + CARD_PAD);
        draw_card(card_cx, card_cy, i, launcher_hover == i);
    }

    /* ── Step 7: separator ── */
    int rows  = (APP_COUNT + CARD_COLS - 1) / CARD_COLS;
    int sep_y = py + GRID_TOP + rows * (CARD_H + CARD_PAD) - CARD_PAD + 16;
    aurora_separator(panel_x + 20, sep_y, PANEL_W - 40);

    /* ── Step 8: power buttons ── */
    int pbtn_y  = sep_y + AURORA_SPACE_3;
    int pbtn_w  = 150, pbtn_h = AURORA_CONTROL_H;
    int pbtn_gx = panel_x + (PANEL_W / 2) - pbtn_w - 8;
    int pbs_x   = panel_x + (PANEL_W / 2) + 8;

    aurora_button((aurora_rect_t){pbtn_gx, pbtn_y, pbtn_w, pbtn_h},
                  "Restart", AURORA_BUTTON_SECONDARY,
                  hover_restart ? AURORA_STATE_HOVER : AURORA_STATE_NORMAL);
    aurora_button((aurora_rect_t){pbs_x, pbtn_y, pbtn_w, pbtn_h},
                  "Shutdown", AURORA_BUTTON_DANGER,
                  hover_shutdown ? AURORA_STATE_HOVER : AURORA_STATE_NORMAL);
}

void launcher_handle_click(int x, int y) {
    if (!launcher_visible) return;
    int py = panel_y;   /* use resting position for hit-testing */

    if (x < panel_x || x >= panel_x + PANEL_W ||
        y < py      || y >= py      + PANEL_H) {
        launcher_hide(); return;
    }

    int total_w = CARD_COLS * CARD_W + (CARD_COLS - 1) * CARD_PAD;
    int grid_x0 = panel_x + (PANEL_W - total_w) / 2;
    int grid_y  = py + GRID_TOP;

    for (int i = 0; i < APP_COUNT; i++) {
        int row = i / CARD_COLS, col = i % CARD_COLS;
        int cx0 = grid_x0 + col * (CARD_W + CARD_PAD);
        int cy0 = grid_y  + row * (CARD_H + CARD_PAD);
        if (x >= cx0 && x < cx0 + CARD_W && y >= cy0 && y < cy0 + CARD_H) {
            if (apps[i].action) apps[i].action();
            launcher_hide(); return;
        }
    }

    int rows_c = (APP_COUNT + CARD_COLS - 1) / CARD_COLS;
    int sep_y  = py + GRID_TOP + rows_c * (CARD_H + CARD_PAD) - CARD_PAD + 16;
    int pby    = sep_y + 12, pbw = 150, pbh = 36;
    int pbtn_gx = panel_x + (PANEL_W / 2) - pbw - 8;
    int pbs_x   = panel_x + (PANEL_W / 2) + 8;
    if (y >= pby && y < pby + pbh) {
        if (x >= pbtn_gx && x < pbtn_gx + pbw) { action_restart();  return; }
        if (x >= pbs_x   && x < pbs_x   + pbw) { action_shutdown(); return; }
    }
    launcher_hide();
}

void launcher_handle_mouse(int x, int y) {
    if (!launcher_visible) return;
    int prev_h   = launcher_hover;
    int prev_r   = hover_restart;
    int prev_s   = hover_shutdown;

    launcher_hover = -1;
    hover_restart  = 0;
    hover_shutdown = 0;

    int total_w = CARD_COLS * CARD_W + (CARD_COLS - 1) * CARD_PAD;
    int grid_x0 = panel_x + (PANEL_W - total_w) / 2;
    int grid_y  = panel_y + GRID_TOP;

    for (int i = 0; i < APP_COUNT; i++) {
        int row = i / CARD_COLS, col = i % CARD_COLS;
        int cx0 = grid_x0 + col * (CARD_W + CARD_PAD);
        int cy0 = grid_y  + row * (CARD_H + CARD_PAD);
        if (x >= cx0 && x < cx0 + CARD_W && y >= cy0 && y < cy0 + CARD_H)
            { launcher_hover = i; break; }
    }

    int rows_m = (APP_COUNT + CARD_COLS - 1) / CARD_COLS;
    int sep_y  = panel_y + GRID_TOP + rows_m * (CARD_H + CARD_PAD) - CARD_PAD + 16;
    int pby    = sep_y + 12, pbw = 150, pbh = 36;
    int pbtn_gx = panel_x + (PANEL_W / 2) - pbw - 8;
    int pbs_x   = panel_x + (PANEL_W / 2) + 8;
    if (y >= pby && y < pby + pbh) {
        if (x >= pbtn_gx && x < pbtn_gx + pbw) hover_restart  = 1;
        if (x >= pbs_x   && x < pbs_x   + pbw) hover_shutdown = 1;
    }

    if (launcher_hover != prev_h || hover_restart != prev_r ||
        hover_shutdown != prev_s)
        fb_scene_dirty = 1;
}

void launcher_handle_key(char key) {
    if (key == 27) { launcher_hide(); return; }
}
