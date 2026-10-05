/* NexOS — kernel/gui/desktop.c
 * Aurora desktop background.
 * The shell uses static layered surfaces so an idle desktop never needs
 * periodic composition merely to animate decoration.
 * MIT License */
#include "desktop.h"
#include "aurora.h"
#include "../drivers/fb.h"
#include "../drivers/font.h"

void desktop_set_phase(uint32_t delta_ms) {
    (void)delta_ms;
}

static void desktop_paint_region(int x, int y, int w, int h) {
    int desk_h = (int)fb.height;
    int desk_w = (int)fb.width;
    int x1 = x < 0 ? 0 : x;
    int y1 = y < 0 ? 0 : y;
    int x2 = x + w > desk_w ? desk_w : x + w;
    int y2 = y + h > desk_h ? desk_h : y + h;
    if (x1 >= x2 || y1 >= y2) return;

    /* Opaque base plus a static, low-cost radial treatment.  This is kept
     * per-pixel deliberately: 16px paint tiles made the desktop visibly
     * banded at 1440x900.  The scene is only evaluated when invalidated, so
     * this does not create an idle animation or repaint loop. */
    for (int py = y1; py < y2; py++) {
        for (int px = x1; px < x2; px++) {
            uint32_t c = aurora_color(AURORA_COLOR_BACKGROUND);
            int dx = px - desk_w * 3 / 4;
            int dy = py - desk_h / 4;
            int d = (dx * dx + dy * dy) / 16384;
            int alpha = 34 - d;
            if (alpha > 0) c = fb_blend(0x5366FF, c, (uint8_t)alpha);

            dx = px - desk_w / 4;
            dy = py - desk_h * 3 / 4;
            d = (dx * dx + dy * dy) / 16384;
            alpha = 28 - d;
            if (alpha > 0) c = fb_blend(0xA05CFF, c, (uint8_t)alpha);
            fb_put_pixel(px, py, c);
        }
    }

    int gx0 = (x1 / 32) * 32;
    int gy0 = (y1 / 32) * 32;
    for (int gy = gy0; gy < y2; gy += 32)
        for (int gx = gx0; gx < x2; gx += 32)
            if (gx >= x1 && gy >= y1)
                fb_put_pixel(gx, gy, aurora_color(AURORA_COLOR_BORDER_SUBTLE));
}

void desktop_draw(void) {
    if (!fb.initialized) return;
    desktop_paint_region(0, 0, (int)fb.width, (int)fb.height);
}

void desktop_paint_rect(int x, int y, int w, int h) {
    if (!fb.initialized) return;
    desktop_paint_region(x, y, w, h);
}
