/* NexOS — kernel/gui/shell.c | Aurora top bar and navigation rail | MIT License */
#include "shell.h"
#include "aurora.h"
#include "wm.h"
#include "taskbar.h"
#include "../drivers/fb.h"
#include "../drivers/font.h"
#include "../drivers/rtc.h"
#include "../drivers/wifi.h"
#include "../net/netif.h"

static void shell_text(int x, int y, const char *s, aurora_color_role_t role,
                       uint32_t bg) {
    font_aurora_puts(x, y, s, 14, aurora_color(role), bg);
}

static uint32_t shell_mix(uint32_t base, uint32_t tint, uint8_t alpha) {
    return fb_blend(tint, base, alpha);
}

static void shell_clock(char out[6]) {
    rtc_time_t t;
    rtc_get_time(&t);
    out[0] = '0' + t.hour / 10;
    out[1] = '0' + t.hour % 10;
    out[2] = ':';
    out[3] = '0' + t.minute / 10;
    out[4] = '0' + t.minute % 10;
    out[5] = 0;
}

void shell_draw_topbar(void) {
    if (!fb.initialized) return;
    int x = 16, y = 12, w = (int)fb.width - 32, h = 44;
    if (w < 160) return;
    uint32_t bg = aurora_color(AURORA_COLOR_SURFACE);
    /* The HTML top bar is a raised glass surface, not a flat strip. */
    fb_fill_rounded_rect(x + 3, y + 4, w, h, 16,
                         shell_mix(aurora_color(AURORA_COLOR_BACKGROUND), bg, 74));
    fb_fill_rounded_rect(x + 1, y + 2, w, h, 16,
                         shell_mix(aurora_color(AURORA_COLOR_BACKGROUND), bg, 132));
    fb_fill_rounded_rect(x, y, w, h, 16, bg);
    fb_draw_rect_outline(x, y, w, h,
                         aurora_color(AURORA_COLOR_BORDER_SUBTLE), 1);

    fb_fill_rounded_rect(x + 16, y + 16, 4, 12, 2,
                         aurora_color(AURORA_COLOR_ACCENT));
    shell_text(x + 28, y + 14, "NexOS", AURORA_COLOR_TEXT_PRIMARY, bg);
    shell_text(x + 28 + font_aurora_str_width("NexOS", 14) + 12,
               y + 14, "Desktop", AURORA_COLOR_TEXT_MUTED, bg);

    /* The reference uses one right-aligned status hierarchy rather than
     * separate native memory cards. */
    char clock[6];
    shell_clock(clock);
    const char *net_label = wifi_is_connected() ? "WiFi" :
                            (netif_is_up() ? "Ethernet" : "Offline");
    int net_w = font_aurora_str_width(net_label, 13);
    int pct_w = font_aurora_str_width("94%", 13);
    int clock_w = font_aurora_str_width(clock, 13);
    int bullet_gap = 12;
    int bullet_w = 4;
    int tw = net_w + pct_w + clock_w + bullet_w * 2 + bullet_gap * 4;
    int sx = x + w - 16 - tw;
    uint32_t status_role = wifi_is_connected() ?
                           AURORA_COLOR_TEXT_SECONDARY :
                           AURORA_COLOR_TEXT_MUTED;
    shell_text(sx, y + 14, net_label, status_role, bg);
    sx += net_w + bullet_gap;
    fb_fill_circle(sx + bullet_w / 2, y + 22, 2,
                   aurora_color(AURORA_COLOR_TEXT_MUTED));
    sx += bullet_w + bullet_gap;
    shell_text(sx, y + 14, "94%", status_role, bg);
    sx += pct_w + bullet_gap;
    fb_fill_circle(sx + bullet_w / 2, y + 22, 2,
                   aurora_color(AURORA_COLOR_TEXT_MUTED));
    sx += bullet_w + bullet_gap;
    shell_text(sx, y + 14, clock, status_role, bg);
}

void shell_draw_left_rail(void) {
    if (!fb.initialized) return;
    int x = 18, y = 82, w = 72;
    int h = 650; /* canonical 1440x900 rail height */
    if (h < 240) h = 240;
    if (y + h > (int)fb.height - 78) h = (int)fb.height - 78 - y;
    if (h < 48) return;

    uint32_t bg = aurora_color(AURORA_COLOR_SURFACE);
    fb_fill_rounded_rect(x + 3, y + 4, w, h, 24,
                         shell_mix(aurora_color(AURORA_COLOR_BACKGROUND), bg, 72));
    fb_fill_rounded_rect(x + 1, y + 2, w, h, 24,
                         shell_mix(aurora_color(AURORA_COLOR_BACKGROUND), bg, 132));
    fb_fill_rounded_rect(x, y, w, h, 24, bg);
    fb_draw_rect_outline(x, y, w, h,
                         aurora_color(AURORA_COLOR_BORDER_SUBTLE), 1);

    /* These are navigation affordances backed by existing shell actions.
     * Their visual treatment is shared with launcher/dock app icons. */
    const aurora_icon_id_t icons[] = {
        AURORA_ICON_APPS, AURORA_ICON_FILES, AURORA_ICON_TERMINAL,
        AURORA_ICON_BROWSER, AURORA_ICON_MONITOR, AURORA_ICON_SETTINGS
    };
    const uint32_t colors[] = {
        0x7C8CFF, 0x78C7FF, 0x5ED69A, 0xC5A8FF, 0xFFA86B, 0xAEB8FF
    };
    int iy = y + 16;
    for (int i = 0; i < 6; i++) {
        if (iy + 48 > y + h - 12) break;
        uint32_t state = (i == 0) ? AURORA_STATE_SELECTED : 0;
        aurora_card((aurora_rect_t){x + 12, iy, 48, 48}, state);
        aurora_app_icon_id(x + 36, iy + 24, 16, icons[i], colors[i]);
        iy += 66;
    }
}
