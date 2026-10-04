/* NexOS — kernel/gui/browser_app.c | NetSurf Browser shell | MIT License */
#include "browser_app.h"
#include "wm.h"
#include "../../ports/netsurf/compat/nexos_frontend.h"
#include "../../ports/netsurf/src/netsurf/include/netsurf/keypress.h"
#include "../../ports/netsurf/src/netsurf/desktop/browser_history.h"
#include "../drivers/fb.h"
#include "../drivers/font.h"
#include "../mm/heap.h"
#include "../kernel.h"
#include <stdint.h>
#include <stddef.h>

void *kmalloc(size_t sz);
void  kfree(void *p);

#define TOOLBAR_H    38
#define STATUSBAR_H  20
#define BUTTON_W     28
#define URL_X        102

static int blen(const char *s)
{
    int n = 0;
    while (s != NULL && s[n]) n++;
    return n;
}

static void bcpy(char *d, const char *s, int max)
{
    int i = 0;
    if (max <= 0) return;
    while (i < max - 1 && s != NULL && s[i]) { d[i] = s[i]; i++; }
    d[i] = 0;
}

static int bstarts(const char *s, const char *prefix)
{
    int i = 0;
    while (prefix[i]) {
        if (s[i] != prefix[i]) return 0;
        i++;
    }
    return 1;
}

static void browser_set_error(browser_app_t *b, const char *message)
{
    bcpy(b->status, message, (int)sizeof(b->status));
    b->state = BSTATE_ERROR;
    wm_invalidate(b->win);
}

void browser_netsurf_set_status(window_t *win, const char *status)
{
    browser_app_t *b = win == NULL ? NULL : (browser_app_t *)win->userdata;
    if (b == NULL || status == NULL) return;
    bcpy(b->status, status, (int)sizeof(b->status));
    wm_invalidate(win);
}

void browser_netsurf_set_url(window_t *win, const char *url)
{
    browser_app_t *b = win == NULL ? NULL : (browser_app_t *)win->userdata;
    if (b == NULL || url == NULL || b->address_focus) return;
    bcpy(b->url, url, BROWSER_URL_MAX);
    b->url_len = blen(b->url);
    wm_invalidate(win);
}

static void browser_normalize_url(browser_app_t *b)
{
    char normalized[BROWSER_URL_MAX];
    int n;

    if (bstarts(b->url, "about:")) return;
    if (bstarts(b->url, "http://") || bstarts(b->url, "https://")) return;

    bcpy(normalized, "https://", BROWSER_URL_MAX);
    n = blen(normalized);
    for (int i = 0; i < BROWSER_URL_MAX - n - 1 && b->url[i]; i++)
        normalized[n + i] = b->url[i];
    normalized[BROWSER_URL_MAX - 1] = 0;
    bcpy(b->url, normalized, BROWSER_URL_MAX);
    b->url_len = blen(b->url);
}

static void browser_navigate(browser_app_t *b)
{
    nserror error;

    if (b->netsurf == NULL || b->url_len == 0) return;
    browser_normalize_url(b);
    b->state = BSTATE_LOADING;
    bcpy(b->status, "Loading...", (int)sizeof(b->status));
    wm_invalidate(b->win);

    error = nexos_netsurf_navigate(b->netsurf, b->url);
    if (error != NSERROR_OK) {
        browser_set_error(b, "Navigation failed");
        klog(LOG_WARN, "Browser: NetSurf navigation error=%d", (int)error);
    }
}

static void browser_paint_content(browser_app_t *b, int x, int y, int w, int h)
{
    fb_fill_rect(x, y, w, h, COL_BASE);
    if (b->netsurf != NULL) {
        if (b->state == BSTATE_LOADING && browser_window_has_content(b->netsurf)) {
            b->state = BSTATE_DONE;
            bcpy(b->status, "Ready", (int)sizeof(b->status));
        }
        nexos_netsurf_paint(b->win);
    } else if (b->state == BSTATE_ERROR) {
        fb_fill_rounded_rect(x + 16, y + 16, w - 32, 56, 8, COL_SURFACE0);
        fb_fill_rect(x + 16, y + 16, 4, 56, COL_RED);
        font_puts(x + 28, y + 24, "Browser Error", COL_RED, COL_SURFACE0);
        font_puts(x + 28, y + 42, b->status, COL_SUBTEXT, COL_SURFACE0);
    }
}

