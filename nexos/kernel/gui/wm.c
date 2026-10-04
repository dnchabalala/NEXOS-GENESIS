/* NexOS — kernel/gui/wm.c | Window Manager | MIT License */
#include "wm.h"
#include "desktop.h"
#include "../drivers/fb.h"
#include "../drivers/font.h"
#include "../mm/heap.h"
#include "../kernel.h"
#include <stdint.h>
#include <stddef.h>

/* provided by heap.h */
void *kmalloc(size_t size);
void  kfree(void *ptr);

/* ── Window list (front = index 0) ──────────────────────────────────────── */
static window_t *wins[WM_MAX_WINDOWS];
static int       win_count = 0;
static int       next_id   = 1;
static window_t *focused_win = NULL;
static int       wm_left_down = 0;

/* ── Internal helpers ────────────────────────────────────────────────────── */
static void strncpy_s(char *d, const char *s, int n) {
    int i = 0;
    while (i < n - 1 && s[i]) { d[i] = s[i]; i++; }
    d[i] = 0;
}

static int point_in_rect(int px, int py, int rx, int ry, int rw, int rh) {
    return px >= rx && px < rx + rw && py >= ry && py < ry + rh;
}
static int point_in_circle(int px, int py, int cx, int cy, int r) {
    int dx = px - cx, dy = py - cy;
    return dx * dx + dy * dy <= r * r;
}

static void wm_draw_window(window_t *win) {
    if (!win->visible || win->state == WIN_MINIMIZED) return;

    /* Every window is composed in a bounded paint phase.  This prevents an
     * application or NetSurf plotter from touching the titlebar, taskbar, or
     * a neighbouring window while the WM is rebuilding the scene. */
    fb_reset_clip();
    fb_set_clip(win->x - WM_SHADOW_OFF - 4, win->y - WM_SHADOW_OFF - 4,
                win->w + WM_SHADOW_OFF * 2 + 8,
                win->h + WM_SHADOW_OFF * 2 + 8);

    /* ── 3-layer graduated drop shadow ─────────────────────────────────── */
    fb_fill_rect_blend(win->x + 10, win->y + 10, win->w, win->h, 0x000000, 42);
    fb_fill_rect_blend(win->x +  6, win->y +  6, win->w, win->h, 0x000000, 24);
    fb_fill_rect_blend(win->x +  2, win->y +  2, win->w, win->h, 0x000000, 10);

    /* ── Focus outer glow ring ──────────────────────────────────────────── */
    if (win->focused) {
        fb_fill_rect_blend(win->x - 3, win->y - 3,
                           win->w + 6, win->h + 6, COL_BLUE, 18);
        fb_fill_rect_blend(win->x - 1, win->y - 1,
                           win->w + 2, win->h + 2, COL_BLUE, 36);
    }

    /* ── Window frame ───────────────────────────────────────────────────── */
    uint32_t frame_c = win->focused ? COL_SURFACE2 : COL_SURFACE1;
    fb_fill_rounded_rect(win->x - 1, win->y - 1,
                         win->w + 2, win->h + 2, 12, frame_c);

    /* ── Titlebar base ──────────────────────────────────────────────────── */
    uint32_t tb_col = win->focused ? 0x24253E : 0x1C1D30;
    fb_fill_rounded_rect(win->x, win->y, win->w, WM_TITLEBAR_H, 10, tb_col);

    /* Specular — bright overlay on upper half + single top-rim pixel */
    fb_fill_rect_blend(win->x + 8,  win->y,
                       win->w - 16, WM_TITLEBAR_H / 2 + 2, 0xFFFFFF, 8);
    fb_fill_rect_blend(win->x + 10, win->y, win->w - 20, 1, 0xFFFFFF, 34);

    /* ── Focus left accent stripe + soft glow ───────────────────────────── */
    if (win->focused) {
        fb_fill_rect_blend(win->x, win->y + 8,
                           5, WM_TITLEBAR_H - 16, COL_BLUE, 28);
        fb_fill_rect(win->x, win->y + 8, 3, WM_TITLEBAR_H - 16, COL_BLUE);
    }

    /* ── Traffic-light buttons with shadow + specular ───────────────────── */
    int bx = win->x + win->w - WM_BTN_GAP;
    int by = win->y + WM_TITLEBAR_H / 2;

    /* per-button drop shadow */
    fb_fill_circle(bx + 1,               by + 1, WM_BTN_R, 0x000000);
    fb_fill_circle(bx - WM_BTN_GAP + 1,  by + 1, WM_BTN_R, 0x000000);
    fb_fill_circle(bx - WM_BTN_GAP*2+1,  by + 1, WM_BTN_R, 0x000000);

    fb_fill_circle(bx,                   by, WM_BTN_R, COL_RED);
    fb_fill_circle(bx - WM_BTN_GAP,      by, WM_BTN_R, COL_YELLOW);
    fb_fill_circle(bx - WM_BTN_GAP * 2,  by, WM_BTN_R, COL_GREEN);

    /* specular highlight: small bright dot top-left of each button */
    fb_fill_circle(bx - 2,                   by - 2, 2,
                   fb_blend(0xFFFFFF, COL_RED,    160));
    fb_fill_circle(bx - WM_BTN_GAP - 2,      by - 2, 2,
                   fb_blend(0xFFFFFF, COL_YELLOW, 160));
    fb_fill_circle(bx - WM_BTN_GAP * 2 - 2,  by - 2, 2,
                   fb_blend(0xFFFFFF, COL_GREEN,  160));

    /* ── Title centered in titlebar ─────────────────────────────────────── */
    int tw = font_str_width(win->title);
    int tx = win->x + (win->w - tw * 8) / 2;
    int ty = win->y + (WM_TITLEBAR_H - 16) / 2;
    font_puts(tx, ty, win->title, COL_TEXT, tb_col);

    /* ── Client area ────────────────────────────────────────────────────── */
    fb_set_clip(win->x, win->y + WM_TITLEBAR_H,
                win->w, win->h - WM_TITLEBAR_H);
    fb_fill_rect(win->x, win->y + WM_TITLEBAR_H,
                 win->w, win->h - WM_TITLEBAR_H, COL_MANTLE);
    /* Hairline separator between titlebar and content */
    fb_fill_rect(win->x, win->y + WM_TITLEBAR_H, win->w, 1, COL_SURFACE1);

    if (win->on_paint) win->on_paint(win);
    fb_reset_clip();
}

