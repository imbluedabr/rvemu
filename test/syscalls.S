/*
    handles user syscalls and dispatches the right syscall handler
*/
.global do_syscall

.section .text
    # a0 = arg0, a7 = syscall number
do_syscall:
    # Dispatch based on syscall number
    li t0, 1
    beq a7, t0, sys_putchar
    li t0, 2
    beq a7, t0, sys_exit

    # Bad syscall
    li a0, -1
    ret

    # int sys_putchar(char c)
sys_putchar:
    # Save regs based on calling convention
    addi sp, sp, -16
    sw s0, (sp)
    sw ra, 4(sp)

    call kputchar
    li a0, 0

    # Restore regs based on calling convention
    lw s0, (sp)
    lw ra, 4(sp)
    addi sp, sp, 16
    ret

    # [[noreturn]] void sys_exit()
sys_exit:
    # Just stop the emulator
    ebreak


