/* NexOS — kernel/gui/settings_app.c | System Settings | MIT License
 *
 * Four-tab settings panel: WiFi | Display | System | About
 * WiFi tab: scan APs, select, enter password, connect / disconnect.
 * No FPU/SSE; integer arithmetic only.
 */
#include "settings_app.h"
#include "anim.h"
#include "wm.h"
#include "notif.h"
#include "aurora.h"
#include "../drivers/fb.h"
#include "../drivers/font.h"
#include "../drivers/wifi.h"
#include "../drivers/timer.h"
#include "../drivers/rtc.h"
#include "../mm/pmm.h"
#include "../mm/heap.h"
#include "../net/netif.h"
#include "../kernel.h"
#include <stdint.h>
#include <stddef.h>

void *kmalloc(size_t sz);
void  kfree(void *p);

static int slen(const char *s) { int n=0; while(s[n]) n++; return n; }
static void scpy(char *d, const char *s, int max) {
    int i=0; while(i<max-1&&s[i]){d[i]=s[i];i++;} d[i]=0;
}
static void mzero(void *p, int n) {
    uint8_t *b=(uint8_t*)p; for(int i=0;i<n;i++) b[i]=0;
}
static void u32s(uint32_t v, char *buf) {
    char t[12]; int ti=0;
    if(!v){t[ti++]='0';}else while(v){t[ti++]='0'+(int)(v%10);v/=10;}
    int bi=0; while(ti>0) buf[bi++]=t[--ti]; buf[bi]=0;
}
static void append(char *d, const char *s, int max) {
    int dl=slen(d), sl=slen(s);
    for(int i=0;i<sl&&dl+i<max-1;i++) d[dl+i]=s[i];
    d[dl+sl<max?dl+sl:max-1]=0;
}
static int sseq(const char *a, const char *b) {
    while(*a&&*a==*b){a++;b++;} return *a==*b;
}

#define SET_W       620
#define SET_H       460
#define TAB_H        36
#define TAB_COUNT     4
#define TAB_W       (SET_W / TAB_COUNT)
#define CONTENT_Y   (TAB_H + 6)
#define AP_ROW_H     38
#define AP_VISIBLE    5

static const char *TAB_NAMES[TAB_COUNT] = { "WiFi", "Display", "System", "About" };

#define C_BG    COL_BASE
#define C_CARD  COL_MANTLE
#define C_LINE  COL_SURFACE0
#define C_DIM   COL_SUBTEXT
#define C_TXT   COL_TEXT
#define C_HL    COL_BLUE
#define C_GRN   0xA6E3A1
#define C_RED   0xF38BA8
#define C_AMBE  0xF9E2AF
#define C_SEL   0x2A2B50
#define C_LOCK  0xCBA6F7

static int settings_sidebar_width(int width) {
    if (width >= 560) return 180;
    if (width >= 390) return 132;
    return 0;
}

static int settings_content_x(int width) {
    return settings_sidebar_width(width);
}

static int settings_content_width(int width) {
    int side = settings_sidebar_width(width);
    return width - side;
}

static void set_hdr(int wx, int wy_client, int cy, const char *title) {
    font_puts(wx + 14, wy_client + cy, title, C_DIM, C_BG);
    int off = 14 + slen(title)*8 + 10;
    fb_fill_rect(wx + off, wy_client + cy + 6, SET_W - off - 14, 1, C_LINE);
}

static void sig_bars(int x, int y, int pct) {
    static const int h[4] = { 4, 7, 11, 14 };
    for (int i = 0; i < 4; i++) {
        int thr = (i+1)*25;
        uint32_t c = (pct >= thr)
            ? (pct>70 ? C_GRN : pct>40 ? C_AMBE : C_RED)
            : 0x45475A;
        fb_fill_rect(x + i*6, y + (14-h[i]), 5, h[i], c);
    }
}

static void set_btn(int x, int y, int w, int h,
                    const char *lbl, uint32_t bg, uint32_t fg, int hov) {
    uint32_t c = hov ? anim_color_lerp(bg, 0xFFFFFF, 32) : bg;
    fb_fill_rounded_rect(x, y, w, h, 6, c);
    fb_draw_rect_outline(x, y, w, h, COL_SURFACE2, 1);
    int tw = slen(lbl)*8;
    font_puts(x+(w-tw)/2, y+(h-14)/2, lbl, fg, c);
}

static void pwd_field(int x, int y, int w, int h,
                      const char *val, int focused) {
    uint32_t bord = focused ? C_HL : COL_SURFACE1;
    fb_fill_rounded_rect(x, y, w, h, 5, COL_CRUST);
    fb_draw_rect_outline(x, y, w, h, bord, focused?2:1);
    int ml=slen(val); if(ml>22) ml=22;
    char stars[24]; for(int i=0;i<ml;i++) stars[i]='*'; stars[ml]=0;
    font_puts(x+8, y+(h-14)/2, stars, C_TXT, COL_CRUST);
    if(focused)
        fb_fill_rect(x+8+ml*8, y+5, 2, h-10, C_HL);
}

