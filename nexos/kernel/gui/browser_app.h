/* NexOS — kernel/gui/browser_app.h | GUI Web Browser | MIT License */
#pragma once
#include "wm.h"
#include <stdint.h>

struct browser_window;

#define BROWSER_URL_MAX   512

typedef enum {
    BSTATE_IDLE,
    BSTATE_LOADING,
    BSTATE_DONE,
    BSTATE_ERROR
} browser_state_t;

typedef struct {
    window_t        *win;
    char             url[BROWSER_URL_MAX];
    int              url_len;
    browser_state_t  state;
    char             status[80];
    struct browser_window *netsurf;
    uint8_t          address_focus;
} browser_app_t;

browser_app_t *browser_create(int x, int y);
void browser_netsurf_set_status(window_t *win, const char *status);
void browser_netsurf_set_url(window_t *win, const char *url);
