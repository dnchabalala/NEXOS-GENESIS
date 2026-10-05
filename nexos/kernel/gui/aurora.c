/* NexOS — kernel/gui/aurora.c | Native Aurora UI foundation | MIT License */
#include "aurora.h"
#include "../drivers/fb.h"
#include "../drivers/font.h"
#include <stddef.h>

/* Legacy palette symbols remain exported through fb.h. They are now owned by
 * the Aurora theme layer rather than by the framebuffer implementation. */
uint32_t col_base     = 0x1E1E2E;
uint32_t col_mantle   = 0x181825;
uint32_t col_crust    = 0x11111B;
uint32_t col_surface0 = 0x313244;
uint32_t col_surface1 = 0x45475A;
uint32_t col_surface2 = 0x585B70;
uint32_t col_overlay0 = 0x6C7086;
uint32_t col_text     = 0xCDD6F4;
uint32_t col_subtext  = 0xA6ADC8;
uint32_t col_blue     = 0x89B4FA;
uint32_t col_lavender = 0xB4BEFE;
uint32_t col_mauve    = 0xCBA6F7;
uint32_t col_red      = 0xF38BA8;
uint32_t col_peach    = 0xFAB387;
uint32_t col_yellow   = 0xF9E2AF;
uint32_t col_green    = 0xA6E3A1;
uint32_t col_teal     = 0x94E2D5;
uint32_t col_sky      = 0x89DCEB;

static aurora_palette_t active_palette = {
    "Aurora / Catppuccin Mocha",
    0x1E1E2E, 0x181825, 0x11111B,
    0x313244, 0x45475A, 0x585B70, 0x6C7086,
    0xCDD6F4, 0xA6ADC8,
    0x89B4FA, 0xB4BEFE, 0xCBA6F7,
    0xF38BA8, 0xFAB387, 0xF9E2AF, 0xA6E3A1,
    0x94E2D5, 0x89DCEB,
    { 0x1E1E2E, 0x89B4FA, 0xCBA6F7, 0xA6E3A1, 0xF38BA8 }
};

/* Canonical Aurora values are exposed for the later shell/app migration. The
 * current desktop deliberately keeps its active Catppuccin palette until
 * those call sites are migrated. */
static const aurora_palette_t aurora_default = {
    "Aurora Dark",
    0x070A12, 0x090D16, 0x05070C,
    0x0D111B, 0x111623, 0x182033, 0x5E687F,
    0xE7EBF5, 0xA7B0C2,
    0x7C8CFF, 0xAEB8FF, 0xC5A8FF,
    0xFF6868, 0xFFA86B, 0xF4C96B, 0x5ED69A,
    0x5AD6C9, 0x78C7FF,
    { 0x070A12, 0x7C8CFF, 0xC5A8FF, 0x5ED69A, 0xFF6868 }
};

static uint32_t mix(uint32_t a, uint32_t b, uint8_t alpha) {
    return fb_blend(b, a, alpha);
}

void aurora_apply_palette(const aurora_palette_t *p) {
    if (!p) return;
    active_palette = *p;
    col_base = p->base; col_mantle = p->mantle; col_crust = p->crust;
    col_surface0 = p->surface0; col_surface1 = p->surface1;
    col_surface2 = p->surface2; col_overlay0 = p->overlay0;
    col_text = p->text; col_subtext = p->subtext;
    col_blue = p->blue; col_lavender = p->lavender; col_mauve = p->mauve;
    col_red = p->red; col_peach = p->peach; col_yellow = p->yellow;
    col_green = p->green; col_teal = p->teal; col_sky = p->sky;
}

const aurora_palette_t *aurora_default_palette(void) {
    return &aurora_default;
}

const aurora_palette_t *aurora_active_palette(void) {
    return &active_palette;
}