static void paint_tabs(settings_app_t *s, int wx, int wy_c) {
    fb_fill_rect(wx, wy_c, SET_W, TAB_H, COL_CRUST);
    fb_fill_rect(wx, wy_c+TAB_H, SET_W, 2, COL_SURFACE1);
    for(int i=0;i<TAB_COUNT;i++) {
        int tx = wx + i*TAB_W;
        int act = (s->tab==i);
        uint32_t tbg = act ? C_BG : (s->hover_tab==i ? COL_SURFACE0 : COL_CRUST);
        fb_fill_rect(tx, wy_c, TAB_W, TAB_H, tbg);
        if(act) fb_fill_rect(tx, wy_c+TAB_H-2, TAB_W, 2, C_HL);
        if(i>0) fb_fill_rect(tx, wy_c+5, 1, TAB_H-10, COL_SURFACE1);
        int tw = slen(TAB_NAMES[i])*8;
        font_puts(tx+(TAB_W-tw)/2, wy_c+(TAB_H-14)/2, TAB_NAMES[i], act?C_HL:C_DIM, tbg);
    }
}

static void paint_wifi(settings_app_t *s, int wx, int wy_c) {
    int cy = wy_c + CONTENT_Y;
    fb_fill_rounded_rect(wx+10, cy+2, SET_W-20, 52, 8, C_CARD);
    fb_draw_rect_outline(wx+10, cy+2, SET_W-20, 52, C_LINE, 1);

    if(wifi_is_connected()) {
        int sig = wifi_get_signal();
        const char *ssid = wifi_get_ssid();
        sig_bars(wx+22, cy+18, sig);
        font_puts(wx+52, cy+10, "Connected to:", C_DIM, C_CARD);
        font_puts(wx+52, cy+26, ssid, C_GRN, C_CARD);
        char sp[8]; u32s((uint32_t)sig, sp);
        char pct[12]; scpy(pct,sp,12); append(pct,"%",12);
        font_puts(wx+52+slen(ssid)*8+12, cy+26, pct, C_DIM, C_CARD);
        set_btn(wx+SET_W-128, cy+12, 112, 28,
                "Disconnect", 0x3B1525, C_RED, s->hover_disconnect);
    } else if(netif_find("eth0") && netif_is_up()) {
        font_puts(wx+22, cy+16, "Ethernet connected on eth0", C_HL, C_CARD);
        font_puts(wx+22, cy+30, "WiFi is available if you want wireless", C_DIM, C_CARD);
    } else {
        font_puts(wx+22, cy+16, "Not Connected", C_RED, C_CARD);
        font_puts(wx+22, cy+30, "Select a network below and press Connect", C_DIM, C_CARD);
    }
    cy += 58;

    set_hdr(wx, 0, cy - wy_c, "Available Networks");
    cy += 20;

    for(int i=0;i<s->ap_count&&i<AP_VISIBLE;i++) {
        int ry  = cy + i*AP_ROW_H;
        int sel = (s->selected_ap==i);
        int hov = (s->hover_ap==i);
        uint32_t rbg = sel ? C_SEL : hov ? COL_SURFACE0 : C_BG;

        fb_fill_rect(wx+10, ry, SET_W-20, AP_ROW_H-2, rbg);
        fb_fill_rect(wx+10, ry+AP_ROW_H-2, SET_W-20, 1, C_LINE);

        sig_bars(wx+18, ry+12, s->aps[i].signal);
        int is_conn = wifi_is_connected() && sseq(wifi_get_ssid(), s->aps[i].ssid);
        uint32_t nc = sel ? C_HL : is_conn ? C_GRN : C_TXT;
        font_puts(wx+50, ry+12, s->aps[i].ssid, nc, rbg);

        char sp[8]; u32s((uint32_t)s->aps[i].signal, sp);
        char pct[12]; scpy(pct,sp,12); append(pct,"%",12);
        font_puts(wx+SET_W-100, ry+12, pct, C_DIM, rbg);

        if(s->aps[i].encrypted)
            font_puts(wx+SET_W-54, ry+12, "WPA2", C_LOCK, rbg);
        else
            font_puts(wx+SET_W-54, ry+12, "Open", C_GRN,  rbg);
    }
    cy += AP_VISIBLE * AP_ROW_H + 8;

    int has_pwd_field = (s->selected_ap>=0 && s->aps[s->selected_ap].encrypted);
    if(has_pwd_field) {
        font_puts(wx+14, cy+4, "Password:", C_DIM, C_BG);
        pwd_field(wx+96, cy, 320, 28, s->password, s->pwd_focus);
        cy += 36;
    }

    if(s->selected_ap >= 0)
        set_btn(wx+14, cy, 116, 30, "Connect", C_HL, 0xFFFFFF, s->hover_connect);
    set_btn(wx+SET_W-116, cy, 100, 30, "Re-scan", COL_SURFACE1, C_TXT, s->hover_rescan);

    if(s->msg_timer>0 && s->msg_buf[0]) {
        uint32_t mc = s->msg_ok ? C_GRN : C_RED;
        font_puts(wx+148, cy+8, s->msg_buf, mc, C_BG);
    }
}

