/*
 * The one libnx facility the 3DS-compatible shim needs without including
 * switch.h: the virtual counter.  libnx implements armGetSystemTick the same
 * way; keeping it in its own unit avoids pulling switch/types.h (whose Result
 * is u32) into sources that use the 3DS shim's Result (s32).
 */
#include <errno.h>
#include <malloc.h>
#include <stddef.h>
#include <stdint.h>

uint64_t armGetSystemTick(void) {
    uint64_t tick;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(tick));
    return tick;
}

/* devkitA64's newlib does not export posix_memalign; the GLES pool allocator
 * in android_c3d.c uses it.  Keep the same alignment and error contract. */
int posix_memalign(void **memptr, size_t alignment, size_t size) {
    if (alignment < sizeof(void *) || (alignment & (alignment - 1)) != 0) {
        return EINVAL;
    }
    void *ptr = memalign(alignment, size);
    if (ptr == NULL) {
        return ENOMEM;
    }
    *memptr = ptr;
    return 0;
}
