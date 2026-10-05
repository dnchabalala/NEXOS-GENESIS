/* NexOS — kernel/drivers/fb.c | Linear framebuffer driver | MIT License */
#include "fb.h"
#include "../mm/heap.h"
#include <stdint.h>

framebuffer_t fb = {0};

#define FB_GUARD_BYTES 16u
#define FB_GUARD_VALUE 0xA5C3F17Eu

static int clip_x0;
static int clip_y0;
static int clip_x1;
static int clip_y1;
static int clip_radius;
static int clip_shape_x;
static int clip_shape_y;
static int clip_shape_w;
static int clip_shape_h;

static int fb_clip_shape_contains(int x, int y) {
    if (clip_radius <= 0) return 1;
    int r = clip_radius;
    int left = clip_shape_x, top = clip_shape_y;
    int right = left + clip_shape_w - 1, bottom = top + clip_shape_h - 1;
    if ((x >= left + r && x <= right - r) ||
        (y >= top + r && y <= bottom - r)) return 1;
    int cx = x < left + r ? left + r : right - r;
    int cy = y < top + r ? top + r : bottom - r;
    int dx = x - cx, dy = y - cy;
    return dx * dx + dy * dy <= r * r;
}

static uint32_t *fb_target(void) {
    return fb.draw_addr != NULL ? fb.draw_addr : fb.addr;
}

int fb_scene_dirty = 1;

void fb_init(uint64_t addr, uint32_t w, uint32_t h,
             uint32_t pitch, uint8_t bpp) {
    fb.addr        = (uint32_t *)(uintptr_t)addr;
    fb.draw_addr   = NULL;
    fb.width       = w;
    fb.height      = h;
    fb.pitch       = pitch;
    fb.bpp         = bpp;
    fb.initialized = 1;
    fb.draw_alloc_base = NULL;
    fb.draw_alloc_bytes = 0;
    fb_reset_clip();
}

int fb_enable_backbuffer(void) {
    if (!fb.initialized || fb.bpp != 32 || fb.draw_addr != NULL)
        return fb.draw_addr != NULL;

    size_t bytes = (size_t)fb.pitch * (size_t)fb.height;
    uint8_t *raw = (uint8_t *)kmalloc(bytes + FB_GUARD_BYTES * 2u);
    if (raw == NULL) return 0;
    uint32_t *before = (uint32_t *)raw;
    uint32_t *after = (uint32_t *)(raw + FB_GUARD_BYTES + bytes);
    for (unsigned i = 0; i < FB_GUARD_BYTES / sizeof(uint32_t); i++) {
        before[i] = FB_GUARD_VALUE;
        after[i] = FB_GUARD_VALUE;
    }
    uint32_t *buffer = (uint32_t *)(raw + FB_GUARD_BYTES);
    for (uint32_t y = 0; y < fb.height; y++) {
        uint8_t *dst = (uint8_t *)buffer + (size_t)y * fb.pitch;
        const uint8_t *src = (const uint8_t *)fb.addr + (size_t)y * fb.pitch;
        for (uint32_t x = 0; x < fb.pitch; x++) dst[x] = src[x];
    }
    fb.draw_addr = buffer;
    fb.draw_alloc_base = raw;
    fb.draw_alloc_bytes = (uint32_t)bytes;
    return 1;
}

int fb_backbuffer_guards_ok(void) {
    if (!fb.draw_addr || !fb.draw_alloc_base) return 1;
    uint8_t *raw = (uint8_t *)fb.draw_alloc_base;
    uint32_t *before = (uint32_t *)raw;
    uint32_t *after = (uint32_t *)(raw + FB_GUARD_BYTES + fb.draw_alloc_bytes);
    for (unsigned i = 0; i < FB_GUARD_BYTES / sizeof(uint32_t); i++) {
        if (before[i] != FB_GUARD_VALUE || after[i] != FB_GUARD_VALUE)
            return 0;
    }
    return 1;
}