uint32_t aurora_color(aurora_color_role_t role) {
    switch (role) {
        case AURORA_COLOR_BACKGROUND:      return COL_BASE;
        case AURORA_COLOR_SURFACE:         return COL_SURFACE0;
        case AURORA_COLOR_ELEVATED:        return COL_SURFACE1;
        case AURORA_COLOR_HOVER:           return COL_SURFACE2;
        case AURORA_COLOR_PRESSED:         return mix(COL_SURFACE2, COL_BLUE, 80);
        case AURORA_COLOR_SELECTED:        return mix(COL_SURFACE0, COL_BLUE, 48);
        case AURORA_COLOR_INACTIVE:        return COL_CRUST;
        case AURORA_COLOR_TEXT_PRIMARY:    return COL_TEXT;
        case AURORA_COLOR_TEXT_SECONDARY:  return COL_SUBTEXT;
        case AURORA_COLOR_TEXT_MUTED:      return COL_OVERLAY0;
        case AURORA_COLOR_TEXT_DISABLED:   return mix(COL_OVERLAY0, COL_CRUST, 120);
        case AURORA_COLOR_ACCENT:          return COL_BLUE;
        case AURORA_COLOR_ACCENT_HOVER:    return mix(COL_BLUE, COL_LAVENDER, 100);
        case AURORA_COLOR_ACCENT_PRESSED:  return mix(COL_BLUE, COL_MAUVE, 80);
        case AURORA_COLOR_BORDER_SUBTLE:   return COL_SURFACE0;
        case AURORA_COLOR_BORDER:          return COL_SURFACE1;
        case AURORA_COLOR_BORDER_FOCUSED:  return COL_BLUE;
        case AURORA_COLOR_SUCCESS:         return COL_GREEN;
        case AURORA_COLOR_WARNING:         return COL_YELLOW;
        case AURORA_COLOR_DANGER:          return COL_RED;
        case AURORA_COLOR_INFORMATION:     return COL_SKY;
        default:                            return COL_TEXT;
    }
}

uint32_t aurora_state_color(aurora_color_role_t normal,
                            aurora_color_role_t hover,
                            aurora_color_role_t pressed,
                            uint32_t state) {
    if (state & AURORA_STATE_DISABLED) return aurora_color(AURORA_COLOR_INACTIVE);
    if (state & AURORA_STATE_PRESSED) return aurora_color(pressed);
    if (state & AURORA_STATE_HOVER) return aurora_color(hover);
    return aurora_color(normal);
}

void aurora_text(int x, int y, const char *text,
                 aurora_text_role_t role, uint32_t background) {
    aurora_color_role_t color = AURORA_COLOR_TEXT_PRIMARY;
    if (role == AURORA_TEXT_CAPTION || role == AURORA_TEXT_BODY)
        color = AURORA_COLOR_TEXT_SECONDARY;
    else if (role == AURORA_TEXT_LABEL)
        color = AURORA_COLOR_TEXT_PRIMARY;
    else if (role == AURORA_TEXT_SECTION || role == AURORA_TEXT_TITLE ||
             role == AURORA_TEXT_DISPLAY)
        color = AURORA_COLOR_TEXT_PRIMARY;

    int size = 14;
    if (role == AURORA_TEXT_CAPTION) size = 12;
    else if (role == AURORA_TEXT_BODY_EMPHASIS || role == AURORA_TEXT_LABEL) size = 16;
    else if (role == AURORA_TEXT_SECTION) size = 20;
    else if (role == AURORA_TEXT_TITLE) size = 24;
    else if (role == AURORA_TEXT_DISPLAY) size = 32;
    font_aurora_puts(x, y, text, size, aurora_color(color), background);
}

