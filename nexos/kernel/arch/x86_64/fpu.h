/* NexOS — x86_64 floating-point/SSE enablement for hosted ports */
#ifndef NEXOS_FPU_H
#define NEXOS_FPU_H

/* Enable the architectural state required by NetSurf's floating-point
 * layout code. The current NexOS scheduler has one runnable kernel thread;
 * per-thread XSAVE state is required before multiple FP-using threads exist. */
void fpu_init(void);

#endif