void fb_commit(void) {
    if (!fb.initialized || fb.draw_addr == NULL) return;
    for (uint32_t y = 0; y < fb.height; y++) {
        uint8_t *dst = (uint8_t *)fb.addr + (size_t)y * fb.pitch;
        const uint8_t *src = (const uint8_t *)fb.draw_addr + (size_t)y * fb.pitch;
        for (uint32_t x = 0; x < fb.pitch; x++) dst[x] = src[x];
    }
}

void fb_commit_rect(int x, int y, int w, int h) {
    if (!fb.initialized || fb.draw_addr == NULL || w <= 0 || h <= 0)
        return;

    int x1 = x + w;
    int y1 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > (int)fb.width) x1 = (int)fb.width;
    if (y1 > (int)fb.height) y1 = (int)fb.height;
    if (x >= x1 || y >= y1) return;

    for (int row = y; row < y1; row++) {
        uint32_t *dst = (uint32_t *)((uint8_t *)fb.addr +
                                     (size_t)row * fb.pitch) + x;
        const uint32_t *src = (const uint32_t *)((const uint8_t *)fb.draw_addr +
                                                 (size_t)row * fb.pitch) + x;
        for (int col = x; col < x1; col++)
            dst[col - x] = src[col - x];
    }
}

void fb_present_pixel(int x, int y, uint32_t color) {
    if (!fb.initialized || (unsigned)x >= fb.width ||
        (unsigned)y >= fb.height)
        return;
    uint32_t *row = (uint32_t *)((uint8_t *)fb.addr +
                                 (size_t)y * fb.pitch);
    row[x] = color;
}

uint32_t *fb_draw_addr(void) { return fb_target(); }

void fb_set_clip(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) {
        clip_x0 = clip_x1 = 0;
        clip_y0 = clip_y1 = 0;
        return;
    }
    int x1 = x + w;
    int y1 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > (int)fb.width) x1 = (int)fb.width;
    if (y1 > (int)fb.height) y1 = (int)fb.height;
    clip_x0 = x; clip_y0 = y; clip_x1 = x1; clip_y1 = y1;
    clip_radius = 0;
}

void fb_set_clip_rounded(int x, int y, int w, int h, int radius) {
    fb_set_clip(x, y, w, h);
    if (clip_x1 <= clip_x0 || clip_y1 <= clip_y0) return;
    clip_shape_x = x;
    clip_shape_y = y;
    clip_shape_w = w;
    clip_shape_h = h;
    clip_radius = radius < 0 ? 0 : radius;
    if (clip_radius * 2 > w) clip_radius = w / 2;
    if (clip_radius * 2 > h) clip_radius = h / 2;
}

void fb_reset_clip(void) {
    clip_x0 = 0; clip_y0 = 0;
    clip_x1 = (int)fb.width; clip_y1 = (int)fb.height;
    clip_radius = 0;
}

int fb_clip_contains(int x, int y) {
    return x >= clip_x0 && x < clip_x1 && y >= clip_y0 && y < clip_y1 &&
           fb_clip_shape_contains(x, y);
}

void fb_put_pixel(int x, int y, uint32_t color) {
    if ((unsigned)x >= fb.width || (unsigned)y >= fb.height ||
        !fb_clip_contains(x, y)) return;
    uint32_t *row = (uint32_t *)((uint8_t *)fb_target() + (uint32_t)y * fb.pitch);
    row[x] = color;
}

uint32_t fb_get_pixel(int x, int y) {
    if ((unsigned)x >= fb.width || (unsigned)y >= fb.height ||
        !fb_clip_contains(x, y)) return 0;
    const uint32_t *row = (const uint32_t *)((uint8_t *)fb_target() + (uint32_t)y * fb.pitch);
    return row[x];
}

/* ── fb_fill_rect ─────────────────────────────────────────────────────────
 * Optimisation: write two 32-bit pixels per 64-bit store.
 * On x86_64 a 64-bit MOQ has the same latency as a 32-bit MOV, so packing
 * two pixels doubles effective write throughput.                           */
