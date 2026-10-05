/* NexOS — kernel/gui/gui.c | Main GUI event loop | MIT License */
#include "gui.h"
#include "wm.h"
#include "desktop.h"
#include "taskbar.h"
#include "launcher.h"
#include "term_app.h"
#include "files_app.h"
#include "sysinfo_app.h"
#include "theme_app.h"
#include "notif.h"
#include "browser_app.h"
#include "calc_app.h"
#include "clock_app.h"
#include "edit_app.h"
#include "viz_app.h"
#include "snake_app.h"
#include "sysmon_app.h"
#include "settings_app.h"
#include "aurora.h"
#include "shell.h"
#include "../../ports/netsurf/compat/nexos_frontend.h"
#include "../drivers/fb.h"
#include "../drivers/console.h"
#include "../drivers/font.h"
#include "../drivers/mouse.h"
#include "../drivers/rtl8139.h"
#include "../drivers/keyboard.h"
#include "../drivers/timer.h"
#include "../kernel.h"

#ifndef GUI_DEBUG_STAGE
#define GUI_DEBUG_STAGE 0
#endif

#if GUI_DEBUG_STAGE != 0
static void gui_debug_framebuffer_report(void) {
    size_t bytes = (size_t)fb.pitch * (size_t)fb.height;
    klog(LOG_INFO,
         "FB GUI layout physical=%ux%u pitch=%u bpp=%u scene=%ux%u stride=%u bytes=%u guards=%s",
         (unsigned)fb.width, (unsigned)fb.height, (unsigned)fb.pitch,
         (unsigned)fb.bpp, (unsigned)fb.width, (unsigned)fb.height,
         (unsigned)fb.pitch, (unsigned)bytes,
         fb_backbuffer_guards_ok() ? "ok" : "BROKEN");
}

static void gui_debug_grid(void) {
    static const int xs[] = {0, 16, 90, 116, 720, 768, 790, 1023,
                             1279, 1336, 1439};
    static const int ys[] = {0, 12, 56, 82, 450, 732, 768, 832, 876, 899};
    const uint32_t line = 0xFFFFFF;
    fb_clear(0x101828);
    fb_reset_clip();
    fb_draw_rect_outline(0, 0, (int)fb.width, (int)fb.height, 0xFF00FF, 1);
    for (unsigned i = 0; i < sizeof(xs) / sizeof(xs[0]); i++) {
        if (xs[i] >= 0 && xs[i] < (int)fb.width)
            fb_draw_line(xs[i], 0, xs[i], (int)fb.height - 1, line);
    }
    for (unsigned i = 0; i < sizeof(ys) / sizeof(ys[0]); i++) {
        if (ys[i] >= 0 && ys[i] < (int)fb.height)
            fb_draw_line(0, ys[i], (int)fb.width - 1, ys[i], line);
    }
    fb_fill_rect(32, 120, 260, 160, 0xB02040);
    fb_fill_rect(340, 120, 260, 160, 0x2050B0);
    fb_fill_rect(648, 120, 260, 160, 0x208050);
    fb_fill_rect(956, 120, 260, 160, 0xB08020);
    font_aurora_puts(42, 132, "Q1 0..719", 20, 0xFFFFFF, 0xB02040);
    font_aurora_puts(350, 132, "Q2 720..", 20, 0xFFFFFF, 0x2050B0);
    font_aurora_puts(658, 132, "Q3", 20, 0xFFFFFF, 0x208050);
    font_aurora_puts(966, 132, "Q4", 20, 0xFFFFFF, 0xB08020);
}

static void gui_debug_font_test(void) {
    fb_clear(aurora_color(AURORA_COLOR_BACKGROUND));
    fb_reset_clip();
    int y = 24;
    const int sizes[] = {12, 14, 16, 20, 24, 32};
    const char *samples[] = {
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
        "abcdefghijklmnopqrstuvwxyz",
        "0123456789",
        "NexOS Desktop",
        "Ethernet   Files   Terminal   Browser",
        "Settings   System Monitor   12:34"
    };
    for (unsigned i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        char label[8];
        label[0] = (char)('0' + sizes[i] / 10);
        label[1] = (char)('0' + sizes[i] % 10);
        label[2] = 'p'; label[3] = 'x'; label[4] = 0;
        font_puts(20, y + 2, label, 0x78C7FF, 0);
        font_aurora_puts(80, y, samples[i], sizes[i], 0xFFFFFF, 0);
        y += sizes[i] + 18;
    }
}

