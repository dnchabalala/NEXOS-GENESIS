/* NexOS — kernel/gui/term_app.c | GUI Terminal application | MIT License */
#include "term_app.h"
#include "wm.h"
#include "aurora.h"
#include "../drivers/fb.h"
#include "../drivers/font.h"
#include "../drivers/timer.h"
#include "../mm/heap.h"
#include "../kernel.h"
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

void *kmalloc(size_t size);
void  kfree(void *ptr);

/* Active terminal — nsh output target */
static term_app_t *g_active_term = NULL;

void term_set_active(term_app_t *t) { g_active_term = t; }
term_app_t *term_get_active(void)   { return g_active_term; }

/* Char-by-char output callback forwarded from nsh → active terminal */
static void term_char_out(char c) {
    if (!g_active_term) return;
    term_app_t *t = g_active_term;
    if (c == '\n') {
        t->col = 0; t->row++;
        if (t->row >= TERM_ROWS) {
            for (int r = 0; r < TERM_ROWS - 1; r++)
                for (int ci = 0; ci <= TERM_COLS; ci++) t->buf[r][ci] = t->buf[r+1][ci];
            for (int ci = 0; ci <= TERM_COLS; ci++) t->buf[TERM_ROWS-1][ci] = 0;
            t->row = TERM_ROWS - 1;
        }
    } else {
        if (t->col >= TERM_COLS) { t->col = 0; t->row++; }
        if (t->row >= TERM_ROWS) t->row = TERM_ROWS - 1;
        t->buf[t->row][t->col++] = c;
    }
    t->dirty = 1;
}

/* ── Internal helpers ────────────────────────────────────────────────────── */
static void term_scroll(term_app_t *t) {
    if (t->row < TERM_ROWS - 1) return;
    for (int r = 0; r < TERM_ROWS - 1; r++)
        for (int c = 0; c <= TERM_COLS; c++)
            t->buf[r][c] = t->buf[r + 1][c];
    for (int c = 0; c <= TERM_COLS; c++) t->buf[TERM_ROWS - 1][c] = 0;
    t->row = TERM_ROWS - 1;
}

static void term_newline(term_app_t *t) {
    t->col = 0; t->row++; term_scroll(t);
}

void term_puts(term_app_t *t, const char *s) {
    while (*s) {
        if (*s == '\n')       { term_newline(t); }
        else if (*s == '\r')  { t->col = 0; }
        else {
            if (t->col >= TERM_COLS) term_newline(t);
            t->buf[t->row][t->col++] = *s;
        }
        s++;
    }
    t->dirty = 1;
}

void term_printf(term_app_t *t, const char *fmt, ...) {
    char buf[512];
    va_list ap; va_start(ap, fmt);
    int bi = 0;
    while (*fmt && bi < 510) {
        if (*fmt != '%') { buf[bi++] = *fmt++; continue; }
        fmt++;
        int kw = 0; char kp = ' ';
        if (*fmt == '0') { kp = '0'; fmt++; }
        while (*fmt >= '0' && *fmt <= '9') { kw = kw * 10 + (*fmt - '0'); fmt++; }
        switch (*fmt) {
        case 'd': { int64_t v = va_arg(ap, int64_t);
            if (v < 0) { buf[bi++] = '-'; v = -v; }
            char t2[20]; int ti = 0;
            do { t2[ti++] = '0' + (int)(v % 10); v /= 10; } while (v);
            while (ti < kw && bi < 510) { buf[bi++] = kp; kw--; }
            while (ti > 0 && bi < 510) buf[bi++] = t2[--ti];
            break; }
        case 'u': { uint64_t v = va_arg(ap, uint64_t);
            char t2[20]; int ti = 0;
            do { t2[ti++] = '0' + (int)(v % 10); v /= 10; } while (v);
            while (ti < kw && bi < 510) { buf[bi++] = kp; kw--; }
            while (ti > 0 && bi < 510) buf[bi++] = t2[--ti];
            break; }
        case 'x': { uint64_t v = va_arg(ap, uint64_t);
            const char *hx = "0123456789abcdef"; char t2[16]; int ti = 0;
            do { t2[ti++] = hx[v & 0xF]; v >>= 4; } while (v);
            while (ti < kw && bi < 510) { buf[bi++] = kp; kw--; }
            while (ti > 0 && bi < 510) buf[bi++] = t2[--ti];
            break; }
        case 's': { const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            while (*s && bi < 510) buf[bi++] = *s++;
            break; }
        case '%': buf[bi++] = '%'; break;
        default:  buf[bi++] = '%'; buf[bi++] = *fmt; break;
        }
        fmt++;
    }
    va_end(ap);
    buf[bi] = 0;
    term_puts(t, buf);
}

static void term_prompt(term_app_t *t) {
    term_puts(t, "\n[root@nexos]$ ");
}

/* ── Built-in commands ───────────────────────────────────────────────────── */
static void term_exec(term_app_t *t, const char *cmd) {
    while (*cmd == ' ') cmd++;
    if (!*cmd) return;

    extern void nsh_set_output(void (*fn)(char));
    extern void nsh_exec_command(const char *cmd);

    term_set_active(t);
    nsh_set_output(term_char_out);
    nsh_exec_command(cmd);
    nsh_set_output(0);
}

