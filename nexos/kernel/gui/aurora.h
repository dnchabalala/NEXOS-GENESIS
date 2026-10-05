/* NexOS — kernel/gui/aurora.h | Native Aurora UI foundation | MIT License */
#pragma once

#include <stdint.h>

/* The palette keeps the legacy theme fields so existing applications remain
 * source-compatible while they migrate to semantic roles. */
typedef struct {
    const char *name;
    uint32_t base, mantle, crust;
    uint32_t surface0, surface1, surface2;
    uint32_t overlay0;
    uint32_t text, subtext;
    uint32_t blue, lavender, mauve;
    uint32_t red, peach, yellow, green;
    uint32_t teal, sky;
    uint32_t dot[5];
} aurora_palette_t;

typedef enum {
    AURORA_COLOR_BACKGROUND = 0,
    AURORA_COLOR_SURFACE,
    AURORA_COLOR_ELEVATED,
    AURORA_COLOR_HOVER,
    AURORA_COLOR_PRESSED,
    AURORA_COLOR_SELECTED,
    AURORA_COLOR_INACTIVE,
    AURORA_COLOR_TEXT_PRIMARY,
    AURORA_COLOR_TEXT_SECONDARY,
    AURORA_COLOR_TEXT_MUTED,
    AURORA_COLOR_TEXT_DISABLED,
    AURORA_COLOR_ACCENT,
    AURORA_COLOR_ACCENT_HOVER,
    AURORA_COLOR_ACCENT_PRESSED,
    AURORA_COLOR_BORDER_SUBTLE,
    AURORA_COLOR_BORDER,
    AURORA_COLOR_BORDER_FOCUSED,
    AURORA_COLOR_SUCCESS,
    AURORA_COLOR_WARNING,
    AURORA_COLOR_DANGER,
    AURORA_COLOR_INFORMATION
} aurora_color_role_t;

typedef enum {
    AURORA_TEXT_CAPTION = 0,
    AURORA_TEXT_BODY,
    AURORA_TEXT_BODY_EMPHASIS,
    AURORA_TEXT_LABEL,
    AURORA_TEXT_SECTION,
    AURORA_TEXT_TITLE,
    AURORA_TEXT_DISPLAY
} aurora_text_role_t;

typedef enum {
    AURORA_BUTTON_SECONDARY = 0,
    AURORA_BUTTON_PRIMARY,
    AURORA_BUTTON_ACCENT,
    AURORA_BUTTON_DANGER
} aurora_button_variant_t;

enum {
    AURORA_STATE_NORMAL   = 0,
    AURORA_STATE_HOVER    = 1u << 0,
    AURORA_STATE_PRESSED  = 1u << 1,
    AURORA_STATE_FOCUSED  = 1u << 2,
    AURORA_STATE_SELECTED = 1u << 3,
    AURORA_STATE_DISABLED = 1u << 4,
    AURORA_STATE_INACTIVE = 1u << 5
};

typedef struct {
    int x, y, w, h;
} aurora_rect_t;

typedef enum {
    AURORA_ICON_APPS = 0,
    AURORA_ICON_FILES,
    AURORA_ICON_TERMINAL,
    AURORA_ICON_BROWSER,
    AURORA_ICON_SETTINGS,
    AURORA_ICON_SYSTEM,
    AURORA_ICON_THEME,
    AURORA_ICON_CALCULATOR,
    AURORA_ICON_CLOCK,
    AURORA_ICON_EDITOR,
    AURORA_ICON_VISUALIZER,
    AURORA_ICON_SNAKE,
    AURORA_ICON_MONITOR,
    AURORA_ICON_WIFI,
    AURORA_ICON_ETHERNET,
    AURORA_ICON_SEARCH,
    AURORA_ICON_POWER,
    AURORA_ICON_RESTART,
    AURORA_ICON_BACK,
    AURORA_ICON_FORWARD,
    AURORA_ICON_RELOAD,
    AURORA_ICON_HOME
} aurora_icon_id_t;

