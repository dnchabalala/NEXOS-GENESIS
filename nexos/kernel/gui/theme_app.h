/* NexOS — kernel/gui/theme_app.h | Theme switcher window | MIT License */
#pragma once
#include "wm.h"
#include "aurora.h"

#define THEME_COUNT 5

typedef aurora_palette_t theme_def_t;

extern const theme_def_t g_themes[THEME_COUNT];
extern int g_active_theme;

void     theme_apply(int id);
window_t *theme_create(int x, int y);