static void browser_paint(window_t *win)
{
    browser_app_t *b = win == NULL ? NULL : win->userdata;
    int wx, wy, ww, client_h, sb_y, cy, ch;
    uint32_t url_col;
    const char *url_disp;
    int max_url_chars, start, cur_x;

    if (b == NULL) return;
    wx = win->x;
    wy = win->y + WM_TITLEBAR_H;
    ww = win->w;
    client_h = win->h - WM_TITLEBAR_H;

    fb_fill_rect(wx, wy, ww, TOOLBAR_H, COL_SURFACE0);
    fb_fill_rect_blend(wx, wy, ww, 1, 0xFFFFFF, 14);

    fb_fill_rounded_rect(wx + 6, wy + 5, BUTTON_W, BUTTON_W, 6, COL_SURFACE1);
    fb_fill_rounded_rect(wx + 38, wy + 5, BUTTON_W, BUTTON_W, 6, COL_SURFACE1);
    fb_fill_rounded_rect(wx + 70, wy + 5, BUTTON_W, BUTTON_W, 6, COL_SURFACE1);
    font_puts(wx + 13, wy + 11, "<", COL_SUBTEXT, COL_SURFACE1);
    font_puts(wx + 45, wy + 11, ">", COL_SUBTEXT, COL_SURFACE1);
    font_puts(wx + 75, wy + 11, "R", COL_SUBTEXT, COL_SURFACE1);

    url_col = b->address_focus ? COL_TEXT : COL_SUBTEXT;
    url_disp = b->url_len > 0 ? b->url : "Enter URL";
    int ub_x = wx + URL_X;
    int ub_w = ww - URL_X - 8;
    uint32_t ub_bg = b->state == BSTATE_ERROR ? 0x2A1520 : COL_BASE;
    fb_fill_rounded_rect(ub_x, wy + 5, ub_w, 28, 6, ub_bg);
    fb_draw_rect_outline(ub_x, wy + 5, ub_w, 28,
                         b->address_focus ? COL_BLUE : COL_SURFACE2, 1);
    max_url_chars = (ub_w - 16) / 8;
    start = b->url_len > max_url_chars ? b->url_len - max_url_chars : 0;
    font_puts(ub_x + 10, wy + 11, url_disp + start, url_col, ub_bg);
    if (b->address_focus && b->url_len < max_url_chars) {
        cur_x = ub_x + 10 + (b->url_len - start) * 8;
        fb_fill_rect(cur_x, wy + 9, 1, 18, COL_BLUE);
    }
    fb_fill_rect(wx, wy + TOOLBAR_H - 1, ww, 1, COL_SURFACE1);

    sb_y = wy + client_h - STATUSBAR_H;
    fb_fill_rect(wx, sb_y, ww, STATUSBAR_H, COL_SURFACE0);
    fb_fill_rect(wx, sb_y, ww, 1, COL_SURFACE1);
    uint32_t pill = b->state == BSTATE_ERROR ? COL_RED :
                    b->state == BSTATE_LOADING ? COL_YELLOW : COL_GREEN;
    fb_fill_rounded_rect(wx + 6, sb_y + 3, 8, 14, 4, pill);
    font_puts(wx + 18, sb_y + 4, b->status, COL_SUBTEXT, COL_SURFACE0);

    cy = wy + TOOLBAR_H;
    ch = sb_y - cy;
    if (ch > 0) browser_paint_content(b, wx, cy, ww, ch);
}

static void browser_key(window_t *win, char key)
{
    browser_app_t *b = win == NULL ? NULL : win->userdata;
    unsigned char code = (unsigned char)key;
    if (b == NULL) return;

    if (b->address_focus) {
        if (key == '\n' || key == '\r') {
            browser_navigate(b);
        } else if (key == 27) {
            b->url_len = 0; b->url[0] = 0;
        } else if (key == '\b' || code == 0x88 || key == 127) {
            if (b->url_len > 0) b->url[--b->url_len] = 0;
        } else if (key >= 32 && key < 127 && b->url_len < BROWSER_URL_MAX - 1) {
            b->url[b->url_len++] = key;
            b->url[b->url_len] = 0;
        }
    } else if (b->netsurf != NULL) {
        uint32_t nskey = code;
        if (code == 0x80) nskey = NS_KEY_UP;
        else if (code == 0x81) nskey = NS_KEY_DOWN;
        else if (code == 0x82) nskey = NS_KEY_LEFT;
        else if (code == 0x83) nskey = NS_KEY_RIGHT;
        else if (code == 0x84) nskey = NS_KEY_TEXT_START;
        else if (code == 0x85) nskey = NS_KEY_TEXT_END;
        else if (code == 0x86) nskey = NS_KEY_PAGE_UP;
        else if (code == 0x87) nskey = NS_KEY_PAGE_DOWN;
        if (!browser_window_key_press(b->netsurf, nskey)) {
            if (code == 0x80) (void)nexos_netsurf_scroll(win, 0, -64);
            else if (code == 0x81) (void)nexos_netsurf_scroll(win, 0, 64);
            else if (code == 0x86) (void)nexos_netsurf_scroll(win, 0, -320);
            else if (code == 0x87) (void)nexos_netsurf_scroll(win, 0, 320);
            else if (code == 0x84) (void)nexos_netsurf_scroll(win, 0, -1000000);
            else if (code == 0x85) (void)nexos_netsurf_scroll(win, 0, 1000000);
        }
    }
    wm_invalidate(win);
}