static void paint_display(settings_app_t *s, int wx, int wy_c) {
    (void)s;
    int cy = wy_c + CONTENT_Y + 12;
    set_hdr(wx, 0, cy-wy_c, "Resolution");
    cy += 22;
    char res[20];
    u32s(fb.width, res); append(res," x ",20); u32s(fb.height, res);
    append(res,"  32-bpp",20);
    fb_fill_rounded_rect(wx+14, cy, SET_W-28, 34, 6, C_CARD);
    font_puts(wx+22, cy+10, res, C_TXT, C_CARD);
    cy += 48;
    set_hdr(wx, 0, cy-wy_c, "Color Theme");
    cy += 22;
    fb_fill_rounded_rect(wx+14, cy, SET_W-28, 50, 6, C_CARD);
    font_puts(wx+22, cy+ 8, "Catppuccin Mocha", C_TXT, C_CARD);
    font_puts(wx+22, cy+24, "Open the Theme app to switch themes.", C_DIM, C_CARD);
    static const uint32_t swatches[] = { 0x89B4FA,0xCBA6F7,0xF38BA8,0xFAB387,0xF9E2AF,0xA6E3A1,0x94E2D5 };
    for(int i=0;i<7;i++) fb_fill_rounded_rect(wx+SET_W-180+i*22, cy+12, 18, 26, 9, swatches[i]);
    cy += 64;
    set_hdr(wx, 0, cy-wy_c, "Brightness");
    cy += 22;
    fb_fill_rounded_rect(wx+14, cy, SET_W-28, 30, 6, COL_SURFACE0);
    int bw = (SET_W-28)*80/100;
    fb_fill_rounded_rect(wx+14, cy, bw, 30, 6, C_HL);
    fb_fill_rect_blend(wx+14, cy, bw, 14, 0xFFFFFF, 20);
    font_puts(wx+14+bw/2-12, cy+8, "80%", 0xFFFFFF, C_HL);
    cy += 44;
    set_hdr(wx, 0, cy-wy_c, "Rendering");
    cy += 22;
    fb_fill_rounded_rect(wx+14, cy, SET_W-28, 34, 6, C_CARD);
    font_puts(wx+22, cy+10, "Software rasteriser  |  No GPU acceleration", C_DIM, C_CARD);
}

