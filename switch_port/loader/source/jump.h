#ifndef EMERALD_NX_JUMP_H
#define EMERALD_NX_JUMP_H

#include <switch.h>

/* x0 = config entries, x1 = entrypoint, x2 = stack top, x3 = process handle.
 * Never returns: the game owns the low stack from here on. */
void switch_jump_to_nro(ConfigEntry *entries, void *entry, void *stack_top,
                        Handle process) __attribute__((noreturn));

/* Runs on the game's low stack if the game exits through libnx. */
void switch_after_game(void) __attribute__((noreturn));

#endif