static void browser_mouse_move(window_t *win, int cx, int cy)
{
    browser_app_t *b = win == NULL ? NULL : win->userdata;
    if (b == NULL || b->netsurf == NULL) return;
    if (cy >= TOOLBAR_H && cy < win->h - WM_TITLEBAR_H - STATUSBAR_H) {
        nexos_netsurf_mouse_track(win, cx, cy - TOOLBAR_H);
        static int diag_budget = 8;
        if (diag_budget > 0) {
            klog(LOG_DEBUG, "INPUT BROWSER move x=%d y=%d content_y=%d",
                 cx, cy, cy - TOOLBAR_H);
            diag_budget--;
        }
    }
}

static void browser_mouse_wheel(window_t *win, int cx, int cy, int delta)
{
    browser_app_t *b = win == NULL ? NULL : win->userdata;
    if (b == NULL || b->netsurf == NULL) return;
    if (cy >= TOOLBAR_H && cy < win->h - WM_TITLEBAR_H - STATUSBAR_H) {
        (void)nexos_netsurf_scroll(win, 0, -delta * 80);
        static int diag_budget = 8;
        if (diag_budget > 0) {
            klog(LOG_DEBUG, "INPUT BROWSER wheel x=%d y=%d delta=%d",
                 cx, cy, delta);
            diag_budget--;
        }
    }
    (void)cx;
}

static void browser_resize(window_t *win)
{
    int content_h;
    if (win == NULL) return;
    content_h = win->h - WM_TITLEBAR_H - TOOLBAR_H - STATUSBAR_H;
    if (content_h < 1) content_h = 1;
    nexos_netsurf_set_viewport(win, 0, TOOLBAR_H, win->w, content_h);
    wm_invalidate(win);
}

static void browser_click(window_t *win, int cx, int cy, int btn)
{
    browser_app_t *b = win == NULL ? NULL : win->userdata;
    if (b == NULL || btn != 1) return;
    {
        static int diag_budget = 16;
        if (diag_budget > 0) {
            klog(LOG_DEBUG, "INPUT BROWSER click x=%d y=%d button=%d",
                 cx, cy, btn);
            diag_budget--;
        }
    }

    if (cy >= 5 && cy < 33 && cx >= 6 && cx < 34) {
        if (b->netsurf != NULL && browser_window_history_back_available(b->netsurf))
            (void)browser_window_history_back(b->netsurf, false);
        return;
    }
    if (cy >= 5 && cy < 33 && cx >= 38 && cx < 66) {
        if (b->netsurf != NULL && browser_window_history_forward_available(b->netsurf))
            (void)browser_window_history_forward(b->netsurf, false);
        return;
    }
    if (cy >= 5 && cy < 33 && cx >= 70 && cx < 98) {
        if (b->netsurf != NULL) (void)browser_window_reload(b->netsurf, false);
        return;
    }
    if (cy >= 5 && cy < 33 && cx >= URL_X) {
        b->address_focus = 1;
        wm_invalidate(win);
        return;
    }
    if (cy >= TOOLBAR_H && cy < win->h - WM_TITLEBAR_H - STATUSBAR_H) {
        b->address_focus = 0;
        if (b->netsurf != NULL)
            browser_window_mouse_click(b->netsurf, BROWSER_MOUSE_CLICK_1,
                                       cx, cy - TOOLBAR_H);
        wm_invalidate(win);
    }
}

static void browser_close(window_t *win)
{
    browser_app_t *b = win == NULL ? NULL : win->userdata;
    if (b != NULL) {
        if (b->netsurf != NULL) {
            browser_window_stop(b->netsurf);
            browser_window_destroy(b->netsurf);
            b->netsurf = NULL;
        }
        nexos_netsurf_detach_window(win);
        kfree(b);
        win->userdata = NULL;
    }
    wm_close(win);
}

browser_app_t *browser_create(int x, int y)
{
    window_t *win = wm_new(x, y, 720, 520, "NexOS Browser");
    browser_app_t *b;
    nserror error;
    int content_h;

    if (win == NULL) return NULL;
    b = kmalloc(sizeof(*b));
    if (b == NULL) { wm_close(win); return NULL; }
    for (int i = 0; i < (int)sizeof(*b); i++) ((uint8_t *)b)[i] = 0;
    b->win = win;
    b->state = BSTATE_IDLE;
    b->address_focus = 1;
    bcpy(b->status, "Ready", (int)sizeof(b->status));
    win->userdata = b;
    win->on_paint = browser_paint;
    win->on_key = browser_key;
    win->on_click = browser_click;
    win->on_mouse_move = browser_mouse_move;
    win->on_mouse_wheel = browser_mouse_wheel;
    win->on_resize = browser_resize;
    win->on_close = browser_close;

    content_h = win->h - WM_TITLEBAR_H - TOOLBAR_H - STATUSBAR_H;
    nexos_netsurf_bind_window(win, 0, TOOLBAR_H, win->w, content_h);
    error = nexos_netsurf_open_blank(&b->netsurf);
    if (error != NSERROR_OK || b->netsurf == NULL) {
        browser_set_error(b, "NetSurf initialization failed");
        klog(LOG_ERROR, "Browser: NetSurf open error=%d", (int)error);
    }
    return b;
}