static void paint_system(settings_app_t *s, int wx, int wy_c) {
    (void)s;
    int cy = wy_c + CONTENT_Y + 12;
    set_hdr(wx, 0, cy-wy_c, "Date & Time");
    cy += 22;
    rtc_time_t rt; rtc_get_time(&rt);
    char tbuf[10];
    tbuf[0]='0'+rt.hour/10;   tbuf[1]='0'+rt.hour%10;   tbuf[2]=':';
    tbuf[3]='0'+rt.minute/10; tbuf[4]='0'+rt.minute%10; tbuf[5]=':';
    tbuf[6]='0'+rt.second/10; tbuf[7]='0'+rt.second%10; tbuf[8]=0;
    char dbuf[24]; u32s((uint32_t)(2000+rt.year),dbuf);
    append(dbuf,"-",24);
    char tmp[4]; u32s(rt.month,tmp);
    if(rt.month<10){char t2[4]; t2[0]='0'; t2[1]=tmp[0]; t2[2]=0; scpy(tmp,t2,4);}
    append(dbuf,tmp,24); append(dbuf,"-",24);
    u32s(rt.day,tmp);
    if(rt.day<10){char t2[4]; t2[0]='0'; t2[1]=tmp[0]; t2[2]=0; scpy(tmp,t2,4);}
    append(dbuf,tmp,24);
    fb_fill_rounded_rect(wx+14, cy, SET_W-28, 42, 6, C_CARD);
    font_puts2x(wx+22, cy+7, tbuf, C_HL, C_CARD);
    font_puts(wx+120, cy+14, dbuf, C_DIM, C_CARD);
    cy += 56;
    set_hdr(wx, 0, cy-wy_c, "Memory");
    cy += 22;
    uint32_t free_mb = (uint32_t)(pmm_get_free_frames()*4/1024);
    char mfr[12]; u32s(free_mb,mfr);
    char mstr[32]; scpy(mstr,mfr,32); append(mstr," MB free / 128 MB total",32);
    uint32_t used_pct = (128-free_mb)*100/128;
    fb_fill_rounded_rect(wx+14, cy, SET_W-28, 32, 6, COL_SURFACE0);
    int mw = (SET_W-28)*(int)used_pct/100;
    uint32_t mc = used_pct>80?C_RED:used_pct>60?C_AMBE:C_GRN;
    if(mw>2) fb_fill_rounded_rect(wx+14, cy, mw, 32, 6, mc);
    font_puts(wx+22, cy+9, mstr, C_TXT, used_pct>50?mc:COL_SURFACE0);
    cy += 46;
    set_hdr(wx, 0, cy-wy_c, "Network");
    cy += 22;
    fb_fill_rounded_rect(wx+14, cy, SET_W-28, 42, 6, C_CARD);
    if(wifi_is_connected()) {
        char nstr[48]; scpy(nstr,"wlan0  connected to  ",48);
        append(nstr,wifi_get_ssid(),48);
        font_puts(wx+22, cy+8, nstr, C_GRN, C_CARD);
        char sstr[32]; u32s((uint32_t)wifi_get_signal(),sstr);
        append(sstr,"% signal",32);
        font_puts(wx+22, cy+24, sstr, C_DIM, C_CARD);
    } else if(netif_find("eth0") && netif_is_up()) {
        font_puts(wx+22, cy+14, "eth0  connected (Ethernet)", C_HL, C_CARD);
    } else {
        font_puts(wx+22, cy+14, "No network interface up", C_RED, C_CARD);
    }
    cy += 56;
    set_hdr(wx, 0, cy-wy_c, "Power");
    cy += 22;
    set_btn(wx+14,  cy, 120, 34, "Reboot",   0x0E1A28, COL_SKY,  0);
    set_btn(wx+150, cy, 120, 34, "Shutdown", 0x280E0E, C_RED,    0);
}

static void paint_about(settings_app_t *s, int wx, int wy_c) {
    (void)s;
    int cy = wy_c + CONTENT_Y + 16;
    font_puts2x(wx+SET_W/2-48, cy, "NexOS", C_HL, C_BG);
    cy += 42;
    const char *ver = "Version  0.1  (x86_64 bare-metal)";
    font_puts(wx+SET_W/2-slen(ver)*4, cy, ver, C_DIM, C_BG);
    cy += 20;
    fb_fill_rect(wx+14, cy, SET_W-28, 1, C_LINE);
    cy += 14;
    static const char *rows[][2] = {
        {"Architecture", "x86_64"},
        {"Kernel",       "NexOS monolithic v0.1"},
        {"Compiler",     "GCC 14.3.0  (-O2 -ffreestanding)"},
        {"Assembler",    "NASM 2.16.03"},
        {"Boot",         "Multiboot2"},
        {"Graphics",     "VESA/QEMU linear framebuffer"},
        {"Networking",   "Simulated 802.11n + RTL8139"},
        {"Syscalls",     "462 Linux-compatible syscalls"},
        {"Built",        "2026"},
    };
    int nr=(int)(sizeof(rows)/sizeof(rows[0]));
    for(int i=0;i<nr;i++){
        uint32_t rb = (i%2)==0 ? C_CARD : C_BG;
        fb_fill_rect(wx+14, cy, SET_W-28, 22, rb);
        font_puts(wx+22,  cy+4, rows[i][0], C_DIM, rb);
        font_puts(wx+190, cy+4, rows[i][1], C_TXT, rb);
        cy += 22;
    }
}

/* Phase 4B presentation: the HTML Settings states use a split view with a
 * navigation rail and a padded card-based content area.  These painters keep
 * the existing state/backend functions above, but make all geometry derive
 * from the live client rectangle. */
static void settings_aurora_nav(settings_app_t *s, int wx, int wy, int ww, int ch) {
    int side = settings_sidebar_width(ww);
    if (side <= 0) return;
    uint32_t side_bg = aurora_color(AURORA_COLOR_INACTIVE);
    fb_fill_rect(wx, wy, side, ch, side_bg);
    fb_fill_rect(wx + side - 1, wy, 1, ch,
                 aurora_color(AURORA_COLOR_BORDER_SUBTLE));
    aurora_text(wx + 18, wy + 18, "SETTINGS", AURORA_TEXT_CAPTION, side_bg);
    for (int i = 0; i < TAB_COUNT; i++) {
        int ny = wy + 50 + i * 40;
        uint32_t state = (s->tab == i) ? AURORA_STATE_SELECTED : AURORA_STATE_NORMAL;
        aurora_list_row((aurora_rect_t){wx + 10, ny, side - 20, 34},
                        TAB_NAMES[i], NULL, state);
    }
}

