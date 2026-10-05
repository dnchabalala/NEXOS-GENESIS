/* NexOS — kernel/gui/files_app.c | GUI File Manager | MIT License */
#include "files_app.h"
#include "wm.h"
#include "aurora.h"
#include "../drivers/fb.h"
#include "../drivers/font.h"
#include "../fs/vfs.h"
#include "../mm/heap.h"
#include "../kernel.h"
#include <stdint.h>
#include <stddef.h>

void *kmalloc(size_t size);
void  kfree(void *ptr);

static int fstrlen(const char *s) { int n = 0; while (s[n]) n++; return n; }
static void fstrcpy(char *d, const char *s, int max) {
    int i = 0; while (i < max - 1 && s[i]) { d[i] = s[i]; i++; } d[i] = 0;
}

static void format_size(uint64_t sz, char *buf) {
    if (sz >= 1024 * 1024) {
        uint64_t mb = sz / (1024 * 1024);
        char t[8]; int ti = 0;
        if (!mb) { t[ti++] = '0'; } else while (mb) { t[ti++] = '0'+(int)(mb%10); mb/=10; }
        int bi = 0; while (ti > 0) buf[bi++] = t[--ti];
        buf[bi++] = 'M'; buf[bi] = 0;
    } else if (sz >= 1024) {
        uint64_t k = sz / 1024;
        char t[8]; int ti = 0;
        if (!k) { t[ti++] = '0'; } else while (k) { t[ti++] = '0'+(int)(k%10); k/=10; }
        int bi = 0; while (ti > 0) buf[bi++] = t[--ti];
        buf[bi++] = 'K'; buf[bi] = 0;
    } else {
        char t[8]; int ti = 0;
        uint64_t v = sz;
        if (!v) { t[ti++] = '0'; } else while (v) { t[ti++] = '0'+(int)(v%10); v/=10; }
        int bi = 0; while (ti > 0) buf[bi++] = t[--ti];
        buf[bi++] = 'B'; buf[bi] = 0;
    }
}

static void files_load_dir(files_app_t *f) {
    f->entry_count = 0; f->selected = 0; f->scroll = 0;
    vfs_node_t *dir = vfs_open(f->cwd, 0);
    if (!dir) return;
    vfs_dirent_t de;
    int idx = 0;
    while (idx < FILES_MAX_ENT) {
        if (vfs_readdir(dir, (uint32_t)idx, &de) != 0) break;
        fstrcpy(f->entry_names[f->entry_count], de.name, VFS_NAME_MAX);
        char full[256];
        int cl = fstrlen(f->cwd);
        fstrcpy(full, f->cwd, 250);
        if (cl > 0 && f->cwd[cl - 1] != '/') { full[cl] = '/'; full[cl + 1] = 0; }
        int fl = fstrlen(full);
        fstrcpy(full + fl, de.name, 250 - fl);
        vfs_stat_t st;
        if (vfs_stat(full, &st) == 0) {
            f->entry_types[f->entry_count] = st.type;
            f->entry_sizes[f->entry_count] = st.size;
        } else {
            f->entry_types[f->entry_count] = VFS_NODE_FILE;
            f->entry_sizes[f->entry_count] = 0;
        }
        f->entry_count++;
        idx++;
    }
    vfs_close(dir);
}

static void files_text(int x, int y, const char *text, int size,
                       uint32_t fg, uint32_t bg) {
    if (text && *text) font_aurora_puts(x, y, text, size, fg, bg);
}

static void files_number(int value, char *out, int cap) {
    int i = 0, n = value;
    if (cap < 2) return;
    if (n == 0) out[i++] = '0';
    else {
        char rev[12]; int r = 0;
        while (n > 0 && r < (int)sizeof(rev)) { rev[r++] = (char)('0' + n % 10); n /= 10; }
        while (r > 0 && i < cap - 1) out[i++] = rev[--r];
    }
    out[i] = 0;
}

static int files_sidebar_width(int ww) {
    if (ww >= 520) return 180;
    if (ww >= 360) return 132;
    return 0;
}

