/* NexOS — kernel/gui/browser_app.c | NetSurf Browser shell | MIT License */
#include "browser_app.h"
#include "wm.h"
#include "aurora.h"
#include "../../ports/netsurf/compat/nexos_frontend.h"
#include "../../ports/netsurf/src/netsurf/include/netsurf/keypress.h"
#include "../../ports/netsurf/src/netsurf/desktop/browser_history.h"
#include "../drivers/fb.h"
#include "../drivers/font.h"
#include "../drivers/timer.h"
#include "../net/http.h"
#include "../mm/heap.h"
#include "../kernel.h"
#include <stdint.h>
#include <stddef.h>

void *kmalloc(size_t sz);
void  kfree(void *p);

#define TOOLBAR_H    56
#define STATUSBAR_H  32
#define BUTTON_W     36
#define URL_X        152

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

static uint64_t browser_nav_start;

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
    static int status_diag_budget = 24;
    if (status_diag_budget > 0) {
        klog(LOG_DEBUG, "T+%llu BROWSER STATUS %s",
            (unsigned long long)(timer_get_ticks() - browser_nav_start), status);
        status_diag_budget--;
    }
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
    browser_nav_start = timer_get_ticks();
    http_trace_navigation_start(browser_nav_start);
    klog(LOG_INFO, "T+0 BROWSER NAV START url=%s", b->url);
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
    fb_fill_rect(x, y, w, h, aurora_color(AURORA_COLOR_BACKGROUND));
    if (b->netsurf != NULL && b->state != BSTATE_ERROR) {
        if (b->state == BSTATE_LOADING && browser_window_has_content(b->netsurf)) {
            b->state = BSTATE_DONE;
            bcpy(b->status, "Ready", (int)sizeof(b->status));
            klog(LOG_INFO, "T+%llu CONTENT DONE / LOADING CLEARED",
                (unsigned long long)(timer_get_ticks() - browser_nav_start));
        }
        nexos_netsurf_paint(b->win);
    } else if (b->state == BSTATE_ERROR) {
        aurora_card((aurora_rect_t){x + 18, y + 18, w - 36, 86}, 0);
        aurora_badge((aurora_rect_t){x + 36, y + 32, 112, 24},
                     "Network error", AURORA_COLOR_DANGER);
        aurora_text(x + 36, y + 66, "Browser Error", AURORA_TEXT_SECTION,
                    aurora_color(AURORA_COLOR_SURFACE));
        aurora_text(x + 36, y + 91, b->status, AURORA_TEXT_CAPTION,
                    aurora_color(AURORA_COLOR_SURFACE));
    }
}

static void browser_control(aurora_rect_t r, aurora_icon_id_t icon, uint32_t state) {
    aurora_icon_button(r, NULL, state);
    aurora_icon_draw((aurora_rect_t){r.x + 6, r.y + 6, r.w - 12, r.h - 12},
                     icon, aurora_color((state & AURORA_STATE_DISABLED) ?
                                         AURORA_COLOR_TEXT_DISABLED :
                                         AURORA_COLOR_TEXT_SECONDARY));
}