void aurora_panel(aurora_rect_t r, int elevated) {
    uint32_t bg = elevated ? aurora_color(AURORA_COLOR_ELEVATED)
                           : aurora_color(AURORA_COLOR_SURFACE);
    /* Three cheap bands provide the depth of the HTML glass surfaces without
     * a blur kernel or an always-running animation. */
    if (elevated) {
        /* Three restrained bands reproduce the HTML shadow without a blur
         * kernel.  The shell uses this same surface grammar everywhere. */
        fb_fill_rounded_rect(r.x + 4, r.y + 5, r.w, r.h, AURORA_RADIUS_PANEL,
                             mix(aurora_color(AURORA_COLOR_BACKGROUND), bg, 72));
        fb_fill_rounded_rect(r.x + 2, r.y + 3, r.w, r.h, AURORA_RADIUS_PANEL,
                             mix(aurora_color(AURORA_COLOR_BACKGROUND), bg, 116));
        fb_fill_rounded_rect(r.x + 1, r.y + 1, r.w, r.h, AURORA_RADIUS_PANEL,
                             mix(aurora_color(AURORA_COLOR_BACKGROUND), bg, 166));
    }
    fb_fill_rounded_rect(r.x, r.y, r.w, r.h, AURORA_RADIUS_PANEL, bg);
    fb_draw_rect_outline(r.x, r.y, r.w, r.h,
                         aurora_color(AURORA_COLOR_BORDER_SUBTLE), 1);
    fb_fill_rect_blend(r.x + AURORA_RADIUS_PANEL, r.y + 1,
                       r.w - 2 * AURORA_RADIUS_PANEL, 1,
                       aurora_color(AURORA_COLOR_TEXT_PRIMARY), 22);
}

void aurora_card(aurora_rect_t r, uint32_t state) {
    uint32_t bg = aurora_state_color(AURORA_COLOR_SURFACE,
                                     AURORA_COLOR_HOVER,
                                     AURORA_COLOR_PRESSED, state);
    /* HTML cards use the medium 16px radius; the window radius belongs to
     * the containing surface, not to every 48px control well. */
    fb_fill_rounded_rect(r.x, r.y, r.w, r.h, AURORA_RADIUS_LARGE, bg);
    fb_draw_rect_outline(r.x, r.y, r.w, r.h,
                         (state & AURORA_STATE_FOCUSED) ?
                         aurora_color(AURORA_COLOR_BORDER_FOCUSED) :
                         aurora_color(AURORA_COLOR_BORDER_SUBTLE), 1);
    fb_fill_rect_blend(r.x + AURORA_RADIUS_LARGE, r.y + 1,
                       r.w - 2 * AURORA_RADIUS_LARGE, 1,
                       aurora_color(AURORA_COLOR_TEXT_PRIMARY),
                       (state & AURORA_STATE_SELECTED) ? 34 : 16);
}

void aurora_separator(int x, int y, int w) {
    fb_fill_rect(x, y, w, 1, aurora_color(AURORA_COLOR_BORDER_SUBTLE));
}

void aurora_button(aurora_rect_t r, const char *label,
                   aurora_button_variant_t variant, uint32_t state) {
    aurora_color_role_t normal = AURORA_COLOR_ELEVATED;
    aurora_color_role_t hover = AURORA_COLOR_HOVER;
    aurora_color_role_t pressed = AURORA_COLOR_PRESSED;
    aurora_color_role_t fg = AURORA_COLOR_TEXT_PRIMARY;
    if (variant == AURORA_BUTTON_PRIMARY || variant == AURORA_BUTTON_ACCENT) {
        normal = AURORA_COLOR_ACCENT;
        hover = AURORA_COLOR_ACCENT_HOVER;
        pressed = AURORA_COLOR_ACCENT_PRESSED;
        fg = AURORA_COLOR_BACKGROUND;
    } else if (variant == AURORA_BUTTON_DANGER) {
        normal = AURORA_COLOR_DANGER;
        hover = AURORA_COLOR_DANGER;
        pressed = AURORA_COLOR_DANGER;
        fg = AURORA_COLOR_BACKGROUND;
    }
    uint32_t bg = aurora_state_color(normal, hover, pressed, state);
    uint32_t border = (state & AURORA_STATE_FOCUSED) ?
                      aurora_color(AURORA_COLOR_BORDER_FOCUSED) :
                      aurora_color(AURORA_COLOR_BORDER);
    if (state & AURORA_STATE_DISABLED) fg = AURORA_COLOR_TEXT_DISABLED;
    fb_fill_rounded_rect(r.x, r.y, r.w, r.h, AURORA_RADIUS_SMALL, bg);
    fb_draw_rect_outline(r.x, r.y, r.w, r.h, border, 1);
    if (label) {
        int tw = font_aurora_str_width(label, 14);
        font_aurora_puts(r.x + (r.w - tw) / 2, r.y + (r.h - 14) / 2,
                         label, 14, aurora_color(fg), bg);
    }
}