/* ── Public API ──────────────────────────────────────────────────────────── */
void wm_init(void) {
    win_count = 0; next_id = 1; focused_win = NULL; wm_left_down = 0;
    for (int i = 0; i < WM_MAX_WINDOWS; i++) wins[i] = NULL;
    klog(LOG_INFO, "WM: initialized");
}

void wm_debug_report(void) {
    int duplicate_ptrs = 0;
    int duplicate_ids = 0;

    for (int i = 0; i < win_count; i++) {
        if (!wins[i]) continue;
        for (int j = i + 1; j < win_count; j++) {
            if (wins[i] == wins[j]) duplicate_ptrs++;
            if (wins[i]->id == wins[j]->id) duplicate_ids++;
        }
    }

    klog(LOG_INFO, "WM STARTUP windows=%d duplicate_ptrs=%d duplicate_ids=%d",
         win_count, duplicate_ptrs, duplicate_ids);
    for (int i = 0; i < win_count; i++) {
        window_t *win = wins[i];
        if (!win) {
            klog(LOG_WARN, "WM WINDOW index=%d null", i);
            continue;
        }
        klog(LOG_INFO, "WM WINDOW index=%d id=%d title=%s state=%d visible=%d",
             i, win->id, win->title, (int)win->state, (int)win->visible);
    }
}

window_t *wm_new(int x, int y, int w, int h, const char *title) {
    if (win_count >= WM_MAX_WINDOWS) return NULL;
    window_t *win = (window_t *)kmalloc(sizeof(window_t));
    if (!win) return NULL;
    /* zero-init */
    for (int i = 0; i < (int)sizeof(window_t); i++) ((uint8_t *)win)[i] = 0;
    win->x = x; win->y = y; win->w = w; win->h = h;
    win->client_w = w;
    win->client_h = h - WM_TITLEBAR_H;
    strncpy_s(win->title, title, 64);
    win->state   = WIN_NORMAL;
    win->visible = 1;
    win->focused = 0;
    win->id      = next_id++;
    /* Do not mutate live window geometry during paint.  The old pop-in
     * animation temporarily changed x/y/w/h while application callbacks
     * were executing, which made clipping and NetSurf viewport state race
     * with composition. */
    win->anim_frames = 0;
    win->orig_x = x; win->orig_y = y;
    win->orig_w = w; win->orig_h = h;
    /* insert at front (top of z-order) */
    for (int i = win_count; i > 0; i--) wins[i] = wins[i - 1];
    wins[0] = win;
    win_count++;
    wm_focus(win);
    return win;
}

