/* Minimal freestanding NetSurf frontend boundary for NexOS.
 *
 * This file deliberately contains no browser implementation.  It supplies
 * the upstream operation tables and maps their lifetime/size operations to
 * the existing NexOS heap and window environment.  Painting is added through
 * the existing NexOS/libnsfb surface as the next frontend increment.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#include "mm/heap.h"
#include "gui/wm.h"
#include "gui/browser_app.h"
#include "drivers/fb.h"
#include "drivers/font.h"
#include "drivers/timer.h"
#include "kernel/kernel.h"

#include "netsurf/netsurf.h"
#include "netsurf/browser_window.h"
#include "netsurf/window.h"
#include "netsurf/bitmap.h"
#include "netsurf/layout.h"
#include "netsurf/misc.h"
#include "netsurf/fetch.h"
#include "netsurf/plot_style.h"
#include "netsurf/plotters.h"
#include "utils/nsurl.h"
#include "utils/nsoption.h"
#include "desktop/save_pdf.h"
#include "desktop/hotlist.h"
#include "desktop/global_history.h"
#include "utils/file.h"

struct gui_window {
	struct browser_window *bw;
	window_t *window;
	int width;
	int height;
	int origin_x;
	int origin_y;
	int scroll_x;
	int scroll_y;
};

struct nexos_bitmap {
	uint32_t *pixels;
	int width;
	int height;
	bool opaque;
};

struct nexos_redraw {
	struct gui_window *gw;
	struct rect clip;
};

static int nexos_min(int a, int b) { return a < b ? a : b; }
static int nexos_max(int a, int b) { return a > b ? a : b; }
static unsigned long nexos_plot_rects;
static unsigned long nexos_plot_texts;
static unsigned long nexos_plot_bitmaps;
static window_t *nexos_host_window;
static int nexos_host_x;
static int nexos_host_y;
static int nexos_host_width;
static int nexos_host_height;
static struct gui_window *nexos_host_gui;
static uint64_t nexos_nav_start;
static int nexos_nav_active;
static int nexos_nav_invalidated;
static int nexos_nav_painted;
static unsigned long nexos_sched_scheduled;
static unsigned long nexos_sched_executed;
static uint64_t nexos_sched_max_lateness;

static void nexos_nav_trace(const char *stage)
{
	if (!nexos_nav_active) return;
	klog(LOG_INFO, "T+%llu %s",
		(unsigned long long)(timer_get_ticks() - nexos_nav_start), stage);
}

void nexos_netsurf_bind_window(window_t *window, int x, int y,
		int width, int height)
{
	nexos_host_window = window;
	nexos_host_x = x;
	nexos_host_y = y;
	nexos_host_width = width;
	nexos_host_height = height;
}

void nexos_netsurf_set_viewport(window_t *window, int x, int y,
		int width, int height)
{
	if (window == NULL || window != nexos_host_window) return;
	nexos_host_x = x;
	nexos_host_y = y;
	nexos_host_width = width;
	nexos_host_height = height;
	if (nexos_host_gui != NULL) {
		nexos_host_gui->origin_x = x;
		nexos_host_gui->origin_y = y;
		nexos_host_gui->width = width;
		nexos_host_gui->height = height;
		/* set_dimensions is only valid for core-managed windows. */
		browser_window_reformat(nexos_host_gui->bw, false, width, height);
	}
}

bool nexos_netsurf_scroll(window_t *window, int dx, int dy)
{
	static int scroll_diag_budget = 16;
	bool handled;
	if (window == NULL || window != nexos_host_window ||
	    nexos_host_gui == NULL || nexos_host_gui->bw == NULL)
		return false;
	handled = browser_window_scroll_at_point(nexos_host_gui->bw, 1, 1, dx, dy);
	if (scroll_diag_budget > 0) {
		klog(LOG_DEBUG, "NETSURF SCROLL dx=%d dy=%d handled=%d actual=(%d,%d)",
			dx, dy, handled ? 1 : 0, nexos_host_gui->scroll_x,
			nexos_host_gui->scroll_y);
		scroll_diag_budget--;
	}
	return handled;
}

void nexos_netsurf_mouse_track(window_t *window, int x, int y)
{
	static int diag_budget = 8;
	if (window == NULL || window != nexos_host_window ||
	    nexos_host_gui == NULL || nexos_host_gui->bw == NULL)
		return;
	browser_window_mouse_track(nexos_host_gui->bw, 0, x, y);
	if (diag_budget > 0) {
		klog(LOG_DEBUG, "INPUT NETSURF move x=%d y=%d", x, y);
		diag_budget--;
	}
}