/* Aurora spacing, dimensions, and radii. These are compile-time values so
 * controls do not allocate or construct style objects during painting. */
#define AURORA_SPACE_1 4
#define AURORA_SPACE_2 8
#define AURORA_SPACE_3 12
#define AURORA_SPACE_4 16
#define AURORA_SPACE_5 24
#define AURORA_SPACE_6 32

#define AURORA_RADIUS_SMALL 8
#define AURORA_RADIUS_MEDIUM 12
#define AURORA_RADIUS_LARGE 16
#define AURORA_RADIUS_PANEL 20
#define AURORA_RADIUS_DIALOG 24
#define AURORA_RADIUS_WINDOW 28

#define AURORA_CONTROL_COMPACT_H 28
#define AURORA_CONTROL_H 36
#define AURORA_TITLEBAR_H 54
#define AURORA_TASKBAR_H 66
#define AURORA_PANEL_PAD 16
#define AURORA_DIALOG_PAD 24
#define AURORA_ROW_H 28
#define AURORA_ICON_SM 16
#define AURORA_ICON_MD 24
#define AURORA_ICON_LG 32
#define AURORA_WINDOW_BUTTON_R 6
#define AURORA_WINDOW_BUTTON_GAP 22
#define AURORA_SCROLLBAR_W 8

void aurora_apply_palette(const aurora_palette_t *palette);
const aurora_palette_t *aurora_default_palette(void);
uint32_t aurora_color(aurora_color_role_t role);
const aurora_palette_t *aurora_active_palette(void);

uint32_t aurora_state_color(aurora_color_role_t normal,
                            aurora_color_role_t hover,
                            aurora_color_role_t pressed,
                            uint32_t state);

void aurora_text(int x, int y, const char *text,
                 aurora_text_role_t role, uint32_t background);
void aurora_panel(aurora_rect_t rect, int elevated);
void aurora_card(aurora_rect_t rect, uint32_t state);
void aurora_separator(int x, int y, int w);
void aurora_button(aurora_rect_t rect, const char *label,
                   aurora_button_variant_t variant, uint32_t state);
void aurora_icon_button(aurora_rect_t rect, const char *glyph,
                        uint32_t state);
void aurora_text_field(aurora_rect_t rect, const char *text,
                       uint32_t state, int password);
void aurora_search_field(aurora_rect_t rect, const char *text,
                         uint32_t state);
void aurora_tab(aurora_rect_t rect, const char *label, uint32_t state);
void aurora_badge(aurora_rect_t rect, const char *label,
                  aurora_color_role_t role);
void aurora_progress(aurora_rect_t rect, uint32_t value,
                     aurora_color_role_t role);
void aurora_list_row(aurora_rect_t rect, const char *label,
                     const char *detail, uint32_t state);
void aurora_table_row(aurora_rect_t rect, const char *left,
                      const char *right, uint32_t state);
void aurora_dialog(aurora_rect_t rect, const char *title);
void aurora_toast(aurora_rect_t rect, const char *title,
                  const char *body, uint32_t progress,
                  aurora_color_role_t role);
void aurora_window_chrome(aurora_rect_t rect, const char *title,
                          int focused);
void aurora_app_icon(int cx, int cy, int radius, char glyph,
                     uint32_t color);
void aurora_app_icon_id(int cx, int cy, int radius, aurora_icon_id_t icon,
                        uint32_t color);
void aurora_icon_draw(aurora_rect_t rect, aurora_icon_id_t icon,
                      uint32_t color);
void aurora_signal_indicator(int x, int y, int signal, int connected);
void aurora_memory_indicator(aurora_rect_t rect, uint32_t used_percent);
void aurora_scrollbar(aurora_rect_t rect, uint32_t position,
                      uint32_t visible, uint32_t total);
