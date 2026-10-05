/* NexOS — kernel/gui/taskbar.c | Aurora floating dock | MIT License */
#include "taskbar.h"
#include "aurora.h"
#include "wm.h"
#include "launcher.h"
#include "../drivers/fb.h"
#include "../kernel.h"

void launch_filemanager(void);
void launch_terminal(void);
void launch_browser(void);
void launch_sysmon(void);
void launch_settings(void);

#define DOCK_H       66
#define DOCK_PAD_X   16
#define DOCK_PAD_Y    9
#define DOCK_GAP     12
#define DOCK_ITEM    48
#define DOCK_RADIUS  24

static int dock_x, dock_y, dock_w;
static int hover_item = -1;

static uint32_t dock_mix(uint32_t base, uint32_t tint, uint8_t alpha) {
    return fb_blend(tint, base, alpha);
}

typedef struct {
    aurora_icon_id_t icon;
    const char *title;
    void (*launch)(void);
} dock_item_t;

static const dock_item_t dock_items[] = {
    { AURORA_ICON_APPS,     "Apps",     NULL },
    { AURORA_ICON_FILES,    "Files",    launch_filemanager },
    { AURORA_ICON_TERMINAL, "Terminal", launch_terminal },
    { AURORA_ICON_BROWSER,  "Browser",  launch_browser },
    { AURORA_ICON_MONITOR,  "Monitor",  launch_sysmon },
    { AURORA_ICON_SETTINGS, "Settings", launch_settings }
};
#define DOCK_COUNT ((int)(sizeof(dock_items) / sizeof(dock_items[0])))

static int dock_count(void) { return DOCK_COUNT; }

static int dock_running(const char *title, window_t **out) {
    for (int i = 0; i < wm_window_count(); i++) {
        window_t *win = wm_get_window(i);
        if (win && win->visible && win->title[0] &&
            win->title[0] == title[0]) {
            if (out) *out = win;
            return 1;
        }
    }
    if (out) *out = NULL;
    return 0;
}

static void dock_layout(void) {
    int count = dock_count();
    dock_w = DOCK_PAD_X * 2 + count * DOCK_ITEM + (count - 1) * DOCK_GAP;
    if (dock_w > (int)fb.width - 24) dock_w = (int)fb.width - 24;
    dock_x = ((int)fb.width - dock_w) / 2;
    dock_y = (int)fb.height - 24 - DOCK_H;
    if (dock_y < 0) dock_y = 0;
}

void taskbar_init(void) {
    dock_layout();
    hover_item = -1;
    klog(LOG_INFO, "Dock: initialized x=%d y=%d w=%d h=%d",
         dock_x, dock_y, dock_w, DOCK_H);
}

int taskbar_get_y(void) { dock_layout(); return dock_y; }

int taskbar_contains(int x, int y) {
    dock_layout();
    return x >= dock_x && x < dock_x + dock_w &&
           y >= dock_y && y < dock_y + DOCK_H;
}

static int item_at(int x, int y) {
    dock_layout();
    if (!taskbar_contains(x, y)) return -1;
    int count = dock_count();
    for (int i = 0; i < count; i++) {
        int ix = dock_x + DOCK_PAD_X + i * (DOCK_ITEM + DOCK_GAP);
        if (x >= ix && x < ix + DOCK_ITEM &&
            y >= dock_y + DOCK_PAD_Y && y < dock_y + DOCK_PAD_Y + DOCK_ITEM)
            return i;
    }
    return -1;
}

void taskbar_get_apps_rect(int *x, int *y, int *w, int *h) {
    dock_layout();
    if (x) *x = dock_x + DOCK_PAD_X;
    if (y) *y = dock_y + DOCK_PAD_Y;
    if (w) *w = DOCK_ITEM;
    if (h) *h = DOCK_ITEM;
}

void taskbar_handle_mouse(int mx, int my) {
    int next = item_at(mx, my);
    if (next != hover_item) { hover_item = next; fb_scene_dirty = 1; }
}