void wm_close(window_t *win) {
    if (!win) return;
    for (int i = 0; i < win_count; i++) {
        if (wins[i] == win) {
            for (int j = i; j < win_count - 1; j++) wins[j] = wins[j + 1];
            wins[win_count - 1] = NULL;
            win_count--;
            if (focused_win == win) {
                focused_win = (win_count > 0) ? wins[0] : NULL;
                if (focused_win) focused_win->focused = 1;
            }
            if (win->pixels) kfree(win->pixels);
            kfree(win);
            fb_scene_dirty = 1;   /* redraw remaining windows over clean bg */
            return;
        }
    }
}

void wm_focus(window_t *win) {
    if (focused_win) focused_win->focused = 0;
    focused_win = win;
    if (win) win->focused = 1;
}

void wm_raise(window_t *win) {
    for (int i = 0; i < win_count; i++) {
        if (wins[i] == win) {
            for (int j = i; j > 0; j--) wins[j] = wins[j - 1];
            wins[0] = win;
            return;
        }
    }
}

void wm_minimize(window_t *win) {
    win->state = WIN_MINIMIZED;
    if (focused_win == win) {
        for (int i = 0; i < win_count; i++) {
            if (wins[i] != win && wins[i]->state != WIN_MINIMIZED) {
                wm_focus(wins[i]); break;
            }
        }
    }
    fb_scene_dirty = 1;   /* repaint remaining windows over clean bg */
}

void wm_toggle_maximize(window_t *win) {
    if (win->state == WIN_MAXIMIZED) {
        win->x = win->orig_x; win->y = win->orig_y;
        win->w = win->orig_w; win->h = win->orig_h;
        win->state = WIN_NORMAL;
    } else {
        win->orig_x = win->x; win->orig_y = win->y;
        win->orig_w = win->w; win->orig_h = win->h;
        win->x = 0; win->y = 0;
        win->w = (int)fb.width;
        win->h = (int)fb.height - 40;
        win->state = WIN_MAXIMIZED;
    }
    if (win->on_resize) win->on_resize(win);
    fb_scene_dirty = 1;   /* layout changed — full repaint */
}

void wm_move(window_t *win, int x, int y) {
    if (!win) return;
    win->x = x; win->y = y;
    fb_scene_dirty = 1;
}
void wm_resize(window_t *win, int w, int h) {
    win->w = w; win->h = h;
    win->client_w = w;
    win->client_h = h - WM_TITLEBAR_H;
    if (win->on_resize) win->on_resize(win);
    fb_scene_dirty = 1;
}
void wm_invalidate(window_t *win) {
    (void)win;
    /* Applications request repaint through the normal frame composition.
     * This must not dirty the desktop background: NetSurf status/content
     * updates are frequent and should not trigger an animated full-screen
     * repaint. */
}

void wm_render_all(void) {
    /* draw back-to-front */
    static int compose_reported = 0;
    int visible = 0;
    int painted = 0;
    for (int i = win_count - 1; i >= 0; i--) {
        window_t *win = wins[i];
        if (!win || !win->visible || win->state == WIN_MINIMIZED) continue;

        visible++;
        wm_draw_window(win);
        painted++;
    }
    if (!compose_reported) {
        klog(LOG_INFO, "GUI COMPOSE visible_windows=%d painted_windows=%d",
             visible, painted);
        compose_reported = 1;
    }
}