static void browser_paint(window_t *win)
{
    browser_app_t *b = win == NULL ? NULL : win->userdata;
    int wx, wy, ww, client_h, sb_y, cy, ch;
    const char *url_disp;
    char url_tail[BROWSER_URL_MAX];
    int max_url_chars, start, cur_x;

    if (b == NULL) return;
    wx = win->x;
    wy = win->y + WM_TITLEBAR_H;
    ww = win->w;
    client_h = win->h - WM_TITLEBAR_H;

    fb_fill_rect(wx, wy, ww, TOOLBAR_H, aurora_color(AURORA_COLOR_SURFACE));
    fb_fill_rect_blend(wx, wy, ww, 1,
                       aurora_color(AURORA_COLOR_TEXT_PRIMARY), 16);

    browser_control((aurora_rect_t){wx + 14, wy + 10, BUTTON_W, 36},
                    AURORA_ICON_BACK, 0);
    browser_control((aurora_rect_t){wx + 59, wy + 10, BUTTON_W, 36},
                    AURORA_ICON_FORWARD, 0);
    browser_control((aurora_rect_t){wx + 104, wy + 10, BUTTON_W, 36},
                    AURORA_ICON_RELOAD, 0);

    url_disp = b->url_len > 0 ? b->url : "Enter URL";
    int ub_x = wx + URL_X;
    int ub_w = ww - URL_X - 14;
    max_url_chars = (ub_w - 20) / 8;
    start = b->url_len > max_url_chars ? b->url_len - max_url_chars : 0;
    bcpy(url_tail, url_disp + start, BROWSER_URL_MAX);
    uint32_t url_state = b->address_focus ? AURORA_STATE_FOCUSED : 0;
    aurora_text_field((aurora_rect_t){ub_x, wy + 10, ub_w, 36},
                      url_tail, url_state, 0);
    if (b->state == BSTATE_ERROR)
        fb_fill_rect_blend(ub_x + 1, wy + 11, 3, 34,
                           aurora_color(AURORA_COLOR_DANGER), 160);
    if (b->address_focus && b->url_len < max_url_chars) {
        cur_x = ub_x + 10 + (b->url_len - start) * 8;
        fb_fill_rect(cur_x, wy + 16, 1, 22,
                     aurora_color(AURORA_COLOR_ACCENT));
    }
    fb_fill_rect(wx, wy + TOOLBAR_H - 1, ww, 1, COL_SURFACE1);

    sb_y = wy + client_h - STATUSBAR_H;
    fb_fill_rect(wx, sb_y, ww, STATUSBAR_H, aurora_color(AURORA_COLOR_SURFACE));
    fb_fill_rect(wx, sb_y, ww, 1, aurora_color(AURORA_COLOR_BORDER_SUBTLE));
    aurora_color_role_t status_role = b->state == BSTATE_ERROR ? AURORA_COLOR_DANGER :
                    b->state == BSTATE_LOADING ? AURORA_COLOR_WARNING : AURORA_COLOR_SUCCESS;
    aurora_badge((aurora_rect_t){wx + 14, sb_y + 5, 64, 22}, b->status, status_role);
    aurora_text(wx + 92, sb_y + 9, "NetSurf • NexOS networking",
                AURORA_TEXT_CAPTION, aurora_color(AURORA_COLOR_SURFACE));

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
        int handled = 0;
        if (code == 0x80) nskey = NS_KEY_UP;
        else if (code == 0x81) nskey = NS_KEY_DOWN;
        else if (code == 0x82) nskey = NS_KEY_LEFT;
        else if (code == 0x83) nskey = NS_KEY_RIGHT;
        else if (code == 0x84) nskey = NS_KEY_TEXT_START;
        else if (code == 0x85) nskey = NS_KEY_TEXT_END;
        else if (code == 0x86) nskey = NS_KEY_PAGE_UP;
        else if (code == 0x87) nskey = NS_KEY_PAGE_DOWN;
        handled = browser_window_key_press(b->netsurf, nskey) ? 1 : 0;
        {
            static int key_diag_budget = 32;
            if (key_diag_budget > 0) {
                klog(LOG_DEBUG, "BROWSER KEY code=0x%x nskey=0x%x handled=%d",
                     code, (unsigned int)nskey, handled);
                key_diag_budget--;
            }
        }
        if (!handled) {
            bool scrolled = false;
            if (code == 0x80) scrolled = nexos_netsurf_scroll(win, 0, -64);
            else if (code == 0x81) scrolled = nexos_netsurf_scroll(win, 0, 64);
            else if (code == 0x86) scrolled = nexos_netsurf_scroll(win, 0, -320);
            else if (code == 0x87) scrolled = nexos_netsurf_scroll(win, 0, 320);
            else if (code == 0x84) scrolled = nexos_netsurf_scroll(win, 0, -1000000);
            else if (code == 0x85) scrolled = nexos_netsurf_scroll(win, 0, 1000000);
            if (scrolled) {
                static int scroll_key_diag_budget = 16;
                if (scroll_key_diag_budget > 0) {
                    klog(LOG_DEBUG, "BROWSER KEY SCROLL code=0x%x result=handled",
                         code);
                    scroll_key_diag_budget--;
                }
            }
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
    klog(LOG_INFO, "BROWSER RESIZE w=%d h=%d viewport_w=%d viewport_h=%d",
         win->w, win->h, win->w, content_h);
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

    if (cy >= 10 && cy < 46 && cx >= 14 && cx < 50) {
        nserror result = NSERROR_BAD_PARAMETER;
        int available = b->netsurf != NULL &&
                        browser_window_history_back_available(b->netsurf);
        klog(LOG_INFO, "BROWSER TOOLBAR action=BACK");
        if (available) result = browser_window_history_back(b->netsurf, false);
        klog(LOG_INFO, "BROWSER TOOLBAR BACK available=%d result=%d",
             available, (int)result);
        return;
    }
    if (cy >= 10 && cy < 46 && cx >= 59 && cx < 95) {
        nserror result = NSERROR_BAD_PARAMETER;
        int available = b->netsurf != NULL &&
                        browser_window_history_forward_available(b->netsurf);
        klog(LOG_INFO, "BROWSER TOOLBAR action=FORWARD");
        if (available) result = browser_window_history_forward(b->netsurf, false);
        klog(LOG_INFO, "BROWSER TOOLBAR FORWARD available=%d result=%d",
             available, (int)result);
        return;
    }
    if (cy >= 10 && cy < 46 && cx >= 104 && cx < 140) {
        klog(LOG_INFO, "BROWSER TOOLBAR action=RELOAD");
        if (b->netsurf != NULL) (void)browser_window_reload(b->netsurf, false);
        return;
    }
    if (cy >= 10 && cy < 46 && cx >= URL_X) {
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