void aurora_icon_button(aurora_rect_t r, const char *glyph, uint32_t state) {
    aurora_button(r, glyph, AURORA_BUTTON_SECONDARY, state);
}

void aurora_text_field(aurora_rect_t r, const char *text,
                       uint32_t state, int password) {
    fb_fill_rounded_rect(r.x, r.y, r.w, r.h, AURORA_RADIUS_SMALL,
                         aurora_color(AURORA_COLOR_INACTIVE));
    fb_draw_rect_outline(r.x, r.y, r.w, r.h,
                         (state & AURORA_STATE_FOCUSED) ?
                         aurora_color(AURORA_COLOR_BORDER_FOCUSED) :
                         aurora_color(AURORA_COLOR_BORDER), 1);
    if (text) {
        char masked[128];
        const char *shown = text;
        if (password) {
            int i = 0;
            while (text[i] && i < (int)sizeof(masked) - 1) {
                masked[i++] = '*';
            }
            masked[i] = 0;
            shown = masked;
        }
        font_aurora_puts(r.x + AURORA_SPACE_2, r.y + (r.h - 14) / 2,
                         shown, 14, aurora_color(AURORA_COLOR_TEXT_PRIMARY),
                         aurora_color(AURORA_COLOR_INACTIVE));
    }
}

void aurora_search_field(aurora_rect_t r, const char *text, uint32_t state) {
    aurora_text_field(r, text, state, 0);
}

void aurora_tab(aurora_rect_t r, const char *label, uint32_t state) {
    uint32_t bg = (state & AURORA_STATE_SELECTED) ?
                  aurora_color(AURORA_COLOR_SURFACE) :
                  aurora_color(AURORA_COLOR_INACTIVE);
    fb_fill_rect(r.x, r.y, r.w, r.h, bg);
    if (state & AURORA_STATE_SELECTED)
        fb_fill_rect(r.x, r.y + r.h - 2, r.w, 2,
                     aurora_color(AURORA_COLOR_ACCENT));
    if (label) {
        int tw = font_aurora_str_width(label, 14);
        font_aurora_puts(r.x + (r.w - tw) / 2, r.y + (r.h - 14) / 2,
                         label, 14, (state & AURORA_STATE_DISABLED) ?
                         aurora_color(AURORA_COLOR_TEXT_DISABLED) :
                         aurora_color((state & AURORA_STATE_SELECTED) ?
                                      AURORA_COLOR_ACCENT : AURORA_COLOR_TEXT_SECONDARY),
                         bg);
    }
}

void aurora_badge(aurora_rect_t r, const char *label,
                  aurora_color_role_t role) {
    uint32_t c = aurora_color(role);
    uint32_t bg = mix(aurora_color(AURORA_COLOR_BACKGROUND), c, 64);
    fb_fill_rounded_rect(r.x, r.y, r.w, r.h, r.h / 2, bg);
    fb_draw_rect_outline(r.x, r.y, r.w, r.h, c, 1);
    if (label)
        font_aurora_puts(r.x + AURORA_SPACE_2, r.y + (r.h - 14) / 2,
                         label, 14, c, bg);
}