static void files_paint(window_t *win) {
    files_app_t *f = (files_app_t *)win->userdata;
    if (!f) return;
    int wx = win->x, wy = win->y + WM_TITLEBAR_H;
    int ww = win->w;
    int client_h = win->h - WM_TITLEBAR_H;

    /* HTML .split: Places column + content column.  The sidebar contracts
     * only when the native window is narrower than the canonical 240px. */
    int side_w = files_sidebar_width(ww);
    int content_x = wx + side_w;
    int content_w = ww - side_w;
    uint32_t surface = aurora_color(AURORA_COLOR_SURFACE);
    uint32_t elevated = aurora_color(AURORA_COLOR_ELEVATED);
    uint32_t bg = aurora_color(AURORA_COLOR_BACKGROUND);

    fb_fill_rect(wx, wy, ww, client_h, bg);
    if (side_w > 0) {
        fb_fill_rect(wx, wy, side_w, client_h, aurora_color(AURORA_COLOR_INACTIVE));
        fb_fill_rect(wx + side_w - 1, wy, 1, client_h,
                     aurora_color(AURORA_COLOR_BORDER));
        files_text(wx + 18, wy + 18, "PLACES", 11,
                   aurora_color(AURORA_COLOR_TEXT_MUTED),
                   aurora_color(AURORA_COLOR_INACTIVE));
        const char *places[] = { "Home", "Desktop", "Documents", "Downloads", "System", "Trash" };
        for (int i = 0; i < 6; i++) {
            int ny = wy + 48 + i * 36;
            uint32_t nbg = (i == 0) ? aurora_color(AURORA_COLOR_SELECTED)
                                    : aurora_color(AURORA_COLOR_INACTIVE);
            if (i == 0) fb_fill_rounded_rect(wx + 10, ny, side_w - 20, 32,
                                               AURORA_RADIUS_SMALL, nbg);
            files_text(wx + 22, ny + 8, places[i], 14,
                       (i == 0) ? aurora_color(AURORA_COLOR_TEXT_PRIMARY)
                                : aurora_color(AURORA_COLOR_TEXT_SECONDARY), nbg);
        }
    }

    /* HTML .toolbar: back, forward, up, breadcrumb field, search field. */
    int toolbar_y = wy;
    fb_fill_rect(content_x, toolbar_y, content_w, 56, surface);
    int bx = content_x + 14;
    int button_w = 30;
    aurora_icon_button((aurora_rect_t){bx, toolbar_y + 10, button_w, 36},
                       "‹", AURORA_STATE_DISABLED);
    bx += button_w + 9;
    aurora_icon_button((aurora_rect_t){bx, toolbar_y + 10, button_w, 36},
                       "›", AURORA_STATE_DISABLED);
    bx += button_w + 9;
    aurora_icon_button((aurora_rect_t){bx, toolbar_y + 10, button_w, 36},
                       "↑", AURORA_STATE_NORMAL);
    bx += button_w + 9;
    int search_w = content_w > 430 ? 132 : 0;
    int path_w = content_w - (bx - content_x) - search_w - (search_w ? 9 : 0) - 14;
    if (path_w < 72) path_w = 72;
    aurora_text_field((aurora_rect_t){bx, toolbar_y + 10, path_w, 36},
                      f->cwd, AURORA_STATE_NORMAL, 0);
    if (search_w) {
        aurora_text_field((aurora_rect_t){bx + path_w + 9, toolbar_y + 10,
                                          search_w, 36},
                          "Search files", AURORA_STATE_NORMAL, 0);
    }
    aurora_separator(content_x, toolbar_y + 55, content_w);

    int sb_y = wy + client_h - 34;
    fb_fill_rect(content_x, sb_y, content_w, 34, surface);
    aurora_separator(content_x, sb_y, content_w);
    char count_buf[16]; files_number(f->entry_count, count_buf, sizeof(count_buf));
    files_text(content_x + 14, sb_y + 9, count_buf, 12,
               aurora_color(AURORA_COLOR_TEXT_SECONDARY), surface);
    files_text(content_x + 14 + fstrlen(count_buf) * 7 + 6, sb_y + 9,
               "items", 12, aurora_color(AURORA_COLOR_TEXT_MUTED), surface);

    int list_x = content_x + 26;
    int list_w = content_w - 52;
    int list_y = wy + 56 + 26;
    int max_y = sb_y - 18;
    if (list_w < 80) { list_x = content_x + 8; list_w = content_w - 16; }

    files_text(list_x, wy + 56 + 26, "Home", 20,
               aurora_color(AURORA_COLOR_TEXT_PRIMARY), bg);
    files_text(list_x, wy + 56 + 54, f->cwd, 12,
               aurora_color(AURORA_COLOR_TEXT_MUTED), bg);
    list_y += 52;

    if (f->entry_count == 0) {
        files_text(list_x + 12, list_y + 42, "Empty folder", 16,
                   aurora_color(AURORA_COLOR_TEXT_MUTED), bg);
    }

    int row_h = 38;
    for (int i = f->scroll; i < f->entry_count && list_y + row_h <= max_y; i++) {
        int is_dir = (f->entry_types[i] & VFS_NODE_DIR);
        int is_sel = (i == f->selected);
        uint32_t row_bg = is_sel ? aurora_color(AURORA_COLOR_SELECTED) : bg;
        if (is_sel) fb_fill_rounded_rect(list_x, list_y, list_w, row_h,
                                         AURORA_RADIUS_SMALL, row_bg);
        files_text(list_x + 14, list_y + 10, is_dir ? "▣" : "▤", 14,
                   is_dir ? aurora_color(AURORA_COLOR_ACCENT)
                          : aurora_color(AURORA_COLOR_TEXT_SECONDARY), row_bg);
        files_text(list_x + 40, list_y + 10, f->entry_names[i], 14,
                   aurora_color(AURORA_COLOR_TEXT_PRIMARY), row_bg);
        char detail[20];
        if (is_dir) fstrcpy(detail, "folder", sizeof(detail));
        else format_size(f->entry_sizes[i], detail);
        int dw = font_aurora_str_width(detail, 12);
        files_text(list_x + list_w - dw - 14, list_y + 11, detail, 12,
                   aurora_color(AURORA_COLOR_TEXT_MUTED), row_bg);
        aurora_separator(list_x, list_y + row_h - 1, list_w);
        list_y += row_h;
    }
}