static void nexos_clip_rect(const struct nexos_redraw *rd,
		int *x0, int *y0, int *x1, int *y1)
{
	*x0 = nexos_max(*x0, rd->clip.x0);
	*y0 = nexos_max(*y0, rd->clip.y0);
	*x1 = nexos_min(*x1, rd->clip.x1);
	*y1 = nexos_min(*y1, rd->clip.y1);
}

static uint32_t nexos_plot_colour(colour c)
{
	/* NexOS and NetSurf both use 0x00RRGGBB for opaque display colours. */
	return c & 0x00ffffffu;
}

static nserror nexos_plot_clip(const struct redraw_context *ctx,
		const struct rect *clip)
{
	struct nexos_redraw *rd = ctx->priv;
	if (rd == NULL || clip == NULL) return NSERROR_BAD_PARAMETER;
	rd->clip = *clip;
	return NSERROR_OK;
}

static nserror nexos_plot_rectangle(const struct redraw_context *ctx,
		const plot_style_t *style, const struct rect *rect)
{
	struct nexos_redraw *rd = ctx->priv;
	int x0, y0, x1, y1;
	if (rd == NULL || style == NULL || rect == NULL) return NSERROR_BAD_PARAMETER;
	x0 = rect->x0; y0 = rect->y0; x1 = rect->x1; y1 = rect->y1;
	nexos_clip_rect(rd, &x0, &y0, &x1, &y1);
	if (x1 <= x0 || y1 <= y0) return NSERROR_OK;
	if (style->fill_type != PLOT_OP_TYPE_NONE)
		nexos_plot_rects++;
	if (style->fill_type != PLOT_OP_TYPE_NONE)
				fb_fill_rect(rd->gw->window->x + rd->gw->origin_x + x0,
					rd->gw->window->y + WM_TITLEBAR_H + rd->gw->origin_y + y0,
				x1 - x0, y1 - y0, nexos_plot_colour(style->fill_colour));
	if (style->stroke_type != PLOT_OP_TYPE_NONE) {
				fb_draw_rect_outline(rd->gw->window->x + rd->gw->origin_x + x0,
					rd->gw->window->y + WM_TITLEBAR_H + rd->gw->origin_y + y0,
				x1 - x0, y1 - y0,
				nexos_plot_colour(style->stroke_colour),
				plot_style_fixed_to_int(style->stroke_width));
	}
	return NSERROR_OK;
}

static nserror nexos_plot_line(const struct redraw_context *ctx,
		const plot_style_t *style, const struct rect *line)
{
	struct nexos_redraw *rd = ctx->priv;
	if (rd == NULL || style == NULL || line == NULL) return NSERROR_BAD_PARAMETER;
	if (style->stroke_type != PLOT_OP_TYPE_NONE)
				fb_draw_line(rd->gw->window->x + rd->gw->origin_x + line->x0,
					rd->gw->window->y + WM_TITLEBAR_H + rd->gw->origin_y + line->y0,
					rd->gw->window->x + rd->gw->origin_x + line->x1,
					rd->gw->window->y + WM_TITLEBAR_H + rd->gw->origin_y + line->y1,
				nexos_plot_colour(style->stroke_colour));
	return NSERROR_OK;
}

static nserror nexos_plot_disc(const struct redraw_context *ctx,
		const plot_style_t *style, int x, int y, int radius)
{
	struct nexos_redraw *rd = ctx->priv;
	if (rd == NULL || style == NULL) return NSERROR_BAD_PARAMETER;
	if (style->fill_type != PLOT_OP_TYPE_NONE)
			fb_fill_circle(rd->gw->window->x + rd->gw->origin_x + x,
				rd->gw->window->y + WM_TITLEBAR_H + rd->gw->origin_y + y, radius,
			nexos_plot_colour(style->fill_colour));
	return NSERROR_OK;
}

static nserror nexos_plot_arc(const struct redraw_context *ctx,
		const plot_style_t *style, int x, int y, int radius,
		int angle1, int angle2)
{
	(void)angle1; (void)angle2;
	return nexos_plot_disc(ctx, style, x, y, radius);
}