void aurora_progress(aurora_rect_t r, uint32_t value,
                     aurora_color_role_t role) {
    if (value > 100) value = 100;
    fb_fill_rounded_rect(r.x, r.y, r.w, r.h, r.h / 2,
                         aurora_color(AURORA_COLOR_INACTIVE));
    int fill = (int)((uint64_t)r.w * value / 100u);
    if (fill > 0)
        fb_fill_rounded_rect(r.x, r.y, fill, r.h, r.h / 2,
                             aurora_color(role));
}

void aurora_list_row(aurora_rect_t r, const char *label,
                     const char *detail, uint32_t state) {
    aurora_card((aurora_rect_t){r.x, r.y, r.w, r.h}, state);
    if (label)
        font_aurora_puts(r.x + AURORA_SPACE_2, r.y + (r.h - 14) / 2,
                         label, 14, aurora_color(AURORA_COLOR_TEXT_PRIMARY),
                         aurora_color((state & AURORA_STATE_SELECTED) ?
                                      AURORA_COLOR_SELECTED : AURORA_COLOR_SURFACE));
    if (detail) {
        int tw = font_aurora_str_width(detail, 14);
        font_aurora_puts(r.x + r.w - tw - AURORA_SPACE_2,
                         r.y + (r.h - 14) / 2, detail, 14,
                         aurora_color(AURORA_COLOR_TEXT_SECONDARY),
                         aurora_color((state & AURORA_STATE_SELECTED) ?
                                      AURORA_COLOR_SELECTED : AURORA_COLOR_SURFACE));
    }
}

void aurora_table_row(aurora_rect_t r, const char *left,
                      const char *right, uint32_t state) {
    aurora_list_row(r, left, right, state);
}

void aurora_dialog(aurora_rect_t r, const char *title) {
    aurora_panel(r, 1);
    if (title) aurora_text(r.x + AURORA_DIALOG_PAD,
                           r.y + AURORA_DIALOG_PAD, title,
                           AURORA_TEXT_SECTION,
                           aurora_color(AURORA_COLOR_ELEVATED));
}

void aurora_toast(aurora_rect_t r, const char *title,
                  const char *body, uint32_t progress,
                  aurora_color_role_t role) {
    aurora_panel(r, 1);
    fb_fill_rect(r.x, r.y + AURORA_SPACE_2, 3,
                 r.h - AURORA_SPACE_4, aurora_color(role));
    if (title) aurora_text(r.x + AURORA_SPACE_3, r.y + AURORA_SPACE_2,
                           title, AURORA_TEXT_BODY_EMPHASIS,
                           aurora_color(AURORA_COLOR_ELEVATED));
    if (body) font_aurora_puts(r.x + AURORA_SPACE_3, r.y + AURORA_SPACE_3 + 20,
                               body, 12, aurora_color(AURORA_COLOR_TEXT_SECONDARY),
                               aurora_color(AURORA_COLOR_ELEVATED));
    if (progress <= 100)
        aurora_progress((aurora_rect_t){r.x + AURORA_SPACE_3,
                         r.y + r.h - AURORA_SPACE_3 - 3,
                         r.w - AURORA_SPACE_4, 3}, progress, role);
}

