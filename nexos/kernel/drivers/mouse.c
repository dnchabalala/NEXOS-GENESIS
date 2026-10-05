/* NexOS — kernel/drivers/mouse.c | PS/2 mouse + software cursor | MIT License */
#include "mouse.h"
#include "fb.h"
#include "../kernel.h"
#include "../arch/x86_64/idt.h"

void irq_install_handler(int irq, void (*handler)(registers_t *));

/* ── PS/2 state ────────────────────────────────────────────────────────── */
static volatile int     mouse_x;
static volatile int     mouse_y;
static volatile int     mouse_tx;
static volatile int     mouse_ty;
static volatile uint8_t mouse_btns;
static volatile uint8_t mouse_cycle;
static uint8_t mouse_bytes[4];
static int mouse_packet_size = 3;
static volatile int mouse_wheel_delta;
static volatile uint32_t mouse_packet_seq;
static volatile uint32_t mouse_debug_seq;
static volatile int mouse_last_dx;
static volatile int mouse_last_dy;
static volatile int mouse_last_wheel;
static volatile int mouse_last_buttons;
static volatile int mouse_last_raw0;
static volatile int mouse_last_overflow_x;
static volatile int mouse_last_overflow_y;
static volatile int mouse_button_old;
static volatile int mouse_button_new;
static volatile int mouse_button_raw;
static volatile uint32_t mouse_button_seq;
static volatile uint32_t mouse_button_debug_seq;
static volatile int mouse_needs_redraw = 0;

/* ── Cursor ─────────────────────────────────────────────────────────────── */
#define CUR_W 12
#define CUR_H 20
static int      cursor_saved_x = -1;
static int      cursor_saved_y = -1;

