#ifndef NEXOS_NETSURF_FRONTEND_H
#define NEXOS_NETSURF_FRONTEND_H

#include "netsurf/browser_window.h"
#include "netsurf/utils/errors.h"
#include "kernel/gui/wm.h"

nserror nexos_netsurf_init(void);
nserror nexos_netsurf_open_blank(struct browser_window **out);
nserror nexos_netsurf_navigate(struct browser_window *bw, const char *address);
void nexos_netsurf_bind_window(window_t *window, int x, int y,
                               int width, int height);
void nexos_netsurf_set_viewport(window_t *window, int x, int y,
                                int width, int height);
void nexos_netsurf_paint(window_t *window);
bool nexos_netsurf_scroll(window_t *window, int dx, int dy);
void nexos_netsurf_mouse_track(window_t *window, int x, int y);
void nexos_netsurf_detach_window(window_t *window);
void nexos_netsurf_pump(void);

#endif