/* ── Paint ───────────────────────────────────────────────────────────────── */
static int term_starts_prompt(const char *s) {
    const char *p = "[root@nexos]$ ";
    int i = 0;
    while (p[i] && s[i] == p[i]) i++;
    return p[i] == 0;
}

static void term_draw_line(term_app_t *t, const char *line, int row,
                           int x, int y, uint32_t bg) {
    if (!line || !*line) return;
    if (row == 0) {
        font_aurora_puts(x, y, line, 16,
                         aurora_color(AURORA_COLOR_TEXT_PRIMARY), bg);
        return;
    }
    if (row == 1) {
        font_aurora_puts(x, y + 1, line, 12,
                         aurora_color(AURORA_COLOR_TEXT_MUTED), bg);
        return;
    }
    if (term_starts_prompt(line)) {
        const char *rest = line + 14;
        font_puts(x, y, "[root@nexos]$", COL_LAVENDER, bg);
        font_puts(x + 14 * 8, y, rest, t->fg, bg);
    } else {
        uint32_t color = (line[0] == 'n' && line[1] == 's' && line[2] == 'h' && line[3] == ':')
                       ? COL_RED : t->fg;
        font_puts(x, y, line, color, bg);
    }
}

static void term_paint(window_t *win) {
    term_app_t *t = (term_app_t *)win->userdata;
    if (!t) return;

    int bx = win->x;
    int by = win->y + WM_TITLEBAR_H;
    int bw = win->w;
    int bh = win->h - WM_TITLEBAR_H;
    uint32_t bg = 0x080B11; /* canonical .term background */
    int line_h = 18;

    /* HTML .term: full client surface, 20px padding, monospace content. */
    fb_fill_rect(bx, by, bw, bh, bg);
    fb_fill_rect_blend(bx, by, 2, bh,
                       aurora_color(AURORA_COLOR_ACCENT), 44);

    int visible_rows = (bh - TERM_PAD * 2) / line_h;
    if (visible_rows < 1) visible_rows = 1;
    int first = t->row >= visible_rows ? t->row - visible_rows + 1 : 0;
    int text_x = bx + 20;
    int text_y = by + 20;
    for (int r = first; r < TERM_ROWS && r < first + visible_rows; r++) {
        if (!t->buf[r][0] && r > t->row) break;
        term_draw_line(t, t->buf[r], r, text_x,
                       text_y + (r - first) * line_h, bg);
    }

    /* Blinking cursor remains bounded to the terminal client. */
    uint64_t tick = timer_get_ticks();
    if ((tick / 500) % 2 == 0) {
        int cx = text_x + t->col * 8;
        int cy = text_y + (t->row - first) * line_h;
        if (t->row >= first && cy + 16 < by + bh - TERM_PAD) {
            fb_fill_rect(cx, cy + 14, 8, 2,
                         aurora_color(AURORA_COLOR_ACCENT));
            fb_fill_rect_blend(cx, cy, 8, 14,
                               aurora_color(AURORA_COLOR_ACCENT), 30);
        }
    }
}

static void term_key(window_t *win, char key) {
    term_app_t *t = (term_app_t *)win->userdata;
    if (!t) return;
    if (key == '\n' || key == '\r') {
        t->input[t->input_len] = 0;
        term_newline(t);
        if (t->input_len > 0) term_exec(t, t->input);
        t->input_len = 0;
        for (int i = 0; i < 256; i++) t->input[i] = 0;
        term_prompt(t);
    } else if (key == '\b') {
        if (t->input_len > 0) {
            t->input[--t->input_len] = 0;
            if (t->col > 0) t->col--;
            t->buf[t->row][t->col] = ' ';
        }
    } else if (key >= 32 && key < 127) {
        if (t->input_len < 255) {
            t->input[t->input_len++] = key;
            if (t->col >= TERM_COLS) term_newline(t);
            t->buf[t->row][t->col++] = key;
        }
    }
    t->dirty = 1;
    wm_invalidate(win);
}

static void term_close(window_t *win) {
    term_app_t *t = (term_app_t *)win->userdata;
    if (t) {
        if (g_active_term == t) g_active_term = NULL;
        kfree(t);
        win->userdata = NULL;
    }
    wm_close(win);
}

/* ── Constructor ─────────────────────────────────────────────────────────── */
term_app_t *term_create(int x, int y) {
    window_t *win = wm_new(x, y,
                           TERM_COLS * 8 + TERM_PAD * 2 + 6,
                           TERM_ROWS * 16 + WM_TITLEBAR_H + TERM_PAD * 2 + 20,
                           "Terminal");
    if (!win) return NULL;

    term_app_t *t = (term_app_t *)kmalloc(sizeof(term_app_t));
    if (!t) { wm_close(win); return NULL; }
    for (int i = 0; i < (int)sizeof(term_app_t); i++) ((uint8_t *)t)[i] = 0;

    t->win = win;
    t->fg  = COL_TEXT;
    t->bg  = COL_CRUST;   /* deeper dark background */

    win->on_paint = term_paint;
    win->on_key   = term_key;
    win->on_close = term_close;
    win->userdata = t;

    term_puts(t, "NexOS Terminal - native shell\n");
    term_puts(t, "Type help for commands.\n\n");
    term_prompt(t);

    if (!g_active_term) g_active_term = t;
    return t;
}