void aurora_window_chrome(aurora_rect_t r, const char *title, int focused) {
    uint32_t bg = focused ? aurora_color(AURORA_COLOR_ELEVATED)
                          : aurora_color(AURORA_COLOR_INACTIVE);
    fb_fill_rounded_rect(r.x + 4, r.y + 5, r.w, r.h, AURORA_RADIUS_WINDOW,
                         mix(aurora_color(AURORA_COLOR_BACKGROUND), bg, 70));
    fb_fill_rounded_rect(r.x + 2, r.y + 3, r.w, r.h, AURORA_RADIUS_WINDOW,
                         mix(aurora_color(AURORA_COLOR_BACKGROUND), bg, 122));
    fb_fill_rounded_rect(r.x, r.y, r.w, r.h, AURORA_RADIUS_WINDOW, bg);
    fb_fill_rect_blend(r.x + AURORA_RADIUS_WINDOW, r.y + 1,
                       r.w - 2 * AURORA_RADIUS_WINDOW, 1,
                       aurora_color(AURORA_COLOR_TEXT_PRIMARY), 24);
    fb_fill_rect(r.x, r.y + r.h - 1, r.w, 1,
                 focused ? aurora_color(AURORA_COLOR_ACCENT) :
                           aurora_color(AURORA_COLOR_BORDER));
    int by = r.y + r.h / 2;
    /* The reference places the traffic lights on the leading edge.  WM hit
     * testing uses the same visual side, while retaining generous circles. */
    int bx = r.x + AURORA_WINDOW_BUTTON_GAP;
    fb_fill_circle(bx, by, AURORA_WINDOW_BUTTON_R,
                   aurora_color(AURORA_COLOR_DANGER));
    fb_fill_circle(bx + AURORA_WINDOW_BUTTON_GAP, by,
                   AURORA_WINDOW_BUTTON_R, aurora_color(AURORA_COLOR_WARNING));
    fb_fill_circle(bx + AURORA_WINDOW_BUTTON_GAP * 2, by,
                   AURORA_WINDOW_BUTTON_R,
                   aurora_color(AURORA_COLOR_SUCCESS));
    if (title) {
        int tw = font_aurora_str_width(title, 14);
        font_aurora_puts(r.x + (r.w - tw) / 2, r.y + (r.h - 14) / 2,
                         title, 14, aurora_color(AURORA_COLOR_TEXT_PRIMARY), bg);
    }
}

void aurora_app_icon(int cx, int cy, int radius, char glyph, uint32_t color) {
    fb_fill_circle(cx, cy, radius, color);
    fb_fill_circle(cx - radius / 4, cy - radius / 4,
                   radius > 8 ? radius - 8 : radius, mix(0xFFFFFF, color, 100));
    char text[2] = { glyph, 0 };
    font_puts(cx - 4, cy - 8, text, aurora_color(AURORA_COLOR_BACKGROUND), color);
}

static void icon_line(int cx, int cy, int s, int x0, int y0, int x1, int y1,
                      uint32_t color) {
    fb_draw_line(cx + x0 * s / 24, cy + y0 * s / 24,
                 cx + x1 * s / 24, cy + y1 * s / 24, color);
}