static nserror nexos_plot_polygon(const struct redraw_context *ctx,
		const plot_style_t *style, const int *p, unsigned int n)
{
	struct nexos_redraw *rd = ctx->priv;
	unsigned int i;
	if (rd == NULL || style == NULL || p == NULL || n < 3)
		return NSERROR_BAD_PARAMETER;
	/* The first-page target uses polygonal decorations sparingly.  Draw a
	 * clipped outline using the existing framebuffer primitive. */
	if (style->stroke_type != PLOT_OP_TYPE_NONE) {
		for (i = 0; i < n; i++) {
			unsigned int j = (i + 1) % n;
				fb_draw_line(rd->gw->window->x + rd->gw->origin_x + p[i * 2],
					rd->gw->window->y + WM_TITLEBAR_H + rd->gw->origin_y + p[i * 2 + 1],
					rd->gw->window->x + rd->gw->origin_x + p[j * 2],
					rd->gw->window->y + WM_TITLEBAR_H + rd->gw->origin_y + p[j * 2 + 1],
				nexos_plot_colour(style->stroke_colour));
		}
	}
	return NSERROR_OK;
}

static nserror nexos_plot_path(const struct redraw_context *ctx,
		const plot_style_t *style, const float *p, unsigned int n,
		const float transform[6])
{
	(void)ctx; (void)style; (void)p; (void)n; (void)transform;
	return NSERROR_OK;
}

static nserror nexos_plot_bitmap(const struct redraw_context *ctx,
		struct bitmap *bitmap, int x, int y, int width, int height,
		colour bg, bitmap_flags_t flags)
{
	struct nexos_redraw *rd = ctx->priv;
	struct nexos_bitmap *bm = (struct nexos_bitmap *)bitmap;
	int sx, sy, dx, dy;
	(void)bg; (void)flags;
	if (rd == NULL || bm == NULL || bm->pixels == NULL || width <= 0 || height <= 0)
		return NSERROR_BAD_PARAMETER;
	nexos_plot_bitmaps++;
	for (dy = 0; dy < height; dy++) {
		int py = y + dy;
		if (py < rd->clip.y0 || py >= rd->clip.y1) continue;
		sy = (dy * bm->height) / height;
		for (dx = 0; dx < width; dx++) {
			int px = x + dx;
			uint32_t c;
			if (px < rd->clip.x0 || px >= rd->clip.x1) continue;
			sx = (dx * bm->width) / width;
			c = bm->pixels[sy * bm->width + sx];
			if ((c >> 24) != 0)
				fb_put_pixel(rd->gw->window->x + rd->gw->origin_x + px,
					rd->gw->window->y + WM_TITLEBAR_H + rd->gw->origin_y + py,
					c & 0x00ffffffu);
		}
	}
	return NSERROR_OK;
}

static nserror nexos_plot_text(const struct redraw_context *ctx,
		const plot_font_style_t *style, int x, int y,
		const char *text, size_t length)
{
	struct nexos_redraw *rd = ctx->priv;
	size_t i;
	int px = x;
	uint32_t fg = style ? nexos_plot_colour(style->foreground) : 0;
	if (rd == NULL || text == NULL) return NSERROR_BAD_PARAMETER;
	nexos_plot_texts++;
	/* NexOS currently exposes its existing 8x16 bitmap font.  Preserve the
	 * upstream text callback and render the UTF-8 ASCII subset directly while
	 * retaining NetSurf's real layout/paint decisions. */
	for (i = 0; i < length; i++) {
		unsigned char ch = (unsigned char)text[i];
		if (ch == '\n') { px = x; y += 16; continue; }
		if (ch < 0x80 && px + 8 > rd->clip.x0 && px < rd->clip.x1 &&
				y - 12 < rd->clip.y1 && y + 4 >= rd->clip.y0)
				font_putchar(rd->gw->window->x + rd->gw->origin_x + px,
					rd->gw->window->y + WM_TITLEBAR_H + rd->gw->origin_y + y - 12,
				(char)ch, fg, 0);
		px += 8;
	}
	return NSERROR_OK;
}

static const struct plotter_table nexos_plotters = {
	.clip = nexos_plot_clip,
	.arc = nexos_plot_arc,
	.disc = nexos_plot_disc,
	.line = nexos_plot_line,
	.rectangle = nexos_plot_rectangle,
	.polygon = nexos_plot_polygon,
	.path = nexos_plot_path,
	.bitmap = nexos_plot_bitmap,
	.text = nexos_plot_text,
	.option_knockout = false,
};

