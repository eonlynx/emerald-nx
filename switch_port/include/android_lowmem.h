#ifndef SWITCH_SHIM_ANDROID_LOWMEM_H
#define SWITCH_SHIM_ANDROID_LOWMEM_H

#include <stddef.h>
#include <stdlib.h>
#include <malloc.h>

/* The loader maps the newlib heap below 4 GiB. Worker stacks therefore come
 * from the same low heap; no Android low-address arena is needed. */
static inline void *AndroidLow_Stack(size_t size)
{
    return memalign(0x1000, (size + 0xFFF) & ~(size_t)0xFFF);
}

static inline void AndroidLow_FreeStack(void *stack, size_t size)
{
    (void)size;
    free(stack);
}

#endif