static void settings_aurora_heading(int x, int y, const char *title) {
    aurora_text(x, y, title, AURORA_TEXT_SECTION,
                aurora_color(AURORA_COLOR_BACKGROUND));
    aurora_text(x, y + 27, "NexOS system preferences", AURORA_TEXT_CAPTION,
                aurora_color(AURORA_COLOR_BACKGROUND));
}

static void settings_aurora_wifi(settings_app_t *s, int x, int y, int w, int h) {
    int cy = y + 68;
    int inner = w - 52;
    settings_aurora_heading(x, y + 26, "Wi-Fi");
    if (inner < 180) inner = w - 24;

    if (wifi_is_connected()) {
        aurora_card((aurora_rect_t){x, cy, inner, 64}, AURORA_STATE_FOCUSED);
        aurora_signal_indicator(x + 18, cy + 20, wifi_get_signal(), 1);
        aurora_text(x + 54, cy + 12, "Connected", AURORA_TEXT_CAPTION,
                    aurora_color(AURORA_COLOR_SELECTED));
        aurora_text(x + 54, cy + 31, wifi_get_ssid(), AURORA_TEXT_BODY_EMPHASIS,
                    aurora_color(AURORA_COLOR_SELECTED));
        if (w > 360)
            aurora_button((aurora_rect_t){x + inner - 118, cy + 16, 104, 32},
                          "Disconnect", AURORA_BUTTON_DANGER,
                          s->hover_disconnect ? AURORA_STATE_HOVER : 0);
    } else {
        aurora_card((aurora_rect_t){x, cy, inner, 64}, 0);
        aurora_badge((aurora_rect_t){x + 16, cy + 14, 98, 24},
                     "Disconnected", AURORA_COLOR_DANGER);
        aurora_text(x + 126, cy + 17, "Choose a network to get online.",
                    AURORA_TEXT_BODY, aurora_color(AURORA_COLOR_SURFACE));
    }
    cy += 82;
    aurora_text(x, cy, "Available networks", AURORA_TEXT_LABEL,
                aurora_color(AURORA_COLOR_BACKGROUND));
    cy += 30;
    for (int i = 0; i < s->ap_count && i < AP_VISIBLE; i++) {
        int selected = s->selected_ap == i;
        int row_w = inner;
        uint32_t state = selected ? AURORA_STATE_SELECTED :
                         (s->hover_ap == i ? AURORA_STATE_HOVER : 0);
        aurora_card((aurora_rect_t){x, cy, row_w, AP_ROW_H}, state);
        aurora_signal_indicator(x + 14, cy + 7, s->aps[i].signal,
                                wifi_is_connected() && sseq(wifi_get_ssid(), s->aps[i].ssid));
        aurora_text(x + 52, cy + 7, s->aps[i].ssid, AURORA_TEXT_LABEL,
                    aurora_color((state & AURORA_STATE_SELECTED) ?
                                 AURORA_COLOR_SELECTED : AURORA_COLOR_SURFACE));
        if (w > 390) {
            char sig[12]; u32s((uint32_t)s->aps[i].signal, sig); append(sig, "%", 12);
            aurora_text(x + row_w - 62, cy + 8, sig, AURORA_TEXT_CAPTION,
                        aurora_color(AURORA_COLOR_TEXT_MUTED));
        }
        cy += AP_ROW_H + 4;
    }
    cy += 8;
    if (s->selected_ap >= 0 && s->aps[s->selected_ap].encrypted) {
        aurora_text(x, cy + 10, "Password", AURORA_TEXT_CAPTION,
                    aurora_color(AURORA_COLOR_TEXT_SECONDARY));
        aurora_text_field((aurora_rect_t){x + 76, cy, inner - 76, 36},
                          s->password, s->pwd_focus ? AURORA_STATE_FOCUSED : 0, 1);
        cy += 46;
    }
    if (s->selected_ap >= 0)
        aurora_button((aurora_rect_t){x, cy, 112, 36}, "Connect",
                      AURORA_BUTTON_PRIMARY,
                      s->hover_connect ? AURORA_STATE_HOVER : 0);
    if (w > 260)
        aurora_button((aurora_rect_t){x + inner - 108, cy, 108, 36}, "Re-scan",
                      AURORA_BUTTON_SECONDARY,
                      s->hover_rescan ? AURORA_STATE_HOVER : 0);
    if (s->msg_timer > 0 && s->msg_buf[0])
        aurora_text(x + 122, cy + 11, s->msg_buf, AURORA_TEXT_CAPTION,
                    aurora_color(s->msg_ok ? AURORA_COLOR_SUCCESS : AURORA_COLOR_DANGER));
    (void)h;
}