void nexos_netsurf_paint(window_t *window)
{
	struct gui_window *gw;
	struct nexos_redraw rd;
	struct redraw_context ctx;
	struct rect clip;

	if (window == NULL || nexos_host_gui == NULL ||
	    nexos_host_gui->window != window) return;
	gw = nexos_host_gui;
	clip.x0 = 0;
	clip.y0 = 0;
	clip.x1 = gw->width;
	clip.y1 = gw->height;
	rd.gw = gw;
	rd.clip = clip;
	ctx.interactive = true;
	ctx.background_images = true;
	ctx.plot = &nexos_plotters;
	ctx.priv = &rd;
	if (!nexos_nav_painted) {
		nexos_nav_painted = 1;
		nexos_nav_trace("FIRST VISIBLE PAINT");
		klog(LOG_INFO, "T+%llu SCHEDULER scheduled=%u executed=%u max_lateness=%llu",
			(unsigned long long)(timer_get_ticks() - nexos_nav_start),
			(unsigned int)nexos_sched_scheduled,
			(unsigned int)nexos_sched_executed,
			(unsigned long long)nexos_sched_max_lateness);
	}
	static int paint_diag_budget = 2;
	if (paint_diag_budget > 0) {
		klog(LOG_INFO, "NETSURF FRAMEBUFFER PAINT width=%d height=%d",
			(int64_t)clip.x1, (int64_t)clip.y1);
		paint_diag_budget--;
	}
	static int redraw_start_budget = 8;
	if (redraw_start_budget > 0) {
		nexos_nav_trace("NETSURF REDRAW START");
		redraw_start_budget--;
	}
	if (browser_window_redraw(gw->bw, gw->scroll_x, gw->scroll_y,
			&clip, &ctx)) {
		static int redraw_diag_budget = 8;
		if (redraw_diag_budget > 0) {
			nexos_nav_trace("NETSURF REDRAW COMPLETE");
			redraw_diag_budget--;
		}
		if (paint_diag_budget > 0) {
			klog(LOG_INFO, "NETSURF PLOT PASS rect=%u text=%u bitmap=%u pixel=%x",
				(unsigned int)nexos_plot_rects,
				(unsigned int)nexos_plot_texts,
				(unsigned int)nexos_plot_bitmaps,
				(unsigned int)fb_get_pixel(window->x + 12,
					window->y + WM_TITLEBAR_H + 12));
			paint_diag_budget--;
		}
	}
}

static bool nexos_registered;
#define NEXOS_NETSURF_TIMER_COUNT 16

struct nexos_timer_slot {
	void (*callback)(void *);
	void *pw;
	uint64_t due;
	bool active;
};

static struct nexos_timer_slot nexos_timers[NEXOS_NETSURF_TIMER_COUNT];

nserror nexos_netsurf_open_url(const char *address,
		struct browser_window **out);

static nserror nexos_set_defaults(struct nsoption_s *defaults)
{
	/* Keep upstream option keys/defaults, overriding only the system colours
	 * needed before ns_system_colour_init().  No hosted Choices file exists
	 * in the first NexOS milestone. */
	defaults[NSOPTION_sys_colour_Canvas].value.c = 0x00aaaaaa;
	defaults[NSOPTION_sys_colour_CanvasText].value.c = 0x00000000;
	defaults[NSOPTION_sys_colour_Highlight].value.c = 0x00ee0000;
	defaults[NSOPTION_sys_colour_HighlightText].value.c = 0x00000000;
	return NSERROR_OK;
}

static nserror nexos_file_unavailable(const char *path, char **out,
		size_t *size)
{
	(void)path;
	(void)out;
	(void)size;
	return NSERROR_NOT_IMPLEMENTED;
}

static nserror nexos_file_mkpath(char **out, size_t *size,
		size_t count, va_list ap)
{
	(void)out;
	(void)size;
	(void)count;
	(void)ap;
	return NSERROR_NOT_IMPLEMENTED;
}

static nserror nexos_file_mkdir(const char *path)
{
	(void)path;
	return NSERROR_NOT_IMPLEMENTED;
}

static nserror nexos_file_url_to_path(struct nsurl *url, char **out)
{
	(void)url;
	(void)out;
	return NSERROR_NOT_IMPLEMENTED;
}

static nserror nexos_file_path_to_url(const char *path, struct nsurl **out)
{
	(void)path;
	(void)out;
	return NSERROR_NOT_IMPLEMENTED;
}