void fb_fill_rect(int x, int y, int w, int h, uint32_t c) {
    if (w <= 0 || h <= 0) return;
    int x1 = x + w, y1 = y + h;
    if (x1 > (int)fb.width)  x1 = (int)fb.width;
    if (y1 > (int)fb.height) y1 = (int)fb.height;
    if (x < clip_x0) x = clip_x0;
    if (y < clip_y0) y = clip_y0;
    if (x1 > clip_x1) x1 = clip_x1;
    if (y1 > clip_y1) y1 = clip_y1;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 <= x || y1 <= y) return;

    /* Rounded window masks are uncommon and bounded to a single window
     * paint.  Use the pixel-safe path there so client content cannot escape
     * the authoritative outer silhouette. */
    if (clip_radius > 0) {
        for (int row = y; row < y1; row++)
            for (int col = x; col < x1; col++)
                if (fb_clip_shape_contains(col, row))
                    fb_put_pixel(col, row, c);
        return;
    }

    /* Pack two pixels into one 64-bit word for wide-store loop */
    uint64_t c2 = ((uint64_t)c << 32) | c;

    for (int row = y; row < y1; row++) {
        uint32_t *p = (uint32_t *)((uint8_t *)fb_target() + (uint32_t)row * fb.pitch) + x;
        int n = x1 - x;

        /* Align to 8 bytes: write the first pixel separately if ptr is
         * only 4-byte aligned (handles odd x values)                       */
        if (((uintptr_t)p & 4) && n > 0) { *p++ = c; n--; }

        /* Main loop: two pixels per 64-bit store */
        uint64_t *p8 = (uint64_t *)p;
        while (n >= 2) { *p8++ = c2; n -= 2; }

        /* Trailing single pixel */
        if (n) *(uint32_t *)p8 = c;
    }
}

void fb_draw_rect_outline(int x, int y, int w, int h,
                          uint32_t color, int t) {
    fb_fill_rect(x,         y,         w, t, color);
    fb_fill_rect(x,         y + h - t, w, t, color);
    fb_fill_rect(x,         y,         t, h, color);
    fb_fill_rect(x + w - t, y,         t, h, color);
}

void fb_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = x1 - x0; if (dx < 0) dx = -dx;
    int dy = y1 - y0; if (dy < 0) dy = -dy;
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        fb_put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

void fb_fill_rounded_rect(int x, int y, int w, int h, int r, uint32_t c) {
    if (r <= 0 || w <= 0 || h <= 0) { fb_fill_rect(x, y, w, h, c); return; }
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    fb_fill_rect(x + r, y,         w - 2 * r, h, c);
    fb_fill_rect(x,     y + r,     r,          h - 2 * r, c);
    fb_fill_rect(x + w - r, y + r, r,          h - 2 * r, c);
    int cx[4] = { x + r,       x + w - 1 - r, x + r,         x + w - 1 - r };
    int cy[4] = { y + r,       y + r,          y + h - 1 - r, y + h - 1 - r };
    for (int q = 0; q < 4; q++) {
        int f = 1 - r, ddx = 0, ddy = -2 * r, px = 0, py = r;
        while (px <= py) {
            int qx, qy;
            switch (q) {
                case 0: qx = -px; qy = -py; break;
                case 1: qx =  px; qy = -py; break;
                case 2: qx = -px; qy =  py; break;
                default: qx =  px; qy =  py; break;
            }
            fb_put_pixel(cx[q] + qx, cy[q] + qy, c);
            switch (q) {
                case 0: qx = -py; qy = -px; break;
                case 1: qx =  py; qy = -px; break;
                case 2: qx = -py; qy =  px; break;
                default: qx =  py; qy =  px; break;
            }
            fb_put_pixel(cx[q] + qx, cy[q] + qy, c);
            if (f >= 0) { py--; ddy += 2; f += ddy; }
            px++; ddx += 2; f += ddx + 1;
        }
    }
    for (int ry = 0; ry <= r; ry++) {
        int half = 0;
        while ((half + 1) * (half + 1) + ry * ry <= r * r) half++;
        fb_fill_rect(x + r - half,       y + r - ry,           half + 1, 1, c);
        fb_fill_rect(x + w - 1 - r,      y + r - ry,           half + 1, 1, c);
        fb_fill_rect(x + r - half,       y + h - 1 - r + ry,   half + 1, 1, c);
        fb_fill_rect(x + w - 1 - r,      y + h - 1 - r + ry,   half + 1, 1, c);
    }
}