static void settings_aurora_display(settings_app_t *s, int x, int y, int w) {
    (void)s;
    int gap = 14, card_w = (w - gap) / 2, cy = y + 76;
    settings_aurora_heading(x, y + 26, "Display");
    aurora_card((aurora_rect_t){x, cy, card_w, 88}, 0);
    aurora_text(x + 16, cy + 14, "RESOLUTION", AURORA_TEXT_CAPTION,
                aurora_color(AURORA_COLOR_TEXT_MUTED));
    char res[20]; u32s(fb.width, res); append(res, " x ", 20); u32s(fb.height, res);
    aurora_text(x + 16, cy + 38, res, AURORA_TEXT_SECTION,
                aurora_color(AURORA_COLOR_TEXT_PRIMARY));
    aurora_card((aurora_rect_t){x + card_w + gap, cy, card_w, 88}, 0);
    aurora_text(x + card_w + gap + 16, cy + 14, "COLOUR DEPTH", AURORA_TEXT_CAPTION,
                aurora_color(AURORA_COLOR_TEXT_MUTED));
    aurora_text(x + card_w + gap + 16, cy + 38, "32-bit", AURORA_TEXT_SECTION,
                aurora_color(AURORA_COLOR_TEXT_PRIMARY));
    cy += 104;
    aurora_card((aurora_rect_t){x, cy, card_w, 106}, 0);
    aurora_text(x + 16, cy + 14, "RENDERING", AURORA_TEXT_CAPTION,
                aurora_color(AURORA_COLOR_TEXT_MUTED));
    aurora_text(x + 16, cy + 38, "Software rasteriser", AURORA_TEXT_LABEL,
                aurora_color(AURORA_COLOR_TEXT_PRIMARY));
    aurora_text(x + 16, cy + 64, "No GPU acceleration", AURORA_TEXT_CAPTION,
                aurora_color(AURORA_COLOR_TEXT_SECONDARY));
    aurora_card((aurora_rect_t){x + card_w + gap, cy, card_w, 106}, 0);
    aurora_text(x + card_w + gap + 16, cy + 14, "BRIGHTNESS", AURORA_TEXT_CAPTION,
                aurora_color(AURORA_COLOR_TEXT_MUTED));
    aurora_text(x + card_w + gap + 16, cy + 38, "80%", AURORA_TEXT_SECTION,
                aurora_color(AURORA_COLOR_TEXT_PRIMARY));
    aurora_progress((aurora_rect_t){x + card_w + gap + 16, cy + 76, card_w - 32, 6},
                    80, AURORA_COLOR_ACCENT);
}

static void settings_aurora_system(settings_app_t *s, int x, int y, int w) {
    (void)s;
    int gap = 14, card_w = (w - 2 * gap) / 3, cy = y + 76;
    char memory[32], network[32], uptime[32];
    uint32_t free_mb = (uint32_t)(pmm_get_free_frames() * 4 / 1024);
    u32s(free_mb, memory); append(memory, " MB free", sizeof(memory));
    if (wifi_is_connected() || (netif_find("eth0") && netif_is_up()))
        scpy(network, "Connected", sizeof(network));
    else
        scpy(network, "Offline", sizeof(network));
    u32s((uint32_t)(timer_get_ticks() / 1000), uptime);
    append(uptime, "s active", sizeof(uptime));
    settings_aurora_heading(x, y + 26, "System");
    const char *labels[] = { "MEMORY", "NETWORK", "UPTIME" };
    const char *values[] = { memory, network, uptime };
    for (int i = 0; i < 3; i++) {
        int cx = x + i * (card_w + gap);
        aurora_card((aurora_rect_t){cx, cy, card_w, 88}, 0);
        aurora_text(cx + 14, cy + 14, labels[i], AURORA_TEXT_CAPTION,
                    aurora_color(AURORA_COLOR_TEXT_MUTED));
        aurora_text(cx + 14, cy + 40, values[i], AURORA_TEXT_SECTION,
                    aurora_color(i == 1 ? AURORA_COLOR_SUCCESS : AURORA_COLOR_TEXT_PRIMARY));
    }
    cy += 108;
    aurora_card((aurora_rect_t){x, cy, w, 94}, 0);
    aurora_text(x + 16, cy + 14, "Power", AURORA_TEXT_SECTION,
                aurora_color(AURORA_COLOR_TEXT_PRIMARY));
    aurora_button((aurora_rect_t){x + 16, cy + 48, 92, 32}, "Restart",
                  AURORA_BUTTON_SECONDARY, 0);
    aurora_button((aurora_rect_t){x + 118, cy + 48, 104, 32}, "Shutdown",
                  AURORA_BUTTON_DANGER, 0);
}