void aurora_icon_draw(aurora_rect_t r, aurora_icon_id_t icon, uint32_t color) {
    int cx = r.x + r.w / 2, cy = r.y + r.h / 2;
    int s = r.w < r.h ? r.w : r.h;
    if (s > 24) s = 24;
    if (s < 8) return;
    int l = s / 2;
    switch (icon) {
        case AURORA_ICON_APPS:
            fb_fill_circle(cx - l/2, cy - l/2, 2, color);
            fb_fill_circle(cx + l/2, cy - l/2, 2, color);
            fb_fill_circle(cx - l/2, cy + l/2, 2, color);
            fb_fill_circle(cx + l/2, cy + l/2, 2, color); break;
        case AURORA_ICON_FILES:
            fb_fill_rounded_rect(cx-l, cy-l/2, s, s/2+3, 3, color);
            fb_fill_rect(cx-l+3, cy-l/2-3, s/3, 4, color); break;
        case AURORA_ICON_TERMINAL:
            fb_draw_rect_outline(cx-l, cy-l*3/4, s, s*3/2, color, 3);
            icon_line(cx, cy, s, -6, -3, -2, 1, color);
            icon_line(cx, cy, s, -2, 1, -6, 5, color);
            fb_fill_rect(cx, cy + s/4, s/3, 2, color); break;
        case AURORA_ICON_BROWSER:
            fb_draw_circle(cx, cy, l, color);
            icon_line(cx, cy, s, -l, 0, l, 0, color);
            icon_line(cx, cy, s, 0, -l, 0, l, color); break;
        case AURORA_ICON_SETTINGS:
            fb_draw_circle(cx, cy, l-2, color);
            fb_fill_circle(cx, cy, l/3, color);
            for (int i = 0; i < 8; i++) {
                int dx = (i & 1) ? l : 0, dy = (i & 2) ? l : 0;
                if (i & 4) dx = -dx;
                if (i == 0 || i == 1) fb_fill_rect(cx + dx - 2, cy + dy - 2, 4, 4, color);
            }
            break;
        case AURORA_ICON_SYSTEM:
        case AURORA_ICON_MONITOR:
            fb_draw_rect_outline(cx-l, cy-l*3/4, s, s, color, 2);
            icon_line(cx, cy, s, -5, l, 5, l, color);
            icon_line(cx, cy, s, 0, l, 0, l+4, color); break;
        case AURORA_ICON_THEME:
            fb_fill_circle(cx, cy, l, color);
            fb_fill_circle(cx + l/3, cy - l/3, l/3, aurora_color(AURORA_COLOR_SURFACE)); break;
        case AURORA_ICON_CALCULATOR:
            fb_draw_rect_outline(cx-l, cy-l, s, s*2, color, 2);
            for (int row=0; row<2; row++) for (int col=0; col<2; col++)
                fb_fill_rect(cx-l/2+col*l/2, cy+row*l/2, 3, 3, color);
            break;
        case AURORA_ICON_CLOCK:
            fb_draw_circle(cx, cy, l, color);
            icon_line(cx, cy, s, 0, 0, 0, -l/2, color);
            icon_line(cx, cy, s, 0, 0, l/2, l/3, color); break;
        case AURORA_ICON_EDITOR:
            icon_line(cx, cy, s, -l/2, l/2, l/2, -l/2, color);
            icon_line(cx, cy, s, -l/2+2, l/2-4, l/2+4, -l/2+2, color);
            break;
        case AURORA_ICON_VISUALIZER:
            for (int i=0;i<5;i++) fb_fill_rect(cx-l+3+i*4, cy-(i%3)*4,
                                                  2, l+(i%3)*4, color); break;
        case AURORA_ICON_SNAKE:
            fb_draw_circle(cx-l/3, cy, l/2, color);
            icon_line(cx, cy, s, -l/3, 0, l/3, 0, color);
            fb_fill_circle(cx+l/2, cy, 2, color); break;
        case AURORA_ICON_WIFI:
            icon_line(cx, cy, s, -l, -l/3, 0, l/2, color);
            icon_line(cx, cy, s, l, -l/3, 0, l/2, color);
            fb_fill_circle(cx, cy+l/2, 2, color); break;
        case AURORA_ICON_ETHERNET:
            fb_draw_rect_outline(cx-l, cy-l/2, s, s, color, 2);
            for (int i=-1;i<=1;i++)
                fb_fill_rect(cx+i*5-1, cy+l/2, 3, 4, color);
            break;
        case AURORA_ICON_SEARCH:
            fb_draw_circle(cx-2, cy-2, l-3, color);
            icon_line(cx, cy, s, 3, 3, l, l, color); break;
        case AURORA_ICON_POWER:
            fb_draw_circle(cx, cy+2, l-3, color);
            fb_fill_rect(cx-1, cy-l, 3, l+5, aurora_color(AURORA_COLOR_SURFACE)); break;
        case AURORA_ICON_RESTART:
            fb_draw_circle(cx, cy, l-3, color);
            icon_line(cx, cy, s, 0, -l, 5, -l/2, color); break;
        case AURORA_ICON_BACK:
            icon_line(cx, cy, s, l, 0, -l/2, 0, color);
            icon_line(cx, cy, s, -l/2, 0, 0, -l/2, color);
            icon_line(cx, cy, s, -l/2, 0, 0, l/2, color); break;
        case AURORA_ICON_FORWARD:
            icon_line(cx, cy, s, -l, 0, l/2, 0, color);
            icon_line(cx, cy, s, l/2, 0, 0, -l/2, color);
            icon_line(cx, cy, s, l/2, 0, 0, l/2, color); break;
        case AURORA_ICON_RELOAD:
            fb_draw_circle(cx, cy, l-3, color);
            icon_line(cx, cy, s, l-3, -2, l-3, -l/2, color); break;
        case AURORA_ICON_HOME:
            icon_line(cx, cy, s, -l, 0, 0, -l, color);
            icon_line(cx, cy, s, 0, -l, l, 0, color);
            fb_draw_rect_outline(cx-l*2/3, cy, s*2/3, l*2/3, color, 2); break;
        default: break;
    }
}