void fb_draw_circle(int cx, int cy, int r, uint32_t color) {
    int x = 0, y = r, f = 1 - r, ddx = 0, ddy = -2 * r;
    fb_put_pixel(cx, cy + r, color);
    fb_put_pixel(cx, cy - r, color);
    fb_put_pixel(cx + r, cy, color);
    fb_put_pixel(cx - r, cy, color);
    while (x < y) {
        if (f >= 0) { y--; ddy += 2; f += ddy; }
        x++; ddx += 2; f += ddx + 1;
        fb_put_pixel(cx + x, cy + y, color);
        fb_put_pixel(cx - x, cy + y, color);
        fb_put_pixel(cx + x, cy - y, color);
        fb_put_pixel(cx - x, cy - y, color);
        fb_put_pixel(cx + y, cy + x, color);
        fb_put_pixel(cx - y, cy + x, color);
        fb_put_pixel(cx + y, cy - x, color);
        fb_put_pixel(cx - y, cy - x, color);
    }
}

void fb_fill_circle(int cx, int cy, int r, uint32_t color) {
    for (int y = -r; y <= r; y++) {
        int xw = 0;
        while (xw * xw + y * y <= r * r) xw++;
        fb_fill_rect(cx - xw + 1, cy + y, xw * 2 - 1, 1, color);
    }
}

void fb_clear(uint32_t color) {
    fb_fill_rect(0, 0, (int)fb.width, (int)fb.height, color);
}

/* ── fb_scroll_up ─────────────────────────────────────────────────────────
 * Optimisation: copy 8 bytes (two pixels) per iteration instead of 1 byte.
 * The framebuffer is always at least 8-byte aligned at row boundaries.    */
void fb_scroll_up(int pixels, uint32_t bg_color) {
    if (pixels <= 0 || (uint32_t)pixels >= fb.height) return;
    uint8_t *dst = (uint8_t *)fb_target();
    uint8_t *src = dst + (uint32_t)pixels * fb.pitch;
    uint32_t bytes = ((uint32_t)fb.height - (uint32_t)pixels) * fb.pitch;

    /* 8-byte word copy — pitch is always a multiple of 4 for 32 bpp */
    uint64_t *d64 = (uint64_t *)dst;
    const uint64_t *s64 = (const uint64_t *)src;
    uint32_t words = bytes / 8;
    for (uint32_t i = 0; i < words; i++) d64[i] = s64[i];
    /* any trailing bytes (rare: only if pitch is not 8-byte aligned) */
    uint32_t rem = bytes & 7;
    uint8_t *dr = (uint8_t *)(d64 + words), *sr = (uint8_t *)(s64 + words);
    for (uint32_t i = 0; i < rem; i++) dr[i] = sr[i];

    fb_fill_rect(0, (int)fb.height - pixels, (int)fb.width, pixels, bg_color);
}

/* ── fb_blend ──────────────────────────────────────────────────────────────
 * Optimisation: replace two integer divisions by 255 with right-shifts by 8
 * (equivalent to dividing by 256 — error ≤1 per channel, imperceptible).
 * Saves 6 division instructions (~20 cycles each → ~1 cycle shift each).  */