static struct gui_file_table nexos_file_table = {
	.mkpath = nexos_file_mkpath,
	.basename = nexos_file_unavailable,
	.nsurl_to_path = nexos_file_url_to_path,
	.path_to_nsurl = nexos_file_path_to_url,
	.mkdir_all = nexos_file_mkdir,
};

/* gui_factory fills this default when a frontend does not provide a file
 * table.  Export the NexOS table so no hosted POSIX file implementation is
 * pulled into the freestanding image. */
struct gui_file_table *default_file_table = &nexos_file_table;

/* Optional desktop persistence/export services are not part of the first
 * NexOS browser milestone.  Keep the upstream engine's callbacks valid and
 * report unsupported operations through their normal NetSurf result types. */
nserror save_pdf(const char *path)
{
	(void)path;
	return NSERROR_NOT_IMPLEMENTED;
}

void hotlist_update_url(struct nsurl *url)
{
	(void)url;
}

nserror global_history_add(struct nsurl *url)
{
	(void)url;
	return NSERROR_OK;
}

static nserror nexos_schedule(int timeout, void (*callback)(void *), void *pw)
{
	int free_slot = -1;
	int i;

	if (callback == NULL) return NSERROR_BAD_PARAMETER;
	if (nexos_nav_active) nexos_sched_scheduled++;

	/* NetSurf permits several independent scheduled callbacks.  Reschedule
	 * only the matching callback/context pair; unrelated callbacks must not
	 * cancel one another (for example llcache drain versus fetch cleanup). */
	for (i = 0; i < NEXOS_NETSURF_TIMER_COUNT; i++) {
		if (!nexos_timers[i].active) {
			if (free_slot < 0) free_slot = i;
			continue;
		}
		if (nexos_timers[i].callback == callback &&
		    nexos_timers[i].pw == pw) {
			if (timeout < 0) {
				nexos_timers[i].active = false;
			} else {
				nexos_timers[i].due = timer_get_ticks() +
					(uint64_t)timeout;
			}
			return NSERROR_OK;
		}
	}

	if (timeout < 0) {
		return NSERROR_OK;
	}
	if (free_slot < 0) return NSERROR_NOMEM;
	nexos_timers[free_slot].callback = callback;
	nexos_timers[free_slot].pw = pw;
	nexos_timers[free_slot].due = timer_get_ticks() + (uint64_t)timeout;
	nexos_timers[free_slot].active = true;
	return NSERROR_OK;
}

void nexos_netsurf_pump(void)
{
	void (*callback)(void *);
	void *pw;
	uint64_t now = timer_get_ticks();
	int i;

	for (i = 0; i < NEXOS_NETSURF_TIMER_COUNT; i++) {
		if (!nexos_timers[i].active || now < nexos_timers[i].due)
			continue;
		callback = nexos_timers[i].callback;
		pw = nexos_timers[i].pw;
		if (nexos_nav_active) {
			uint64_t lateness = now - nexos_timers[i].due;
			nexos_sched_executed++;
			if (lateness > nexos_sched_max_lateness)
				nexos_sched_max_lateness = lateness;
		}
		nexos_timers[i].active = false;
		if (callback != NULL) callback(pw);
	}
}

static void nexos_deferred_open(void *pw)
{
	struct browser_window *bw = NULL;
	(void)nexos_netsurf_open_url((const char *)pw, &bw);
}

void nexos_netsurf_schedule_url(const char *address, int delay_ms)
{
	if (address != NULL) (void)nexos_schedule(delay_ms, nexos_deferred_open,
			(void *)address);
}

static struct gui_window *nexos_window_create(struct browser_window *bw,
		struct gui_window *existing, gui_window_create_flags flags)
{
	struct gui_window *gw;
	(void)existing;
	(void)flags;

	gw = kmalloc(sizeof(*gw));
	if (gw == NULL) return NULL;
	gw->bw = bw;
	gw->width = nexos_host_window != NULL ? nexos_host_width : 720;
	gw->height = nexos_host_window != NULL ? nexos_host_height : 428;
	gw->origin_x = nexos_host_window != NULL ? nexos_host_x : 0;
	gw->origin_y = nexos_host_window != NULL ? nexos_host_y : 0;
	gw->scroll_x = 0;
	gw->scroll_y = 0;
	gw->window = nexos_host_window != NULL ? nexos_host_window :
		wm_new(80, 50, 720, 460, "NexOS Browser");
	if (gw->window == NULL) {
		kfree(gw);
		return NULL;
	}
	nexos_host_gui = gw;
	if (nexos_host_window == NULL) {
		gw->window->userdata = gw;
		gw->window->on_paint = nexos_netsurf_paint;
	}
	return gw;
}