void aurora_app_icon_id(int cx, int cy, int radius, aurora_icon_id_t icon,
                        uint32_t color) {
    int size = radius * 2;
    /* A shared dark icon well keeps rail, dock and launcher icons coherent;
     * the semantic tint belongs to the mark, not the whole card. */
    uint32_t well = mix(aurora_color(AURORA_COLOR_BACKGROUND), color, 42);
    uint32_t rim = mix(aurora_color(AURORA_COLOR_BORDER), color, 72);
    fb_fill_rounded_rect(cx - radius, cy - radius, size, size,
                         radius > 12 ? 16 : 8, well);
    fb_draw_rect_outline(cx - radius, cy - radius, size, size, rim, 1);
    fb_fill_rect_blend(cx - radius + 2, cy - radius + 2, size - 4, 1,
                       aurora_color(AURORA_COLOR_TEXT_PRIMARY), 24);
    aurora_icon_draw((aurora_rect_t){cx-radius, cy-radius, size, size},
                     icon, color);
}

void aurora_signal_indicator(int x, int y, int signal, int connected) {
    int heights[3] = { 4, 8, 12 };
    uint32_t hi = signal > 70 ? aurora_color(AURORA_COLOR_SUCCESS) :
                  signal > 40 ? aurora_color(AURORA_COLOR_WARNING) :
                  aurora_color(AURORA_COLOR_DANGER);
    for (int i = 0; i < 3; i++) {
        uint32_t c = connected && signal >= (i == 0 ? 20 : i == 1 ? 40 : 70)
                   ? hi : aurora_color(AURORA_COLOR_INACTIVE);
        fb_fill_rect(x + i * 5, y + 12 - heights[i], 4, heights[i], c);
    }
}

void aurora_memory_indicator(aurora_rect_t r, uint32_t used_percent) {
    aurora_progress(r, used_percent,
                    used_percent > 85 ? AURORA_COLOR_DANGER :
                    used_percent > 60 ? AURORA_COLOR_WARNING :
                    AURORA_COLOR_ACCENT);
}

void aurora_scrollbar(aurora_rect_t r, uint32_t position,
                      uint32_t visible, uint32_t total) {
    fb_fill_rounded_rect(r.x, r.y, r.w, r.h, r.w / 2,
                         aurora_color(AURORA_COLOR_INACTIVE));
    if (total == 0 || visible >= total) return;
    uint32_t thumb_h = (uint32_t)r.h * visible / total;
    if (thumb_h < AURORA_SPACE_6) thumb_h = AURORA_SPACE_6;
    uint32_t max_pos = total - visible;
    uint32_t travel = (uint32_t)r.h - thumb_h;
    int ty = r.y + (max_pos ? (int)(travel * position / max_pos) : 0);
    fb_fill_rounded_rect(r.x, ty, r.w, (int)thumb_h,
                         r.w / 2, aurora_color(AURORA_COLOR_ACCENT));
}