static void gui_debug_stage(int stage) {
    gui_debug_framebuffer_report();
    if (stage == 1) gui_debug_grid();
    else if (stage == 8) gui_debug_font_test();
    else if (stage == 9) {
        fb_clear(0x101828);
        fb_reset_clip();
        fb_fill_rounded_rect(18, 82, 72, 650, 24, 0x0D111B);
        fb_draw_rect_outline(18, 82, 72, 650, 0xFFFFFF, 1);
    }
    else if (stage == 10) {
        fb_clear(0x101828);
        fb_reset_clip();
        aurora_app_icon_id(54, 122, 16, AURORA_ICON_APPS, 0x7C8CFF);
    }
    else if (stage == 11 || stage == 12) {
        /* Screenshot-only shell states.  They exercise the real launcher
         * and notification renderers without adding runtime product state. */
        fb_clear(COL_BASE);
        desktop_draw();
        shell_draw_topbar();
        shell_draw_left_rail();
        term_app_t *t = term_create(186, 218);
        files_app_t *f = files_create(606, 262);
        if (t && t->win) wm_resize(t->win, 540, 320);
        if (f && f->win) wm_resize(f->win, 610, 380);
        wm_render_all();
        taskbar_draw();
        if (stage == 11) {
            launcher_show(0, 0);
            launcher_tick(1000);
            launcher_draw();
        } else {
            notif_show("NexOS", "Welcome to NexOS 0.1", 4000);
            notif_draw();
        }
    }
    else {
        fb_clear(COL_BASE);
        desktop_draw();
        if (stage >= 3) shell_draw_topbar();
        if (stage >= 4) shell_draw_left_rail();
        if (stage >= 5) taskbar_draw();
        if (stage >= 6) {
            term_create(150, 160);
            if (stage >= 7) files_create(440, 128);
            wm_render_all();
        }
    }
    if (!fb_backbuffer_guards_ok())
        klog(LOG_ERROR, "FB GUI backbuffer guard corrupted at stage=%d", stage);
    fb_commit();
    klog(LOG_INFO, "FB GUI DEBUG stage=%d complete", stage);
    for (;;) { __asm__ volatile("sti; hlt"); }
}
#endif


/* ── Launch helpers (called from launcher.c) ─────────────────────────────── */
static int term_count  = 0;
static int files_count = 0;

void launch_terminal(void) {
    int ox = 60 + (term_count % 4) * 40;
    int oy = 60 + (term_count % 4) * 30;
    term_create(ox, oy);
    term_count++;
}

void launch_filemanager(void) {
    int ox = 560 + (files_count % 3) * 20;
    int oy = 80  + (files_count % 3) * 20;
    files_create(ox, oy);
    files_count++;
}

void launch_sysinfo(void) {
    sysinfo_create(300, 100);
}

void launch_themewin(void) {
    theme_create(380, 200);
}

void launch_browser(void) {
    static int bc = 0;
    int ox = 80  + (bc % 3) * 24;
    int oy = 50  + (bc % 3) * 18;
    browser_create(ox, oy);
    bc++;
}

void launch_calc(void) {
    static int cc = 0;
    int ox = 420 + (cc % 4) * 18;
    int oy = 90  + (cc % 4) * 18;
    calc_create(ox, oy);
    cc++;
}

void launch_clock(void) {
    static int kc = 0;
    int ox = 520 + (kc % 3) * 16;
    int oy = 260 + (kc % 3) * 16;
    clock_create(ox, oy);
    kc++;
}

void launch_editor(void) {
    static int ec = 0;
    int ox = 80 + (ec % 5) * 22;
    int oy = 60 + (ec % 5) * 22;
    edit_create(ox, oy, NULL);
    ec++;
}

void launch_visualizer(void) {
    static int vc = 0;
    int ox = 180 + (vc % 4) * 18;
    int oy = 80  + (vc % 4) * 18;
    viz_create(ox, oy);
    vc++;
}