void wm_handle_mouse(int x, int y, int left, int right) {
    (void)right;
    int pressed = left && !wm_left_down;
    wm_left_down = left ? 1 : 0;

    if (focused_win && focused_win->dragging && left) {
        int nx = x - focused_win->drag_ox;
        int ny = y - focused_win->drag_oy;
        if (nx < 0) nx = 0;
        if (ny < 0) ny = 0;
        if (nx + focused_win->w > (int)fb.width)
            nx = (int)fb.width - focused_win->w;
        if (ny + focused_win->h > (int)fb.height - 40)
            ny = (int)fb.height - 40 - focused_win->h;
        if (nx != focused_win->x || ny != focused_win->y) {
            wm_move(focused_win, nx, ny);
        }
        return;
    }

    /* Movement is a separate frontend event from a button press.  Deliver
     * it to the topmost window under the pointer so browsers can perform
     * real NetSurf hover/hit testing without receiving toolbar coordinates. */
    for (int i = 0; i < win_count; i++) {
        window_t *win = wins[i];
        if (!win || !win->visible || win->state == WIN_MINIMIZED ||
            win->on_mouse_move == NULL) continue;
        if (point_in_rect(x, y, win->x, win->y + WM_TITLEBAR_H,
                          win->w, win->h - WM_TITLEBAR_H)) {
            win->on_mouse_move(win, x - win->x,
                               y - win->y - WM_TITLEBAR_H);
            static int move_diag_budget = 8;
            if (move_diag_budget > 0) {
                klog(LOG_DEBUG, "INPUT WM move x=%d y=%d window=%s",
                     x, y, win->title);
                move_diag_budget--;
            }
            break;
        }
    }

    if (!pressed) return;

    for (int i = 0; i < win_count; i++) {
        window_t *win = wins[i];
        if (!win || !win->visible || win->state == WIN_MINIMIZED) continue;

        int bx = win->x + win->w - WM_BTN_GAP;
        int by = win->y + WM_TITLEBAR_H / 2;

        /* close button */
        if (point_in_circle(x, y, bx, by, WM_BTN_R)) {
            if (win->on_close) win->on_close(win);
            else wm_close(win);
            return;
        }
        /* min button */
        if (point_in_circle(x, y, bx - WM_BTN_GAP, by, WM_BTN_R)) {
            wm_minimize(win); return;
        }
        /* max button */
        if (point_in_circle(x, y, bx - WM_BTN_GAP * 2, by, WM_BTN_R)) {
            wm_toggle_maximize(win); return;
        }
        /* title bar drag */
        if (point_in_rect(x, y, win->x, win->y, win->w, WM_TITLEBAR_H)) {
            wm_raise(win); wm_focus(win);
            win->dragging = 1;
            win->drag_ox = x - win->x;
            win->drag_oy = y - win->y;
            return;
        }
        /* client area click */
        if (point_in_rect(x, y, win->x, win->y + WM_TITLEBAR_H,
                          win->w, win->h - WM_TITLEBAR_H)) {
            wm_raise(win); wm_focus(win);
            if (win->on_click)
                win->on_click(win,
                    x - win->x,
                    y - win->y - WM_TITLEBAR_H,
                    left ? 1 : 2);
            return;
        }
    }
}

void wm_handle_mouse_release(int x, int y) {
    (void)x; (void)y;
    wm_left_down = 0;
    if (focused_win && focused_win->dragging) {
        focused_win->dragging = 0;
        fb_scene_dirty = 1;  /* one final full repaint to clean up any artifacts */
    }
}

void wm_handle_mouse_wheel(int x, int y, int delta) {
    if (delta == 0) return;
    for (int i = 0; i < win_count; i++) {
        window_t *win = wins[i];
        if (!win || !win->visible || win->state == WIN_MINIMIZED ||
            win->on_mouse_wheel == NULL) continue;
        if (point_in_rect(x, y, win->x, win->y + WM_TITLEBAR_H,
                          win->w, win->h - WM_TITLEBAR_H)) {
            win->on_mouse_wheel(win, x - win->x,
                                y - win->y - WM_TITLEBAR_H, delta);
            static int wheel_diag_budget = 8;
            if (wheel_diag_budget > 0) {
                klog(LOG_DEBUG, "INPUT WM wheel x=%d y=%d delta=%d window=%s",
                     x, y, delta, win->title);
                wheel_diag_budget--;
            }
            return;
        }
    }
}

void wm_handle_key(char key) {
    if (focused_win && focused_win->on_key)
        focused_win->on_key(focused_win, key);
}

window_t *wm_focused(void) { return focused_win; }

void wm_put_pixel(window_t *w, int x, int y, uint32_t color) {
    fb_put_pixel(w->x + x, w->y + WM_TITLEBAR_H + y, color);
}

int wm_window_count(void) { return win_count; }
window_t *wm_get_window(int idx) {
    if (idx < 0 || idx >= win_count) return NULL;
    return wins[idx];
}