static void settings_aurora_about(settings_app_t *s, int x, int y, int w) {
    (void)s;
    settings_aurora_heading(x, y + 26, "About");
    aurora_card((aurora_rect_t){x, y + 76, w, 250}, AURORA_STATE_FOCUSED);
    aurora_text(x + 22, y + 98, "NexOS", AURORA_TEXT_DISPLAY,
                aurora_color(AURORA_COLOR_ELEVATED));
    aurora_text(x + 22, y + 140, "0.1 Genesis", AURORA_TEXT_LABEL,
                aurora_color(AURORA_COLOR_ACCENT));
    const char *labels[] = { "ARCHITECTURE", "KERNEL", "BOOT", "GRAPHICS", "NETWORKING", "SYSCALLS" };
    const char *values[] = { "x86_64", "NexOS Custom", "Multiboot2",
                             "VESA/QEMU framebuffer", "RTL8139 + simulated Wi-Fi",
                             "Linux-compatible" };
    for (int i = 0; i < 6; i++) {
        int col = i & 1, row = i / 2;
        int cx = x + 22 + col * (w / 2), cy = y + 182 + row * 36;
        aurora_text(cx, cy, labels[i], AURORA_TEXT_CAPTION,
                    aurora_color(AURORA_COLOR_TEXT_MUTED));
        aurora_text(cx, cy + 16, values[i], AURORA_TEXT_CAPTION,
                    aurora_color(AURORA_COLOR_TEXT_SECONDARY));
    }
}

static void settings_paint(window_t *win) {
    settings_app_t *s = (settings_app_t *)win->userdata;
    if(!s) return;
    int wx   = win->x;
    int wy_c = win->y + WM_TITLEBAR_H;
    int client_h = win->h - WM_TITLEBAR_H;
    int side = settings_sidebar_width(win->w);
    int content_x = wx + side;
    int content_w = win->w - side;
    uint32_t bg = aurora_color(AURORA_COLOR_BACKGROUND);
    fb_fill_rect(wx, wy_c, win->w, client_h, bg);
    settings_aurora_nav(s, wx, wy_c, win->w, client_h);
    if (content_w < 1) return;
    fb_fill_rect(content_x, wy_c, content_w, client_h, bg);
    int pad = content_w >= 480 ? 26 : content_w >= 300 ? 16 : 8;
    int x = content_x + pad;
    int w = content_w - pad * 2;
    if (w < 80) w = content_w - 12;
    switch(s->tab) {
    case 0: settings_aurora_wifi(s, x, wy_c, w, client_h); break;
    case 1: settings_aurora_display(s, x, wy_c, w); break;
    case 2: settings_aurora_system(s, x, wy_c, w); break;
    case 3: settings_aurora_about(s, x, wy_c, w); break;
    }
}

static int in_rect(int px, int py, int rx, int ry, int rw, int rh) {
    return px>=rx && px<rx+rw && py>=ry && py<ry+rh;
}
static int ap_list_cy(void) { return CONTENT_Y + 58 + 20; }
static int after_ap_cy(void) { return ap_list_cy() + AP_VISIBLE * AP_ROW_H + 8; }