void taskbar_draw(void) {
    if (!fb.initialized) return;
    dock_layout();
    uint32_t bg = aurora_color(AURORA_COLOR_SURFACE);
    fb_fill_rounded_rect(dock_x + 4, dock_y + 5, dock_w, DOCK_H, DOCK_RADIUS,
                         dock_mix(aurora_color(AURORA_COLOR_BACKGROUND), bg, 64));
    fb_fill_rounded_rect(dock_x + 2, dock_y + 3, dock_w, DOCK_H, DOCK_RADIUS,
                         dock_mix(aurora_color(AURORA_COLOR_BACKGROUND), bg, 116));
    fb_fill_rounded_rect(dock_x, dock_y, dock_w, DOCK_H, DOCK_RADIUS, bg);
    fb_draw_rect_outline(dock_x, dock_y, dock_w, DOCK_H,
                         aurora_color(AURORA_COLOR_BORDER_SUBTLE), 1);
    fb_fill_rect_blend(dock_x + DOCK_RADIUS, dock_y + 1,
                       dock_w - 2 * DOCK_RADIUS, 1,
                       aurora_color(AURORA_COLOR_TEXT_PRIMARY), 26);

    int count = dock_count();
    int ix = dock_x + DOCK_PAD_X;
    int iy = dock_y + DOCK_PAD_Y;
    uint32_t apps_state = hover_item == 0 ? AURORA_STATE_HOVER : 0;
    if (launcher_is_visible()) apps_state |= AURORA_STATE_SELECTED;
    aurora_card((aurora_rect_t){ix, iy, DOCK_ITEM, DOCK_ITEM}, apps_state);
    aurora_app_icon_id(ix + 24, iy + 24, 16, AURORA_ICON_APPS,
                       aurora_color(AURORA_COLOR_ACCENT));
    if (launcher_is_visible())
        fb_fill_rounded_rect(ix + 16, dock_y + DOCK_H - 7, 16, 4, 2,
                             aurora_color(AURORA_COLOR_ACCENT));
    ix += DOCK_ITEM + DOCK_GAP;

    for (int i = 1; i < count; i++) {
        window_t *win = NULL;
        int running = dock_running(dock_items[i].title, &win);
        uint32_t state = (running && win && win->focused) ?
                         AURORA_STATE_SELECTED : 0;
        if (hover_item == i) state |= AURORA_STATE_HOVER;
        aurora_card((aurora_rect_t){ix, iy, DOCK_ITEM, DOCK_ITEM}, state);
        aurora_app_icon_id(ix + 24, iy + 24, 16, dock_items[i].icon,
                        !running ? aurora_color(AURORA_COLOR_TEXT_MUTED) :
                        (win->state == WIN_MINIMIZED ?
                        aurora_color(AURORA_COLOR_TEXT_MUTED) :
                        (win->focused ? aurora_color(AURORA_COLOR_ACCENT) :
                                         aurora_color(AURORA_COLOR_INFORMATION))));
        if (running)
            fb_fill_rounded_rect(ix + 16, dock_y + DOCK_H - 7, 16, 4, 2,
                                 win && win->focused ?
                                 aurora_color(AURORA_COLOR_ACCENT) :
                                 aurora_color(AURORA_COLOR_BORDER));
        ix += DOCK_ITEM + DOCK_GAP;
    }
}

void taskbar_handle_click(int x, int y) {
    int hit = item_at(x, y);
    if (hit < 0) return;
    if (hit == 0) {
        if (launcher_is_visible()) launcher_hide();
        else launcher_show(x, y);
        return;
    }
    if (hit >= 1 && hit < dock_count()) {
        window_t *win = NULL;
        if (dock_running(dock_items[hit].title, &win)) {
            if (win->state == WIN_MINIMIZED) {
                win->state = WIN_NORMAL;
                wm_focus(win); wm_raise(win); fb_scene_dirty = 1;
            } else if (win->focused) wm_minimize(win);
            else { wm_focus(win); wm_raise(win); fb_scene_dirty = 1; }
        } else if (dock_items[hit].launch) {
            dock_items[hit].launch();
        }
    }
}

void taskbar_update(void) { }
