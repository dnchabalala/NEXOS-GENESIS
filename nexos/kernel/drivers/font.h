/* NexOS — kernel/drivers/font.h | 8x16 bitmap font renderer | MIT License */
#pragma once
#include <stdint.h>

void font_putchar(int x, int y, char c, uint32_t fg, uint32_t bg);
void font_puts(int x, int y, const char *s, uint32_t fg, uint32_t bg);
void font_printf(int x, int y, uint32_t fg, uint32_t bg, const char *fmt, ...);
void font_putchar2x(int x, int y, char c, uint32_t fg, uint32_t bg);
void font_puts2x(int x, int y, const char *s, uint32_t fg, uint32_t bg);
int  font_str_width(const char *s);
int  font_str_width2x(const char *s);

/* Build-time rasterized Aurora sans-serif atlas.  The legacy 8x16 API above
 * remains the boot, terminal, and compatibility renderer. */
void font_aurora_puts(int x, int y, const char *s, int pixel_height,
                      uint32_t fg, uint32_t bg);
int  font_aurora_str_width(const char *s, int pixel_height);
