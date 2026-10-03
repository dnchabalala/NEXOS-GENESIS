/* NexOS libnsfb surface adapter.
 *
 * This is a surface for the upstream libnsfb API. It deliberately reuses the
 * existing NexOS framebuffer and PS/2 drivers; it does not plot HTML or CSS.
 * NSFB_SURFACE_ABLE is the unused upstream surface slot reserved for a
 * framebuffer implementation, so no new graphics ABI is required here.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "libnsfb.h"
#include "libnsfb_event.h"
#include "nsfb.h"
#include "surface.h"
#include "plot.h"

#include "../../../../kernel/drivers/fb.h"
#include "../../../../kernel/drivers/keyboard.h"
#include "../../../../kernel/drivers/mouse.h"

static int nexos_defaults(nsfb_t *surface) {
    surface->width = fb.initialized ? (int)fb.width : 1024;
    surface->height = fb.initialized ? (int)fb.height : 768;
    surface->format = NSFB_FMT_XRGB8888;
    select_plotters(surface);
    return 0;
}

static int nexos_geometry(nsfb_t *surface, int width, int height,
                          enum nsfb_format_e format) {
    if (width > 0) surface->width = width;
    if (height > 0) surface->height = height;
    if (format != NSFB_FMT_ANY) surface->format = format;

    if (surface->format != NSFB_FMT_XRGB8888 || surface->width <= 0 ||
        surface->height <= 0)
        return -1;

    select_plotters(surface);
    return 0;
}

static int nexos_initialise(nsfb_t *surface) {
    if (!fb.initialized || fb.bpp != 32 || !fb.addr) return -1;
    if ((uint32_t)surface->width > fb.width ||
        (uint32_t)surface->height > fb.height)
        return -1;

    /* NetSurf's 32bpp XRGB plotter writes directly into NexOS VRAM. */
    surface->ptr = (uint8_t *)fb.addr;
    surface->linelen = (int)fb.pitch;
    return 0;
}

static int nexos_finalise(nsfb_t *surface) {
    surface->ptr = NULL;
    surface->linelen = 0;
    return 0;
}

static int nexos_update(nsfb_t *surface, nsfb_bbox_t *box) {
    (void)surface;
    (void)box;
    /* The NexOS framebuffer is scan-out memory; no flush syscall is needed.
     * The GUI can use its existing scene-dirty flag when this is integrated. */
    return 0;
}

static enum nsfb_key_code_e nexos_key(char key) {
    switch ((unsigned char)key) {
    case 0x80: return NSFB_KEY_UP;
    case 0x81: return NSFB_KEY_DOWN;
    case 0x82: return NSFB_KEY_LEFT;
    case 0x83: return NSFB_KEY_RIGHT;
    case 0x84: return NSFB_KEY_HOME;
    case 0x85: return NSFB_KEY_END;
    case 0x86: return NSFB_KEY_PAGEUP;
    case 0x87: return NSFB_KEY_PAGEDOWN;
    case 0x88: return NSFB_KEY_DELETE;
    default:
        if ((unsigned char)key < 128)
            return (enum nsfb_key_code_e)(unsigned char)key;
        return NSFB_KEY_UNKNOWN;
    }
}

static bool nexos_input(nsfb_t *surface, nsfb_event_t *event, int timeout) {
    static int last_x;
    static int last_y;
    static uint8_t last_buttons;
    int changed;
    uint8_t buttons;

    (void)surface;
    (void)timeout;

    if (keyboard_available()) {
        event->type = NSFB_EVENT_KEY_DOWN;
        event->value.keycode = nexos_key(keyboard_getchar());
        return true;
    }

    changed = mouse_needs_update();
    buttons = mouse_get_btns();
    if (changed || mouse_get_x() != last_x || mouse_get_y() != last_y) {
        last_x = mouse_get_x();
        last_y = mouse_get_y();
        event->type = NSFB_EVENT_MOVE_ABSOLUTE;
        event->value.vector.x = last_x;
        event->value.vector.y = last_y;
        event->value.vector.z = 0;
        return true;
    }

    if (buttons != last_buttons) {
        uint8_t delta = (uint8_t)(buttons ^ last_buttons);
        last_buttons = buttons;
        event->type = (buttons & delta) ? NSFB_EVENT_KEY_DOWN
                                        : NSFB_EVENT_KEY_UP;
        event->value.keycode = (delta & 1) ? NSFB_KEY_MOUSE_1 :
                               (delta & 2) ? NSFB_KEY_MOUSE_3 :
                               NSFB_KEY_UNKNOWN;
        return event->value.keycode != NSFB_KEY_UNKNOWN;
    }

    event->type = NSFB_EVENT_NONE;
    return false;
}

static const nsfb_surface_rtns_t nexos_routines = {
    .defaults = nexos_defaults,
    .initialise = nexos_initialise,
    .finalise = nexos_finalise,
    .geometry = nexos_geometry,
    .input = nexos_input,
    .update = nexos_update,
};

NSFB_SURFACE_DEF(nexos, NSFB_SURFACE_ABLE, &nexos_routines)
