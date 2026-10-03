/* NexOS — kernel/arch/x86_64/fpu.c | x86_64 FPU/SSE setup | MIT License */
#include "fpu.h"
#include "../../kernel.h"
#include <stdint.h>

#define CR0_MP  (1ULL << 1)
#define CR0_EM  (1ULL << 2)
#define CR4_OSFXSR      (1ULL << 9)
#define CR4_OSXMMEXCPT  (1ULL << 10)

void fpu_init(void) {
    uint64_t cr0;
    uint64_t cr4;

    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    __asm__ volatile ("mov %%cr4, %0" : "=r"(cr4));

    /* x86_64 requires SSE2. Allow native FPU instructions and ask the CPU
     * to report unmasked SSE exceptions through the normal exception path. */
    cr0 &= ~CR0_EM;
    cr0 |= CR0_MP;
    cr4 |= CR4_OSFXSR | CR4_OSXMMEXCPT;

    __asm__ volatile ("mov %0, %%cr0" :: "r"(cr0) : "memory");
    __asm__ volatile ("mov %0, %%cr4" :: "r"(cr4) : "memory");
    __asm__ volatile ("fninit");

    klog(LOG_INFO, "FPU/SSE enabled for NetSurf-compatible floating point");
}