void launch_snake(void) {
    static int sc = 0;
    int ox = 160 + (sc % 4) * 18;
    int oy = 80  + (sc % 4) * 18;
    snake_create(ox, oy);
    sc++;
}

void launch_sysmon(void) {
    static int mc = 0;
    int ox = 120 + (mc % 4) * 16;
    int oy = 70  + (mc % 4) * 16;
    sysmon_create(ox, oy);
    mc++;
}

void launch_settings(void) {
    static int sc = 0;
    int ox = 160 + (sc % 3) * 20;
    int oy = 80  + (sc % 3) * 20;
    settings_create(ox, oy);
    sc++;
}

/* ── Simple pseudo-random for window placement ─────────────────────────── */
static uint32_t g_seed = 12345;
static int prand(int range) {
    g_seed = g_seed * 1664525u + 1013904223u;
    return (int)((g_seed >> 16) % (uint32_t)range);
}

/* ── Main event loop ─────────────────────────────────────────────────────── */
void gui_main(void) {
    if (!fb.initialized) {
        klog(LOG_ERROR, "GUI: framebuffer not initialised — cannot start");
        return;
    }

    /* Early console output used the physical framebuffer directly.  From
     * the GUI phase onward, compose complete frames off-screen and commit
     * them once, so cursor/window/NetSurf paints cannot expose intermediate
     * states on scanout. */
    if (!fb_enable_backbuffer())
        klog(LOG_WARN, "GUI: backbuffer unavailable; using direct framebuffer");

    /* From this point onward the physical display is owned by the GUI
     * compositor.  klog continues to serial, but the legacy framebuffer
     * text console must no longer write into the GUI backbuffer. */
    console_set_display_enabled(0);

    /* 1. Init subsystems */
    aurora_apply_palette(aurora_default_palette());
    mouse_init();
    wm_init();
    taskbar_init();
    notif_init();

#if GUI_DEBUG_STAGE != 0
    /* Diagnostic builds intentionally stop after one deterministic frame so
     * QEMU screendumps can identify the first layer that corrupts pixels. */
    gui_debug_stage(GUI_DEBUG_STAGE);
#endif

    /* 2. Create the intended startup applications before the first commit.
     * The first visible frame must already be a complete composed scene,
     * rather than a desktop-only frame followed by a second scene. */
    /* Canonical shell composition: the rail occupies the left margin and
     * startup windows sit inside the central work area. */
    /* Match the canonical DESK-002/003 composition: Terminal recedes at
     * left, Files sits forward at right with deliberate overlap.  Resize is
     * shell geometry only; each app keeps its existing backend/client code. */
    term_app_t *startup_term = term_create(186, 218);
    files_app_t *startup_files = files_create(606, 262);
    if (startup_term && startup_term->win)
        wm_resize(startup_term->win, 540, 320);
    if (startup_files && startup_files->win)
        wm_resize(startup_files->win, 610, 380);

    wm_debug_report();

    /* Welcome notification */
    notif_show("NexOS", "Welcome to NexOS 0.1", 4000);

    /* 3. Draw and commit the complete initial desktop exactly once. */
    fb_clear(COL_BASE);
    desktop_draw();
    shell_draw_topbar();
    shell_draw_left_rail();
    wm_render_all();
    taskbar_draw();
    notif_draw();
    fb_commit();

    klog(LOG_INFO, "GUI: entering main loop");

    uint64_t last_frame   = 0;
    uint64_t last_clock   = 0;
    uint64_t last_notif   = 0;
    int      prev_left    = 0;
    int      prev_right   = 0;
    int      prev_mx      = -1;
    int      prev_my      = -1;
    int      ctrl_held    = 0;
    int      input_diag_budget = 50;
    int      button_diag_budget = 12;
    int      dispatch_diag_budget = 12;
    int      cursor_diag_budget = 12;
    int      cursor_present_diag_budget = 12;
    int      compose_diag_budget = 8;
    uint32_t scene_generation = 0;

    while (1) {
        uint64_t now = timer_get_ticks();

        /* The cursor is an overlay on the previous complete frame.  Remove
         * it before networking, input callbacks, or application work can
         * repaint the surface underneath it. */
        fb_reset_clip();
        cursor_restore();

        /* Process NIC packets outside interrupt context.  The RTL8139 IRQ
         * only marks RX work pending because the network stack allocates. */
        rtl8139_service();
        nexos_netsurf_pump();

        /* ── Keyboard events ────────────────────────────────────────────── */
        while (keyboard_available()) {
            char key = keyboard_getchar();

            /* Ctrl key combos arrive as ASCII control codes 1-26 */
            if (key >= 1 && key <= 26) {
                ctrl_held = 1;
                switch (key) {
                case 20: /* Ctrl+T — new terminal */
                    term_create(80 + prand(200), 60 + prand(100));
                    term_count++;
                    break;
                case 6:  /* Ctrl+F — new files */
                    launch_filemanager();
                    break;
                case 9:  /* Ctrl+I — system info */
                    launch_sysinfo();
                    break;
                case 23: /* Ctrl+W */
                case 17: /* Ctrl+Q — close focused window */
                    {
                        window_t *fw = wm_focused();
                        if (fw) {
                            if (fw->on_close) fw->on_close(fw);
                            else wm_close(fw);
                        }
                    }
                    break;
                default:
                    wm_handle_key(key);
                    break;
                }
            } else {
                ctrl_held = 0;
                if (key == 27) { /* Escape */
                    if (launcher_is_visible()) launcher_hide();
                } else {
                    wm_handle_key(key);
                    if (launcher_is_visible()) launcher_handle_key(key);
                }
            }
            (void)ctrl_held;
        }

        /* ── Mouse events ───────────────────────────────────────────────── */
        /* Publish the latest complete packet position before hit-testing and
         * drawing.  The driver deliberately does not interpolate here: the
         * host pointer must not lag behind the physical mouse. */
        mouse_needs_update();
        int mx    = mouse_get_x();
        int my    = mouse_get_y();
        int left  = mouse_left();
        int right = mouse_right();
        int wheel = mouse_get_wheel();
        int packet_dx, packet_dy, packet_buttons, packet_wheel;
        int packet_raw0, packet_overflow_x, packet_overflow_y;
        int button_old, button_new, button_raw;

        if (input_diag_budget > 0 &&
            mouse_take_debug(&packet_dx, &packet_dy, &packet_buttons,
                             &packet_wheel, &packet_raw0,
                             &packet_overflow_x, &packet_overflow_y)) {
            klog(LOG_DEBUG,
                 "MOUSE PACKET raw0=0x%x dx=%d dy=%d buttons=%d wheel=%d overflow_x=%d overflow_y=%d",
                 packet_raw0, packet_dx, packet_dy, packet_buttons,
                 packet_wheel, packet_overflow_x, packet_overflow_y);
            input_diag_budget--;
        }

        if (button_diag_budget > 0 &&
            mouse_take_button_debug(&button_old, &button_new, &button_raw)) {
            klog(LOG_DEBUG,
                 "INPUT BUTTON raw0=0x%x old=%d new=%d left=%d right=%d middle=%d",
                 button_raw, button_old, button_new,
                 button_new & 1, (button_new >> 1) & 1,
                 (button_new >> 2) & 1);
            button_diag_budget--;
        }

        int left_click  = left  && !prev_left;
        int right_click = right && !prev_right;
        int released    = !left && prev_left;
        int cursor_changed = mx != prev_mx || my != prev_my;
        int input_changed = left_click || right_click || released ||
                            wheel != 0 || cursor_changed;

        if (cursor_changed && cursor_diag_budget > 0) {
            klog(LOG_DEBUG,
                 "GUI CURSOR old=(%d,%d) new=(%d,%d) changed=yes scene_dirty=%d",
                 prev_mx, prev_my, mx, my, fb_scene_dirty);
            cursor_diag_budget--;
        }

        if (dispatch_diag_budget > 0 && (left_click || released)) {
            klog(LOG_DEBUG, "GUI BUTTON event=%s x=%d y=%d left=%d right=%d",
                 left_click ? "DOWN" : "UP", mx, my, left, right);
            dispatch_diag_budget--;
        }

        if (input_changed) {
            /* Forward mouse position to launcher for hover highlighting */
            if (launcher_is_visible())
                launcher_handle_mouse(mx, my);
            /* Taskbar hover glow — always forward mouse position */
            taskbar_handle_mouse(mx, my);
            if (launcher_is_visible() && left_click) {
                launcher_handle_click(mx, my);
            } else if (right_click && !taskbar_contains(mx, my)) {
                launcher_show(mx, my);
            } else {
                if (taskbar_contains(mx, my) && left_click) {
                    taskbar_handle_click(mx, my);
                } else {
                    wm_handle_mouse(mx, my, left, right);
                }
            }
            prev_mx = mx; prev_my = my;
        }
        if (released) wm_handle_mouse_release(mx, my);
        if (wheel != 0) wm_handle_mouse_wheel(mx, my, wheel);
        prev_left  = left;
        prev_right = right;

        /* Cursor movement does not require rebuilding the desktop scene.
         * The backbuffer contains the scene without the cursor after the
         * restore above, so present only the old and new cursor rectangles.
         * This keeps pointer motion responsive without reintroducing the
         * old full-frame repaint storm. */
        int cursor_presented = 0;
        int scene_pending = fb_scene_dirty || left_click || right_click ||
                            released || wheel != 0;
        if (cursor_changed && !scene_pending) {
            /* cursor_restore() at loop start already copied the old
             * rectangle from the clean scene to scanout.  Draw only the
             * new transient overlay; never copy cursor pixels into the
             * scene backbuffer. */
            cursor_draw(mx, my);
            cursor_presented = 1;
            if (cursor_present_diag_budget > 0) {
                klog(LOG_DEBUG,
                     "CURSOR PRESENT position=(%d,%d) cursor_only=yes scene_dirty=%d",
                     mx, my, fb_scene_dirty);
                cursor_present_diag_budget--;
            }
        }

        /* ── Frame render (~30 fps) ─────────────────────────────────────── */
        if (now - last_frame >= 33) {
            uint32_t frame_ms = (uint32_t)(now - last_frame);
            last_frame = now;
            int scene_changed = fb_scene_dirty || left_click || right_click ||
                                released || wheel != 0;
            /* prev_mx/prev_my are updated by input dispatch above, so use
             * the captured transition rather than comparing them again. */
            int present_changed = scene_changed ||
                                  (cursor_changed && !cursor_presented);

            /* Advance launcher open/close animation */
            launcher_tick(frame_ms);

            /* Repaint desktop gradient only when something changed */
            if (fb_scene_dirty) {
                /* Application/NetSurf painting may leave a content clip
                 * active. A complete scene rebuild must always start from
                 * the full-screen compositor clip. */
                fb_reset_clip();
                desktop_draw();
                shell_draw_topbar();
                shell_draw_left_rail();
                fb_scene_dirty = 0;
            }

            if (scene_changed) {
                scene_generation++;
                if (compose_diag_budget > 0) {
                    klog(LOG_DEBUG, "SCENE COMPOSE generation=%u mode=full",
                         scene_generation);
                    compose_diag_budget--;
                }
                fb_reset_clip();
                wm_render_all();
                fb_reset_clip();
                taskbar_draw();
                fb_reset_clip();
                if (launcher_is_visible()) launcher_draw();
                fb_reset_clip();
                notif_draw();
            }

            /* Commit the cursor-free scene before presenting the transient
             * cursor overlay.  Committing after cursor_draw() would copy the
             * clean backbuffer over the cursor and cause flicker/disappearing
             * pointer frames during clicks, drags, and scene invalidation. */
            if (present_changed && scene_changed)
                fb_commit();
            fb_reset_clip();
            cursor_draw(mx, my);
        }

        /* ── Clock update every second ──────────────────────────────────── */
        if (now - last_clock >= 1000) {
            last_clock = now;
            taskbar_update();
        }

        /* ── Notification tick every 33ms ───────────────────────────────── */
        if (now - last_notif >= 33) {
            notif_tick((uint32_t)(now - last_notif));
            last_notif = now;
        }

        /* Yield CPU */
        __asm__ volatile ("sti; hlt");
    }
}