uint32_t fb_blend(uint32_t fg, uint32_t bg, uint8_t alpha) {
    uint32_t inv = (uint32_t)(255u - alpha);
    uint8_t r = (uint8_t)((((fg >> 16) & 0xFFu) * alpha +
                            ((bg >> 16) & 0xFFu) * inv) >> 8);
    uint8_t g = (uint8_t)((((fg >>  8) & 0xFFu) * alpha +
                            ((bg >>  8) & 0xFFu) * inv) >> 8);
    uint8_t b = (uint8_t)((( fg        & 0xFFu) * alpha +
                            ( bg        & 0xFFu) * inv) >> 8);
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

void fb_copy_rect(int sx, int sy, int dx, int dy, int w, int h) {
    if (w <= 0 || h <= 0) return;
    /* Clip the complete source/destination rectangle before forming row
     * pointers.  The old code formed pointers before checking negative
     * coordinates, which is undefined behavior and can corrupt the
     * framebuffer when a window animation crosses an edge. */
    if (sx < 0) { int n = -sx; sx += n; dx += n; w -= n; }
    if (dx < 0) { int n = -dx; sx += n; dx += n; w -= n; }
    if (sy < 0) { int n = -sy; sy += n; dy += n; h -= n; }
    if (dy < 0) { int n = -dy; sy += n; dy += n; h -= n; }
    if (sx + w > (int)fb.width)  w = (int)fb.width - sx;
    if (dx + w > (int)fb.width)  w = (int)fb.width - dx;
    if (sy + h > (int)fb.height) h = (int)fb.height - sy;
    if (dy + h > (int)fb.height) h = (int)fb.height - dy;
    if (dx < clip_x0) { int n = clip_x0 - dx; sx += n; dx += n; w -= n; }
    if (dy < clip_y0) { int n = clip_y0 - dy; sy += n; dy += n; h -= n; }
    if (dx + w > clip_x1) w = clip_x1 - dx;
    if (dy + h > clip_y1) h = clip_y1 - dy;
    if (w <= 0 || h <= 0) return;

    for (int row = 0; row < h; row++) {
        int srow = sy + row, drow = dy + row;
        uint32_t *s = (uint32_t *)((uint8_t *)fb_target() +
                                   (uint32_t)srow * fb.pitch) + sx;
        uint32_t *d = (uint32_t *)((uint8_t *)fb_target() +
                                   (uint32_t)drow * fb.pitch) + dx;
        for (int col = 0; col < w; col++) d[col] = s[col];
    }
}

/* ── fb_fill_rect_blend ───────────────────────────────────────────────────
 * Optimisation:
 *   1. Pre-compute fg_r/g/b * alpha once outside the loops (saves 3 muls/pixel).
 *   2. Use >>8 shift instead of /255 (saves 3 divisions/pixel).            */
void fb_fill_rect_blend(int x, int y, int w, int h, uint32_t color, uint8_t alpha) {
    if (w <= 0 || h <= 0 || alpha == 0) return;
    if (alpha == 255) { fb_fill_rect(x, y, w, h, color); return; }
    int x1 = x + w, y1 = y + h;
    if (x1 > (int)fb.width)  x1 = (int)fb.width;
    if (y1 > (int)fb.height) y1 = (int)fb.height;
    if (x < clip_x0) x = clip_x0;
    if (y < clip_y0) y = clip_y0;
    if (x1 > clip_x1) x1 = clip_x1;
    if (y1 > clip_y1) y1 = clip_y1;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 <= x || y1 <= y) return;

    /* Hoist the constant fg * alpha products out of the inner loop */
    uint32_t fg_r    = ((color >> 16) & 0xFFu) * (uint32_t)alpha;
    uint32_t fg_g    = ((color >>  8) & 0xFFu) * (uint32_t)alpha;
    uint32_t fg_b    = ( color        & 0xFFu) * (uint32_t)alpha;
    uint32_t inv     = (uint32_t)(255u - alpha);

    for (int row = y; row < y1; row++) {
        uint32_t *p = (uint32_t *)((uint8_t *)fb_target() + (uint32_t)row * fb.pitch) + x;
        for (int col = x; col < x1; col++, p++) {
            uint32_t bg = *p;
            uint8_t r = (uint8_t)((fg_r + ((bg >> 16) & 0xFFu) * inv) >> 8);
            uint8_t g = (uint8_t)((fg_g + ((bg >>  8) & 0xFFu) * inv) >> 8);
            uint8_t b = (uint8_t)((fg_b + ( bg        & 0xFFu) * inv) >> 8);
            *p = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
        }
    }
}

