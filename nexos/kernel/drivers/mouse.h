/* NexOS — kernel/drivers/mouse.h | PS/2 mouse driver | MIT License */
#pragma once
#include <stdint.h>

void    mouse_init(void);
int     mouse_get_x(void);
int     mouse_get_y(void);
uint8_t mouse_get_btns(void);
int     mouse_left(void);
int     mouse_right(void);
int     mouse_needs_update(void);
int     mouse_get_wheel(void);
int     mouse_take_debug(int *dx, int *dy, int *buttons, int *wheel,
                         int *raw_byte0, int *overflow_x, int *overflow_y);
int     mouse_take_button_debug(int *old_buttons, int *new_buttons,
                                int *raw_byte0);
void    cursor_restore(void);
void    cursor_draw(int x, int y);
