/* NexOS — kernel/gui/notif.c
 * Toast notifications with slide-in (ease-out-cubic) and fade-out animations.
 * MIT License */
#include "notif.h"
#include "anim.h"
#include "aurora.h"
#include "../drivers/fb.h"
#include "../drivers/font.h"
#include <stdint.h>

#define NOTIF_W    320
#define NOTIF_H     72
#define NOTIF_PAD   15
#define NOTIF_GAP   10

#define FADE_MS    600   /* fade-out window at end of life (ms) */

typedef struct {
    char     title[32];
    char     body[80];
    uint32_t ms_left;
    uint32_t ms_total;
    int      slide_in;   /* 0 = off-screen right, 256 = parked */
    uint8_t  active;
} notif_t;

static notif_t notifs[NOTIF_MAX];

static void nstr_cpy(char *d, const char *s, int max) {
    int i = 0; while (i < max - 1 && s[i]) { d[i] = s[i]; i++; } d[i] = 0;
}

void notif_init(void) {
    for (int i = 0; i < NOTIF_MAX; i++) notifs[i].active = 0;
}

void notif_show(const char *title, const char *body, uint32_t ms) {
    for (int i = 0; i < NOTIF_MAX; i++) {
        if (!notifs[i].active) {
            nstr_cpy(notifs[i].title, title, 32);
            nstr_cpy(notifs[i].body,  body,  80);
            notifs[i].ms_left  = ms;
            notifs[i].ms_total = ms;
            /* The compositor is event-driven; show the toast in its parked
             * position rather than forcing a full-scene animation loop. */
            notifs[i].slide_in = 256;
            notifs[i].active   = 1;
            fb_scene_dirty = 1;
            return;
        }
    }
    /* evict oldest */
    for (int i = 0; i < NOTIF_MAX - 1; i++) notifs[i] = notifs[i + 1];
    int last = NOTIF_MAX - 1;
    nstr_cpy(notifs[last].title, title, 32);
    nstr_cpy(notifs[last].body,  body,  80);
    notifs[last].ms_left  = ms;
    notifs[last].ms_total = ms;
    notifs[last].slide_in = 256;
    notifs[last].active   = 1;
    fb_scene_dirty = 1;
}

void notif_tick(uint32_t delta_ms) {
    for (int i = 0; i < NOTIF_MAX; i++) {
        if (!notifs[i].active) continue;

        /* age out */
        if (notifs[i].ms_left <= delta_ms) {
            notifs[i].active = 0;
            fb_scene_dirty = 1;
        }
        else notifs[i].ms_left -= delta_ms;
    }
}

void notif_draw(void) {
    if (!fb.initialized) return;

    int count = 0;
    for (int i = NOTIF_MAX - 1; i >= 0; i--) {
        if (!notifs[i].active) continue;

        /* ── Position ── */
        int nx_rest = (int)fb.width - NOTIF_W - 24;
        int ny      = (int)fb.height - 24 - 66 -
                      (count + 1) * (NOTIF_H + NOTIF_GAP);

        /* Ease-out-cubic slide: notification decelerates into resting position */
        int eased = anim_ease_out_cubic(notifs[i].slide_in);
        int x_off = (NOTIF_W + 20) * (256 - eased) / 256;
        int nx    = nx_rest + x_off;

        /* ── Fade-out when near end of life ── */
        int fade = 256;
        if (notifs[i].slide_in >= 256 && notifs[i].ms_left < (uint32_t)FADE_MS)
            fade = (int)(notifs[i].ms_left * 256 / FADE_MS);
        uint8_t bg_alpha   = (uint8_t)(220 * fade / 256);
        uint8_t rim_alpha  = (uint8_t)(160 * fade / 256);
        uint8_t acc_alpha  = (uint8_t)(255 * fade / 256);

        if (bg_alpha < 4) { count++; continue; }

        /* ── Aurora toast surface ── */
        fb_fill_rect_blend(nx + 4, ny + 4, NOTIF_W, NOTIF_H,
                           aurora_color(AURORA_COLOR_BACKGROUND),
                           (uint8_t)(bg_alpha / 3));
        aurora_panel((aurora_rect_t){nx, ny, NOTIF_W, NOTIF_H}, 1);
        fb_fill_rect_blend(nx + 12, ny, NOTIF_W - 24, 1,
                           aurora_color(AURORA_COLOR_BORDER_FOCUSED), rim_alpha);

        /* ── Left accent bar ── */
        fb_fill_rect_blend(nx, ny + 6, 3, NOTIF_H - 12,
                           aurora_color(AURORA_COLOR_ACCENT), acc_alpha);

        /* ── Progress underline — shows remaining life ── */
        if (notifs[i].ms_total > 0) {
            int bar_w = (int)((uint32_t)NOTIF_W * notifs[i].ms_left /
                              notifs[i].ms_total);
            if (bar_w > 0)
                fb_fill_rect_blend(nx, ny + NOTIF_H - 2, bar_w, 2,
                                   aurora_color(AURORA_COLOR_ACCENT),
                                   (uint8_t)(120 * fade / 256));
        }

        /* ── Text (fades toward background colour as notification expires) ── */
        if (fade > 12) {
            uint32_t c_title = anim_color_lerp(
                aurora_color(AURORA_COLOR_ELEVATED),
                aurora_color(AURORA_COLOR_ACCENT), fade);
            uint32_t c_body  = anim_color_lerp(
                aurora_color(AURORA_COLOR_ELEVATED),
                aurora_color(AURORA_COLOR_TEXT_PRIMARY), fade);
            font_aurora_puts(nx + NOTIF_PAD + 5, ny + 10, notifs[i].title,
                             14, c_title, aurora_color(AURORA_COLOR_ELEVATED));
            font_aurora_puts(nx + NOTIF_PAD + 5, ny + 32, notifs[i].body,
                             12, c_body, aurora_color(AURORA_COLOR_ELEVATED));
        }

        count++;
        if (count >= NOTIF_MAX) break;
    }
}