/* ── fb_blur_rect ─────────────────────────────────────────────────────────
 * Optimisation: true separable two-pass box blur.
 *
 * Old approach: O(r²) per pixel (naïve 2-D kernel, step-sampled).
 * New approach: O(1) amortised per pixel using a horizontal sliding-window
 *   sum (pass 1 → scratch buffer) followed by a vertical sliding-window sum
 *   (pass 2 → framebuffer).  For r=4 on a 600×500 window this reduces the
 *   sample count from ~27 M to ~5.4 M, and all accesses are sequential.
 *
 * Edge clamping: pixels outside the rect boundary are clamped to the nearest
 * edge pixel, so the window divisor stays constant at (2r+1) — no division
 * variable across pixels, letting the compiler use a multiply reciprocal.   */
void fb_blur_rect(int x, int y, int w, int h, int radius) {
    if (!fb.initialized || w <= 0 || h <= 0 || radius <= 0) return;
    if (radius > 8) radius = 8;

    int x0 = x, y0 = y;
    int x1 = x + w, y1 = y + h;
    if (x1 > (int)fb.width)  x1 = (int)fb.width;
    if (y1 > (int)fb.height) y1 = (int)fb.height;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x0 < clip_x0) x0 = clip_x0;
    if (y0 < clip_y0) y0 = clip_y0;
    if (x1 > clip_x1) x1 = clip_x1;
    if (y1 > clip_y1) y1 = clip_y1;
    int rw = x1 - x0, rh = y1 - y0;
    if (rw <= 0 || rh <= 0) return;

    /* Allocate scratch buffer for the horizontal-pass result */
    uint32_t *scratch = (uint32_t *)kmalloc((size_t)rw * (size_t)rh * sizeof(uint32_t));
    if (!scratch) {
        /* Fallback: original step-sampled single-pass (always succeeds) */
        int step = 3;
        for (int py = y0; py < y1; py++) {
            uint32_t *row = (uint32_t *)((uint8_t *)fb_target() + (uint32_t)py * fb.pitch);
            for (int px = x0; px < x1; px++) {
                uint32_t sr = 0, sg = 0, sb = 0; int n = 0;
                for (int dy = -radius; dy <= radius; dy += step) {
                    int sy = py + dy;
                    if (sy < y0 || sy >= y1) continue;
                    uint32_t *srow = (uint32_t *)((uint8_t *)fb_target() + (uint32_t)sy * fb.pitch);
                    for (int dx = -radius; dx <= radius; dx += step) {
                        int sx2 = px + dx;
                        if (sx2 < x0 || sx2 >= x1) continue;
                        uint32_t c = srow[sx2];
                        sr += (c >> 16) & 0xFF; sg += (c >> 8) & 0xFF; sb += c & 0xFF; n++;
                    }
                }
                if (n > 0) row[px] = ((sr/(uint32_t)n)<<16)|((sg/(uint32_t)n)<<8)|(sb/(uint32_t)n);
            }
        }
        return;
    }

    int diam = 2 * radius + 1;

    /* ── Pass 1: horizontal sliding-window blur → scratch ──────────────── */
    for (int py = y0; py < y1; py++) {
        const uint32_t *src = (const uint32_t *)
            ((uint8_t *)fb_target() + (uint32_t)py * fb.pitch);
        uint32_t *dst = scratch + (py - y0) * rw;

        /* Prime the sliding window for column 0 */
        uint32_t sr = 0, sg = 0, sb = 0;
        for (int k = -radius; k <= radius; k++) {
            int sx = x0 + k;
            if (sx < x0) sx = x0;       /* clamp left edge  */
            if (sx >= x1) sx = x1 - 1;  /* clamp right edge */
            uint32_t c = src[sx];
            sr += (c >> 16) & 0xFFu;
            sg += (c >>  8) & 0xFFu;
            sb +=  c        & 0xFFu;
        }
        dst[0] = ((sr / (uint32_t)diam) << 16) |
                 ((sg / (uint32_t)diam) <<  8) |
                  (sb / (uint32_t)diam);

        /* Slide the window one pixel to the right for each remaining column */
        for (int px = 1; px < rw; px++) {
            /* Remove the pixel that just left the left side of the window */
            int rem_x = (x0 + px - 1) - radius;
            if (rem_x < x0) rem_x = x0;
            uint32_t rem_c = src[rem_x];
            sr -= (rem_c >> 16) & 0xFFu;
            sg -= (rem_c >>  8) & 0xFFu;
            sb -=  rem_c        & 0xFFu;

            /* Add the pixel that just entered the right side of the window */
            int add_x = (x0 + px) + radius;
            if (add_x >= x1) add_x = x1 - 1;
            uint32_t add_c = src[add_x];
            sr += (add_c >> 16) & 0xFFu;
            sg += (add_c >>  8) & 0xFFu;
            sb +=  add_c        & 0xFFu;

            dst[px] = ((sr / (uint32_t)diam) << 16) |
                      ((sg / (uint32_t)diam) <<  8) |
                       (sb / (uint32_t)diam);
        }
    }

    /* ── Pass 2: vertical sliding-window blur (from scratch) → framebuffer */
    for (int px = x0; px < x1; px++) {
        int col = px - x0;

        /* Prime the sliding window for row 0 */
        uint32_t sr = 0, sg = 0, sb = 0;
        for (int k = -radius; k <= radius; k++) {
            int sy = k;
            if (sy < 0)   sy = 0;
            if (sy >= rh) sy = rh - 1;
            uint32_t c = scratch[sy * rw + col];
            sr += (c >> 16) & 0xFFu;
            sg += (c >>  8) & 0xFFu;
            sb +=  c        & 0xFFu;
        }
        {
            uint32_t *drow = (uint32_t *)((uint8_t *)fb_target() + (uint32_t)y0 * fb.pitch);
            drow[px] = ((sr / (uint32_t)diam) << 16) |
                       ((sg / (uint32_t)diam) <<  8) |
                        (sb / (uint32_t)diam);
        }

        /* Slide downward */
        for (int py = 1; py < rh; py++) {
            /* Remove top pixel leaving the window */
            int rem_y = (py - 1) - radius;
            if (rem_y < 0)   rem_y = 0;
            uint32_t rem_c = scratch[rem_y * rw + col];
            sr -= (rem_c >> 16) & 0xFFu;
            sg -= (rem_c >>  8) & 0xFFu;
            sb -=  rem_c        & 0xFFu;

            /* Add bottom pixel entering the window */
            int add_y = py + radius;
            if (add_y >= rh) add_y = rh - 1;
            uint32_t add_c = scratch[add_y * rw + col];
            sr += (add_c >> 16) & 0xFFu;
            sg += (add_c >>  8) & 0xFFu;
            sb +=  add_c        & 0xFFu;

            uint32_t *drow = (uint32_t *)((uint8_t *)fb_target() +
                             (uint32_t)(y0 + py) * fb.pitch);
            drow[px] = ((sr / (uint32_t)diam) << 16) |
                       ((sg / (uint32_t)diam) <<  8) |
                        (sb / (uint32_t)diam);
        }
    }

    kfree(scratch);
}
