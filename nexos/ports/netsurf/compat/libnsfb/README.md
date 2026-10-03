# libnsfb NexOS surface

`nexos_surface.c` is the first real frontend adapter. It maps the upstream
libnsfb surface ABI onto `kernel/drivers/fb.{h,c}`, `keyboard.{h,c}`, and
`mouse.{h,c}`. The `NSFB_SURFACE_ABLE` slot is used as the existing unused
surface enum; the registered surface name is `nexos`, so the upstream
framebuffer executable is launched with `-f nexos`.

This adapter is not yet linked into the NexOS kernel or a NetSurf executable.
Its next integration step is to add this source to libnsfb's surface Makefile
under a `NETSURF_NEXOS` build flag and compile it with the NexOS include/root
paths. The existing host libnsfb build remains unchanged.