static void nexos_window_destroy(struct gui_window *gw)
{
	if (gw == NULL) return;
	if (gw == nexos_host_gui) {
		nexos_host_gui = NULL;
		nexos_host_window = NULL;
	} else if (gw->window != NULL) {
		wm_close(gw->window);
	}
	kfree(gw);
}

static nserror nexos_window_invalidate(struct gui_window *gw,
		const struct rect *rect)
{
	(void)rect;
	if (!nexos_nav_invalidated) {
		nexos_nav_invalidated = 1;
		nexos_nav_trace("FIRST PAINT REQUEST");
	}
	if (gw != NULL && gw->window != NULL) wm_invalidate(gw->window);
	return NSERROR_OK;
}

static bool nexos_window_get_scroll(struct gui_window *gw, int *x, int *y)
{
	if (gw == NULL || x == NULL || y == NULL) return false;
	*x = gw->scroll_x;
	*y = gw->scroll_y;
	return true;
}

static nserror nexos_window_set_scroll(struct gui_window *gw,
		const struct rect *rect)
{
	if (gw == NULL || rect == NULL) return NSERROR_BAD_PARAMETER;
	gw->scroll_x = rect->x0;
	gw->scroll_y = rect->y0;
	if (gw->window != NULL) wm_invalidate(gw->window);
	return NSERROR_OK;
}

static nserror nexos_window_dimensions(struct gui_window *gw,
		int *width, int *height)
{
	if (gw == NULL || width == NULL || height == NULL)
		return NSERROR_BAD_PARAMETER;
	*width = gw->width;
	*height = gw->height;
	return NSERROR_OK;
}

static nserror nexos_window_event(struct gui_window *gw,
		enum gui_window_event event)
{
	(void)gw;
	(void)event;
	return NSERROR_OK;
}

static void nexos_window_set_title(struct gui_window *gw, const char *title)
{
	(void)gw;
	(void)title;
}

static nserror nexos_window_set_url(struct gui_window *gw, struct nsurl *url)
{
	if (gw != NULL && gw->window != NULL && url != NULL)
		browser_netsurf_set_url(gw->window, nsurl_access(url));
	return NSERROR_OK;
}

static void nexos_window_set_status(struct gui_window *gw, const char *text)
{
	if (gw != NULL && gw->window != NULL)
		browser_netsurf_set_status(gw->window, text);
}

static struct gui_window_table nexos_window_table = {
	.create = nexos_window_create,
	.destroy = nexos_window_destroy,
	.invalidate = nexos_window_invalidate,
	.get_scroll = nexos_window_get_scroll,
	.set_scroll = nexos_window_set_scroll,
	.get_dimensions = nexos_window_dimensions,
	.event = nexos_window_event,
	.set_title = nexos_window_set_title,
	.set_url = nexos_window_set_url,
	.set_status = nexos_window_set_status,
};

static void *nexos_bitmap_create(int width, int height,
		enum gui_bitmap_flags flags)
{
	struct nexos_bitmap *bitmap;
	size_t count;

	if (width <= 0 || height <= 0) return NULL;
	bitmap = kmalloc(sizeof(*bitmap));
	if (bitmap == NULL) return NULL;
	count = (size_t)width * (size_t)height;
	bitmap->pixels = kmalloc(count * sizeof(*bitmap->pixels));
	if (bitmap->pixels == NULL) {
		kfree(bitmap);
		return NULL;
	}
	bitmap->width = width;
	bitmap->height = height;
	bitmap->opaque = (flags & BITMAP_OPAQUE) != 0;
	return bitmap;
}

static void nexos_bitmap_destroy(void *ptr)
{
	struct nexos_bitmap *bitmap = ptr;
	if (bitmap == NULL) return;
	kfree(bitmap->pixels);
	kfree(bitmap);
}

static void nexos_bitmap_set_opaque(void *ptr, bool opaque)
{
	((struct nexos_bitmap *)ptr)->opaque = opaque;
}

static bool nexos_bitmap_get_opaque(void *ptr)
{
	return ((struct nexos_bitmap *)ptr)->opaque;
}

