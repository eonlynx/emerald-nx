.section .text
.global switch_jump_to_nro
.type switch_jump_to_nro, %function
switch_jump_to_nro:
    /* NRO entry ABI: x0 = config entries, x1 = -1, x2 = entrypoint.
     * The game's _start saves LR as the loader return address, so use BL R
     * and land on switch_after_game if the game ever returns. */
    mov x9, x1
    mov x10, x2
    mov x1, #-1
    mov x2, x9
    mov sp, x10
    blr x2
    b switch_after_game
.size switch_jump_to_nro, . - switch_jump_to_nro
