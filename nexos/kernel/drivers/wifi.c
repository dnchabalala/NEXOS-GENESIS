/* NexOS — kernel/drivers/wifi.c | Wi-Fi capability placeholder | MIT License
 *
 * No Wi-Fi chipset driver is implemented yet. Ethernet is provided by the
 * real RTL8139 driver; this module must never fabricate radio state.
 */
#include "wifi.h"
#include "../net/netif.h"
#include "../kernel.h"

/* ── State ───────────────────────────────────────────────────────────────── */
static int  wifi_up      = 0;
static char wifi_ssid[WIFI_SSID_LEN] = {0};
static int  wifi_sig     = 0;

/* ── Helpers ─────────────────────────────────────────────────────────────── */
/* ── Public API ──────────────────────────────────────────────────────────── */
void wifi_init(void) {
    wifi_up  = 0;
    wifi_ssid[0] = 0;
    wifi_sig = 0;
    klog(LOG_INFO, "WiFi: no supported hardware driver; Ethernet remains available");
}

int wifi_scan(wifi_ap_t *out, int max) {
    (void)out; (void)max;
    return 0;
}

int wifi_connect(const char *ssid, const char *password) {
    (void)ssid; (void)password;
    klog(LOG_WARN, "WiFi: no supported hardware driver");
    return -1;
}

void wifi_disconnect(void) {
    if (!wifi_up) return;
    wifi_up      = 0;
    wifi_ssid[0] = 0;
    wifi_sig     = 0;
    /* Mark wlan0 DOWN so netif_is_up() returns the correct state */
    netif_set_down("wlan0");
    klog(LOG_INFO, "WiFi: disconnected");
}

int         wifi_is_connected(void) { return wifi_up; }
const char *wifi_get_ssid(void)     { return wifi_ssid; }
int         wifi_get_signal(void)   { return wifi_sig; }