static void files_click(window_t *win, int cx, int cy, int btn) {
    (void)btn;
    files_app_t *f = (files_app_t *)win->userdata;
    if (!f) return;

    int side_w = files_sidebar_width(win->w);
    int content_x = side_w;
    /* HTML toolbar: only Up has an existing backend action. */
    if (cx >= content_x + 14 + 2 * 39 &&
        cx < content_x + 14 + 3 * 39 && cy >= 0 && cy < 56) {
        int cl = fstrlen(f->cwd);
        if (cl > 1) {
            int i = cl - 1;
            while (i > 0 && f->cwd[i] != '/') i--;
            if (i == 0) f->cwd[1] = 0; else f->cwd[i] = 0;
            files_load_dir(f);
        }
        return;
    }
    if (cx < content_x || cy < 56) return;

    int item_y = cy - 56 - 26 - 52;
    if (item_y < 0) return;
    int idx = f->scroll + item_y / 38;
    if (idx < 0 || idx >= f->entry_count) return;

    if (f->selected == idx) {
        if (f->entry_types[idx] & VFS_NODE_DIR) {
            int cl = fstrlen(f->cwd);
            char npath[256];
            fstrcpy(npath, f->cwd, 250);
            if (cl > 1) { npath[cl] = '/'; npath[cl + 1] = 0; cl++; }
            fstrcpy(npath + cl, f->entry_names[idx], 250 - cl);
            fstrcpy(f->cwd, npath, 256);
            files_load_dir(f);
        }
    } else {
        f->selected = idx;
    }
    (void)cx;
}

static void files_wheel(window_t *win, int cx, int cy, int delta) {
    (void)cx; (void)cy;
    files_app_t *f = (files_app_t *)win->userdata;
    if (!f || f->entry_count <= 0) return;
    int visible = (win->h - WM_TITLEBAR_H - 150) / 38;
    if (visible < 1) visible = 1;
    int max_scroll = f->entry_count - visible;
    if (max_scroll < 0) max_scroll = 0;
    f->scroll += delta < 0 ? 2 : -2;
    if (f->scroll < 0) f->scroll = 0;
    if (f->scroll > max_scroll) f->scroll = max_scroll;
    wm_invalidate(win);
}

static void files_close(window_t *win) {
    files_app_t *f = (files_app_t *)win->userdata;
    if (f) { kfree(f); win->userdata = NULL; }
    wm_close(win);
}

files_app_t *files_create(int x, int y) {
    window_t *win = wm_new(x, y, 460, 480, "Files");
    if (!win) return NULL;
    files_app_t *f = (files_app_t *)kmalloc(sizeof(files_app_t));
    if (!f) { wm_close(win); return NULL; }
    for (int i = 0; i < (int)sizeof(files_app_t); i++) ((uint8_t *)f)[i] = 0;

    f->win = win;
    f->cwd[0] = '/'; f->cwd[1] = 0;
    files_load_dir(f);

    win->on_paint = files_paint;
    win->on_click = files_click;
    win->on_mouse_wheel = files_wheel;
    win->on_close = files_close;
    win->userdata = f;
    return f;
}
