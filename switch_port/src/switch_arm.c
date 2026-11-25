/*
 * The one libnx facility the 3DS-compatible shim needs without including
 * switch.h: the virtual counter.  libnx implements armGetSystemTick the same
 * way; keeping it in its own unit avoids pulling switch/types.h (whose Result
 * is u32) into sources that use the 3DS shim's Result (s32).
 */
#include <stdint.h>

uint64_t armGetSystemTick(void) {
    uint64_t tick;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(tick));
    return tick;
}