static const uint8_t cursor_bmp[CUR_H][CUR_W] = {
    {1,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0,0,0,0,0},
    {1,2,1,0,0,0,0,0,0,0,0,0},
    {1,2,2,1,0,0,0,0,0,0,0,0},
    {1,2,2,2,1,0,0,0,0,0,0,0},
    {1,2,2,2,2,1,0,0,0,0,0,0},
    {1,2,2,2,2,2,1,0,0,0,0,0},
    {1,2,2,2,2,2,2,1,0,0,0,0},
    {1,2,2,2,2,2,2,2,1,0,0,0},
    {1,2,2,2,2,2,2,2,2,1,0,0},
    {1,2,2,2,2,2,2,1,1,1,0,0},
    {1,2,2,2,1,2,2,1,0,0,0,0},
    {1,2,2,1,0,1,2,2,1,0,0,0},
    {1,2,1,0,0,1,2,2,1,0,0,0},
    {1,1,0,0,0,0,1,2,2,1,0,0},
    {0,0,0,0,0,0,1,2,2,1,0,0},
    {0,0,0,0,0,0,0,1,1,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
};

/* ── I/O helpers ─────────────────────────────────────────────────────────── */
static void mouse_wait_write(void) {
    int t = 100000;
    while (t-- && (io_inb(0x64) & 2));
}
static void mouse_wait_read(void) {
    int t = 100000;
    while (t-- && !(io_inb(0x64) & 1));
}
static void mouse_write(uint8_t val) {
    mouse_wait_write(); io_outb(0x64, 0xD4);
    mouse_wait_write(); io_outb(0x60, val);
}
static uint8_t mouse_read(void) {
    mouse_wait_read();
    return io_inb(0x60);
}

/* ── IRQ12 handler ───────────────────────────────────────────────────────── */
static void mouse_irq_handler(registers_t *r) {
    (void)r;
    uint8_t b = io_inb(0x60);
    /* byte 0 must have bit3 set */
    if (mouse_cycle == 0 && !(b & 0x08)) return;
    mouse_bytes[mouse_cycle++] = b;
    if (mouse_cycle == (uint8_t)mouse_packet_size) {
        mouse_cycle = 0;
        int old_buttons = mouse_btns;
        mouse_btns = mouse_bytes[0] & 0x07;
        if (mouse_btns != old_buttons) {
            mouse_button_old = old_buttons;
            mouse_button_new = mouse_btns;
            mouse_button_raw = mouse_bytes[0];
            mouse_button_seq++;
        }
        int dx = (int)(int8_t)mouse_bytes[1];
        int dy = -(int)(int8_t)mouse_bytes[2];
        /* An overflow bit means the device could not represent the
         * movement. Do not turn that condition into a fabricated 127px
         * jump; keep the button/wheel state and discard only that axis. */
        if (mouse_bytes[0] & 0x40) dx = 0;
        if (mouse_bytes[0] & 0x80) dy = 0;
        mouse_last_raw0 = mouse_bytes[0];
        mouse_last_overflow_x = (mouse_bytes[0] & 0x40) != 0;
        mouse_last_overflow_y = (mouse_bytes[0] & 0x80) != 0;
        mouse_tx += dx;
        mouse_ty += dy;
        if (mouse_packet_size == 4) {
            int wheel = mouse_bytes[3] & 0x0f;
            if (wheel & 0x08) wheel -= 16;
            mouse_wheel_delta += wheel;
            mouse_last_wheel = wheel;
        } else {
            mouse_last_wheel = 0;
        }
        mouse_last_dx = dx;
        mouse_last_dy = dy;
        mouse_last_buttons = mouse_btns;
        mouse_packet_seq++;
        if (mouse_tx < 0) mouse_tx = 0;
        if (mouse_ty < 0) mouse_ty = 0;
        if (fb.initialized) {
            if (mouse_tx >= (int)fb.width)  mouse_tx = (int)fb.width  - 1;
            if (mouse_ty >= (int)fb.height) mouse_ty = (int)fb.height - 1;
        }
        mouse_needs_redraw = 1;
    }
}

/* ── Public API ──────────────────────────────────────────────────────────── */
void mouse_init(void) {
    mouse_x = (int)(fb.initialized ? fb.width  / 2 : 512);
    mouse_y = (int)(fb.initialized ? fb.height / 2 : 384);
    mouse_tx = mouse_x;
    mouse_ty = mouse_y;
    mouse_cycle = 0; mouse_btns = 0; mouse_wheel_delta = 0;
    mouse_packet_seq = 0; mouse_debug_seq = 0;
    mouse_last_dx = 0; mouse_last_dy = 0;
    mouse_last_wheel = 0; mouse_last_buttons = 0;
    mouse_last_raw0 = 0;
    mouse_last_overflow_x = 0; mouse_last_overflow_y = 0;
    mouse_button_old = 0; mouse_button_new = 0; mouse_button_raw = 0;
    mouse_button_seq = 0; mouse_button_debug_seq = 0;

    mouse_wait_write(); io_outb(0x64, 0xA8);
    mouse_wait_write(); io_outb(0x64, 0x20);
    mouse_wait_read();
    uint8_t status = io_inb(0x60) | 2;
    mouse_wait_write(); io_outb(0x64, 0x60);
    mouse_wait_write(); io_outb(0x60, status);
    mouse_write(0xF6); mouse_read();

    /* Negotiate the standard IntelliMouse extension.  A normal PS/2
     * device remains a three-byte device; QEMU and common hardware expose
     * wheel support as ID 3 after this sample-rate sequence. */
    mouse_write(0xF3); mouse_read(); mouse_write(200); mouse_read();
    mouse_write(0xF3); mouse_read(); mouse_write(100); mouse_read();
    mouse_write(0xF3); mouse_read(); mouse_write(80);  mouse_read();
    mouse_write(0xF2); mouse_read();
    {
        uint8_t id = mouse_read();
        mouse_packet_size = (id == 3) ? 4 : 3;
    }
    mouse_write(0xF4); mouse_read();

    irq_install_handler(12, mouse_irq_handler);
    klog(LOG_INFO, "Mouse: PS/2 initialized wheel=%s",
         mouse_packet_size == 4 ? "yes" : "no");
}

int     mouse_get_x(void)    { return mouse_x; }
int     mouse_get_y(void)    { return mouse_y; }
uint8_t mouse_get_btns(void) { return mouse_btns; }
int     mouse_left(void)     { return mouse_btns & 1; }
int     mouse_right(void)    { return mouse_btns & 2; }
int mouse_get_wheel(void) {
    int delta = mouse_wheel_delta;
    mouse_wheel_delta = 0;
    return delta;
}
int mouse_take_debug(int *dx, int *dy, int *buttons, int *wheel,
                     int *raw_byte0, int *overflow_x, int *overflow_y) {
    uint32_t seq = mouse_packet_seq;
    if (seq == mouse_debug_seq) return 0;
    mouse_debug_seq = seq;
    if (dx) *dx = mouse_last_dx;
    if (dy) *dy = mouse_last_dy;
    if (buttons) *buttons = mouse_last_buttons;
    if (wheel) *wheel = mouse_last_wheel;
    if (raw_byte0) *raw_byte0 = mouse_last_raw0;
    if (overflow_x) *overflow_x = mouse_last_overflow_x;
    if (overflow_y) *overflow_y = mouse_last_overflow_y;
    return 1;
}
int mouse_take_button_debug(int *old_buttons, int *new_buttons,
                            int *raw_byte0) {
    uint32_t seq = mouse_button_seq;
    if (seq == mouse_button_debug_seq) return 0;
    mouse_button_debug_seq = seq;
    if (old_buttons) *old_buttons = mouse_button_old;
    if (new_buttons) *new_buttons = mouse_button_new;
    if (raw_byte0) *raw_byte0 = mouse_button_raw;
    return 1;
}
int mouse_needs_update(void) {
    int changed = 0;

    /* Present the accumulated raw position directly. Interpolating toward
     * it made the cursor visibly lag behind the host pointer and replay
     * queued movement after the physical motion had stopped. */
    if (mouse_x != mouse_tx || mouse_y != mouse_ty) {
        mouse_x = mouse_tx;
        mouse_y = mouse_ty;
        changed = 1;
    }
    if (changed) return 1;
    if (mouse_needs_redraw) { mouse_needs_redraw = 0; return 1; }
    return 0;
}

void cursor_restore(void) {
    if (cursor_saved_x < 0) return;
    /* The scene backbuffer is authoritative and never contains cursor
     * pixels. Restore the complete old overlay rectangle from that scene;
     * do not rely on a saved underlay captured before a scene change. */
    fb_commit_rect(cursor_saved_x, cursor_saved_y, CUR_W, CUR_H);
    cursor_saved_x = -1; cursor_saved_y = -1;
}

void cursor_draw(int x, int y) {
    cursor_restore();
    cursor_saved_x = x; cursor_saved_y = y;
    for (int cy = 0; cy < CUR_H; cy++) {
        for (int cx = 0; cx < CUR_W; cx++) {
            if      (cursor_bmp[cy][cx] == 1) fb_present_pixel(x + cx, y + cy, 0x000000);
            else if (cursor_bmp[cy][cx] == 2) fb_present_pixel(x + cx, y + cy, 0xFFFFFF);
        }
    }
}