static unsigned char *nexos_bitmap_buffer(void *ptr)
{
	return (unsigned char *)((struct nexos_bitmap *)ptr)->pixels;
}

static size_t nexos_bitmap_stride(void *ptr)
{
	return (size_t)((struct nexos_bitmap *)ptr)->width * sizeof(uint32_t);
}

static int nexos_bitmap_width(void *ptr)
{
	return ((struct nexos_bitmap *)ptr)->width;
}

static int nexos_bitmap_height(void *ptr)
{
	return ((struct nexos_bitmap *)ptr)->height;
}

static void nexos_bitmap_modified(void *ptr)
{
	(void)ptr;
}

static nserror nexos_bitmap_render(struct bitmap *bitmap,
		struct hlcache_handle *content)
{
	(void)bitmap;
	(void)content;
	return NSERROR_NOT_IMPLEMENTED;
}

static struct gui_bitmap_table nexos_bitmap_table = {
	.create = nexos_bitmap_create,
	.destroy = nexos_bitmap_destroy,
	.set_opaque = nexos_bitmap_set_opaque,
	.get_opaque = nexos_bitmap_get_opaque,
	.get_buffer = nexos_bitmap_buffer,
	.get_rowstride = nexos_bitmap_stride,
	.get_width = nexos_bitmap_width,
	.get_height = nexos_bitmap_height,
	.modified = nexos_bitmap_modified,
	.render = nexos_bitmap_render,
};

static nserror nexos_text_width(const struct plot_font_style *style,
		const char *text, size_t length, int *width)
{
	int size = style == NULL ? 16 : plot_style_fixed_to_int(style->size);
	if (size < 1) size = 1;
	if (width == NULL) return NSERROR_BAD_PARAMETER;
	*width = (int)((length * (size + 1)) / 2);
	(void)text;
	return NSERROR_OK;
}

static nserror nexos_text_position(const struct plot_font_style *style,
		const char *text, size_t length, int x, size_t *offset, int *actual)
{
	int width;
	if (offset == NULL || actual == NULL) return NSERROR_BAD_PARAMETER;
	(void)nexos_text_width(style, text, length, &width);
	if (x <= 0) *offset = 0;
	else if (x >= width) *offset = length;
	else *offset = (size_t)((x * (int)length) / width);
	*actual = x;
	return NSERROR_OK;
}

static nserror nexos_text_split(const struct plot_font_style *style,
		const char *text, size_t length, int x, size_t *offset, int *actual)
{
	return nexos_text_position(style, text, length, x, offset, actual);
}

static struct gui_layout_table nexos_layout_table = {
	.width = nexos_text_width,
	.position = nexos_text_position,
	.split = nexos_text_split,
};

/* NetSurf's portable engine requests these built-in stylesheets through the
 * resource: scheme before it can convert an HTML DOM into layout boxes.  A
 * hosted frontend normally supplies them from its resource directory; the
 * freestanding image has no filesystem, so keep the small platform resource
 * set in the frontend boundary. */
static const uint8_t nexos_default_css[] =
	"html{display:block}head{display:none}body{display:block;margin:8px;line-height:1.33}"
	"div{display:block}p{display:block;margin:1em 0}h1{display:block;font-size:2em;font-weight:bold;margin:.67em 0}"
	"h2{display:block;font-size:1.5em;font-weight:bold;margin:.69em 0}h3{display:block;font-size:1.17em;font-weight:bold;margin:.83em 0}"
	"a{color:#0000ee;text-decoration:underline}strong{font-weight:bold}em{font-style:italic}"
	"ul,ol{display:block;margin:1em 0;padding-left:40px}li{display:list-item}blockquote{display:block;margin:1em 40px}"
	"br{display:block}img{display:inline}body{font-family:sans-serif;font-size:16px;color:#000;background:#fff}";

static const uint8_t nexos_quirks_css[] =
	"table{font-size:medium;font-style:normal;font-variant:normal;font-weight:normal}";

static const char *nexos_filetype(const char *path)
{
	const char *dot;
	if (path == NULL) return "application/octet-stream";
	dot = strrchr(path, '.');
	if (dot != NULL && strcmp(dot, ".css") == 0) return "text/css";
	if (dot != NULL && strcmp(dot, ".png") == 0) return "image/png";
	if (dot != NULL && (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0))
		return "text/html";
	return "application/octet-stream";
}