static void settings_click(window_t *win, int cx, int cy, int btn) {
    (void)btn;
    settings_app_t *s = (settings_app_t *)win->userdata;
    if(!s) return;
    {
        int side = settings_sidebar_width(win->w);
        if (side > 0 && cx < side && cy >= 50 && cy < 50 + TAB_COUNT * 40) {
            int newtab = (cy - 50) / 40;
            if (newtab >= 0 && newtab < TAB_COUNT) {
                s->tab = newtab;
                if (newtab == 0) {
                    s->ap_count = wifi_scan(s->aps, WIFI_MAX_APS);
                    s->selected_ap = -1; s->pwd_len = 0;
                    s->password[0] = 0; s->pwd_focus = 0;
                }
            }
            return;
        }
    }
    if(s->tab==2) {
        int side = settings_sidebar_width(win->w);
        int content_w = win->w - side;
        int pad = content_w >= 480 ? 26 : content_w >= 300 ? 16 : 8;
        int x = side + pad;
        int w = content_w - pad * 2;
        int pcy = 76 + 108 + 48;
        if(in_rect(cx,cy, x + 16, pcy, 92, 32)) {
            __asm__ volatile("mov $0xFE,%%al; out %%al,$0x64" ::: "eax");
            return;
        }
        if(in_rect(cx,cy, x + 118, pcy, 104, 32)) {
            __asm__ volatile("mov $0x2000,%%ax; mov $0x604,%%dx; outw %%ax,%%dx" ::: "eax","edx");
            return;
        }
        return;
    }
    if(s->tab != 0) return;
    int side = settings_sidebar_width(win->w);
    int content_x = side;
    int content_w = win->w - side;
    int pad = content_w >= 480 ? 26 : content_w >= 300 ? 16 : 8;
    int x = content_x + pad;
    int w = content_w - pad * 2;
    int cy0 = 68;
    int inner = w - 52;
    if (inner < 180) inner = w - 24;
    if(wifi_is_connected()) {
        if(in_rect(cx,cy, x + inner - 118, cy0 + 16, 104, 32)) {
            wifi_disconnect();
            scpy(s->msg_buf, "Disconnected.", 64);
            s->msg_ok    = 0;
            s->msg_timer = 120;
            return;
        }
    }
    int acy = cy0 + 82 + 30;
    for(int i=0;i<s->ap_count&&i<AP_VISIBLE;i++) {
        int ry = acy + i*AP_ROW_H;
        if(in_rect(cx,cy, x, ry, inner, AP_ROW_H)) {
            if(s->selected_ap == i) {
                s->selected_ap = -1;
                s->pwd_focus   = 0;
            } else {
                s->selected_ap = i;
                s->pwd_focus   = s->aps[i].encrypted;
                s->pwd_len     = 0;
                s->password[0] = 0;
            }
            return;
        }
    }
    int aacy = acy + AP_VISIBLE * (AP_ROW_H + 4) + 8;
    if(s->selected_ap>=0 && s->aps[s->selected_ap].encrypted) {
        if(in_rect(cx,cy, x + 76, aacy, inner - 76, 36)) {
            s->pwd_focus = 1;
            return;
        }
        aacy += 36;
    }
    if(s->selected_ap>=0) {
        if(in_rect(cx,cy, x, aacy, 112, 36)) {
            int r = wifi_connect(s->aps[s->selected_ap].ssid, s->password);
            if(r==0) {
                scpy(s->msg_buf, "Connected!", 64);
                s->msg_ok    = 1;
                s->msg_timer = 150;
                notif_show("WiFi", s->aps[s->selected_ap].ssid, 3000);
            } else {
                scpy(s->msg_buf, "Connection failed", 64);
                s->msg_ok    = 0;
                s->msg_timer = 150;
            }
            s->selected_ap = -1;
            s->pwd_len     = 0;
            s->password[0] = 0;
            s->pwd_focus   = 0;
            return;
        }
    }
    if(in_rect(cx,cy, x + inner - 108, aacy, 108, 36)) {
        s->ap_count    = wifi_scan(s->aps, WIFI_MAX_APS);
        s->selected_ap = -1;
        scpy(s->msg_buf, "Scan complete.", 64);
        s->msg_ok    = 1;
        s->msg_timer = 90;
    }
}

static void settings_key(window_t *win, char key) {
    settings_app_t *s = (settings_app_t *)win->userdata;
    if(!s || s->tab!=0 || !s->pwd_focus) return;
    if(key=='\b' || key==127) {
        if(s->pwd_len>0) { s->pwd_len--; s->password[s->pwd_len]=0; }
    } else if(key=='\r' || key=='\n') {
        if(s->selected_ap>=0) {
            int r = wifi_connect(s->aps[s->selected_ap].ssid, s->password);
            if(r==0) {
                scpy(s->msg_buf,"Connected!",64);
                s->msg_ok=1; s->msg_timer=150;
                notif_show("WiFi", s->aps[s->selected_ap].ssid, 3000);
            } else {
                scpy(s->msg_buf,"Connection failed",64);
                s->msg_ok=0; s->msg_timer=150;
            }
            s->selected_ap=-1; s->pwd_len=0;
            s->password[0]=0;  s->pwd_focus=0;
        }
    } else if(key>=32 && key<127) {
        if(s->pwd_len<63) {
            s->password[s->pwd_len++]=key;
            s->password[s->pwd_len]=0;
        }
    }
}

static void settings_close(window_t *win) {
    settings_app_t *s = (settings_app_t *)win->userdata;
    if(s) { kfree(s); win->userdata = NULL; }
    wm_close(win);
}

settings_app_t *settings_create(int x, int y) {
    settings_app_t *s = (settings_app_t *)kmalloc(sizeof(settings_app_t));
    if(!s) return NULL;
    mzero(s, (int)sizeof(settings_app_t));
    s->tab         =  0;
    s->selected_ap = -1;
    s->hover_tab   = -1;
    s->hover_ap    = -1;
    s->ap_count = wifi_scan(s->aps, WIFI_MAX_APS);
    window_t *win = wm_new(x, y, SET_W, SET_H, "Settings");
    if(!win) { kfree(s); return NULL; }
    win->userdata  = s;
    s->win         = win;
    win->on_paint  = settings_paint;
    win->on_click  = settings_click;
    win->on_key    = settings_key;
    win->on_close  = settings_close;
    klog(LOG_INFO, "Settings: opened");
    return s;
}