static nserror nexos_get_resource_data(const char *path,
		const uint8_t **data, size_t *size)
{
	if (path == NULL || data == NULL || size == NULL) return NSERROR_BAD_PARAMETER;
	if (strcmp(path, "default.css") == 0) {
		*data = nexos_default_css;
		*size = sizeof(nexos_default_css) - 1;
		return NSERROR_OK;
	}
	if (strcmp(path, "quirks.css") == 0) {
		*data = nexos_quirks_css;
		*size = sizeof(nexos_quirks_css) - 1;
		return NSERROR_OK;
	}
	return NSERROR_NOT_FOUND;
}

static nserror nexos_release_resource_data(const uint8_t *data)
{
	(void)data;
	return NSERROR_OK;
}

static struct gui_fetch_table nexos_fetch_table = {
	.filetype = nexos_filetype,
	.get_resource_data = nexos_get_resource_data,
	.release_resource_data = nexos_release_resource_data,
};

static struct gui_misc_table nexos_misc_table = {
	.schedule = nexos_schedule,
};

static struct netsurf_table nexos_table = {
	.misc = &nexos_misc_table,
	.window = &nexos_window_table,
	.fetch = &nexos_fetch_table,
	.file = &nexos_file_table,
	.bitmap = &nexos_bitmap_table,
	.layout = &nexos_layout_table,
};

nserror nexos_netsurf_init(void)
{
	nserror error;
	if (nexos_registered) return NSERROR_OK;
	error = netsurf_register(&nexos_table);
	if (error != NSERROR_OK) return error;
	error = nsoption_init(nexos_set_defaults, NULL, NULL);
	if (error != NSERROR_OK) return error;
	error = netsurf_init(NULL);
	if (error != NSERROR_OK) return error;
	nexos_registered = true;
	return NSERROR_OK;
}

nserror nexos_netsurf_open(struct nsurl *url, struct browser_window **out)
{
	if (out == NULL) return NSERROR_BAD_PARAMETER;
	if (!nexos_registered) {
		nserror error = nexos_netsurf_init();
		if (error != NSERROR_OK) return error;
	}
	return browser_window_create(BW_CREATE_HISTORY, url, NULL, NULL, out);
}

nserror nexos_netsurf_open_url(const char *address,
		struct browser_window **out)
{
	nsurl *url = NULL;
	nserror error;

	if (address == NULL || out == NULL) return NSERROR_BAD_PARAMETER;
	error = nsurl_create(address, &url);
	if (error != NSERROR_OK) return error;
	error = nexos_netsurf_open(url, out);
	nsurl_unref(url);
	return error;
}

nserror nexos_netsurf_navigate(struct browser_window *bw, const char *address)
{
	struct nsurl *url = NULL;
	nserror error;

	if (bw == NULL || address == NULL) return NSERROR_BAD_PARAMETER;
	nexos_nav_start = timer_get_ticks();
	nexos_nav_active = 1;
	nexos_nav_invalidated = 0;
	nexos_nav_painted = 0;
	nexos_sched_scheduled = 0;
	nexos_sched_executed = 0;
	nexos_sched_max_lateness = 0;
	klog(LOG_INFO, "T+0 NETSURF NAV START url=%s", address);
	klog(LOG_INFO, "NETSURF NAV START url=%s", address);
	error = nsurl_create(address, &url);
	if (error != NSERROR_OK) {
		klog(LOG_WARN, "NETSURF NAV URL ERROR=%d url=%s", (int)error, address);
		return error;
	}
	error = browser_window_navigate(bw, url, NULL,
		BW_NAVIGATE_HISTORY, NULL, NULL, NULL);
	klog(error == NSERROR_OK ? LOG_INFO : LOG_WARN,
		"NETSURF NAV %s error=%d url=%s",
		error == NSERROR_OK ? "QUEUED" : "FAILED", (int)error, address);
	nsurl_unref(url);
	return error;
}

void nexos_netsurf_detach_window(window_t *window)
{
	if (window == NULL || window != nexos_host_window) return;
	nexos_host_window = NULL;
	nexos_host_gui = NULL;
}

nserror nexos_netsurf_open_blank(struct browser_window **out)
{
	if (out == NULL) return NSERROR_BAD_PARAMETER;
	if (!nexos_registered) {
		nserror error = nexos_netsurf_init();
		if (error != NSERROR_OK) return error;
	}
	return browser_window_create(BW_CREATE_HISTORY, NULL, NULL, NULL, out);
}
