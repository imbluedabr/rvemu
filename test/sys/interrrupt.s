/*
    handle exceptions and interrupt by dispatching the right handler.
    interrupt handlers are stored in the irq_table, and the device belonging to that interrupt is stored in the irq_device_table.
*/

.section .data

#void (*irq_table[16])(struct device* dev)
irq_table:
    .space 64

#struct device* irq_device_table[16]
irq_device_table:
    .space 64

msg_exception:
    # "Exception 0x"
    .asciz "Exception 0x"

msg_fatal_noproc:
    # "Fatal exception\n"
    .asciz "Fatal: no active process\r\n"
msg_fatal_mexc:
    .asciz "Fatal: exception in M-mode\r\n"

hex_chars:
    # "0123456789abcdef"
    .asciz "0123456789ABCDEF"

.section .text
.extern kputs
.extern kputc
.extern syscall_handler
.extern ready_tail
.extern schedule_new_task

.global irq_init
.global irq_register
.global enter_user

    # void irq_init()
irq_init:
    # set mstatus.MPIE to enable interrupts
    li t0, 0x80
    csrrs t0, mstatus, t0
    la sp, __stack_top
    csrw mscratch, sp

    la t0, handler
    csrw mtvec, t0
    ret

    # void irq_register(int irq, void (*handler)(), struct device* dev)
irq_register:
    # MIE |= 0x10000 << irq
    li t0, 0x10000
    sll t0, t0, a0
    csrrs zero, mie, t0
    # irq_table[irq] = handler
    slli a0, a0, 2
    la t0, irq_table
    add t0, t0, a0
    sw a1, 0(t0)
    # irq_device_table[irq] = dev
    la t0, irq_device_table
    add t0, t0, a0
    sw a2, 0(t0)
    ret

    # void trap_main(struct regs *regs)
trap_main:
    # Save regs based on calling convention
    addi sp, sp, -16
    sw s0, (sp)
    sw ra, 4(sp)

    mv s0, a0
    csrr t0, mcause
    li t1, 11 # Environment call from User mode
    beq t0, t1, trap_mcall
    li t1, 8 # "Environment call from User mode"
    beq t0, t1, trap_ucall
    li t1, 0x80000000 # int bit set
    and t2, t0, t1
    beq t2, t1, trap_irq
    j do_bad_exception # unhandled exception

trap_irq:
    andi t0, t0, 0x1F
    addi t0, t0, -16
    slli t0, t0, 2
    la t1, irq_device_table
    add t1, t1, t0
    lw a0, 0(t1)
    la t1, irq_table
    add t1, t1, t0
    lw t1, 0(t1)
    jalr ra, t1, 0

    j trap_main_exit

trap_ucall:
    # ready_tail->save_pc = regs->pc
    la t0, ready_tail
    lw t0, 0(t0)
    lw t1, 0(s0)
    sw t1, 12(t0)

    # ready_tail->mode = KERNEL
    li t1, 3
    sb t1, 18(t0)

    # regs->pc = syscall_handler
    la t1, syscall_handler
    sw t1, 0(s0)
    j trap_main_exit

trap_mcall:
    # regs->pc = ready_tail->save_pc
    la t0, ready_tail
    lw t0, 0(t0)
    lw t1, 12(t0)
    sw t1, 0(s0)
    # ready_tail->save_pc = NULL
    sw zero, 12(t0)
    
    # ready_tail->mode = USER
    sb zero, 18(t0)
    
    # if (ready_tail->state == ZOMBIE) schedule_new_task()
    lbu t1, 17(t0)
    bne t1, zero, 1f
    call schedule_new_task
1:

    # Bump user pc by 4
    # Skip over ecall instruction
    lw t0, 0(s0)
    addi t0, t0, 4
    sw t0, 0(s0)
    
trap_main_exit:
    # Restore regs based on calling convention
    lw s0, (sp)
    lw ra, 4(sp)
    addi sp, sp, 16
    ret

    # [[noreturn]] void do_bad_exception(struct regs *regs, long cause)
    # Print message about bad U-mode exception, then stop
do_bad_exception:
    mv s0, t0

    # Equivalent of printf("Exception 0x%x", cause);
    la a0, msg_exception
    call kputs

    mv a0, s0
    la t0, hex_chars
    add t0, t0, a0
    lbu a0, (t0)
    call kputc

    li a0, 0xD # '\r'
    call kputc
    li a0, 0xA # '\n'
    call kputc

    # Stop the emulator
    ebreak


fatal_noproc:
    # Print message about fatal exception, then stop
    la a0, msg_fatal_noproc
    call kputs
    ebreak
fatal_mexc:
    la a0, msg_fatal_mexc
    call kputs
    ebreak

    # The big exception handler
handler:

    # Save all registers
    addi sp, sp, -128
    sw x1, 4(sp)
    sw x2, 8(sp)
    sw x3, 12(sp)
    sw x4, 16(sp)
    sw x5, 20(sp)
    sw x6, 24(sp)
    sw x7, 28(sp)
    sw x8, 32(sp)
    sw x9, 36(sp)
    sw x10, 40(sp)
    sw x11, 44(sp)
    sw x12, 48(sp)
    sw x13, 52(sp)
    sw x14, 56(sp)
    sw x15, 60(sp)
    sw x16, 64(sp)
    sw x17, 68(sp)
    sw x18, 72(sp)
    sw x19, 76(sp)
    sw x20, 80(sp)
    sw x21, 84(sp)
    sw x22, 88(sp)
    sw x23, 92(sp)
    sw x24, 96(sp)
    sw x25, 100(sp)
    sw x26, 104(sp)
    sw x27, 108(sp)
    sw x28, 112(sp)
    sw x29, 116(sp)
    sw x30, 120(sp)
    sw x31, 124(sp)
    
    # save pc
    csrr t0, mepc
    sw t0, 0(sp)

    # pass the user sp to trap_main
    mv a0, sp

	# save user stack pointer
    # if (!ready_tail)
	la t0, ready_tail
	lw t0, 0(t0)
    beq t0, zero, fatal_noproc
	# ready_tail->sp = sp
	sw a0, 0(t0)

    # load kernel stack pointer
	csrrw sp, mscratch, zero

    # If mscratch was 0, this is exception from M-mode
    # Can't handle that, it's a fatal error
    beq sp, zero, fatal_mexc
    
    call trap_main
    # ... falls through after trap_main ...
enter_user:
    # save kernel sp
    csrrw zero, mscratch, sp
    
    # load user stack pointer
    # sp = ready_tail->sp
	la t0, ready_tail
	lw t0, 0(t0)
    beq t0, zero, fatal_noproc
	lw sp, 0(t0)

    # Set mstatus.MPP = ready_tail->mode
    li t1, 0x1800
    csrrc zero, mstatus, t1
    lbu t1, 18(t0)
    slli t1, t1, 11
    csrrs zero, mstatus, t1

    # Set mepc = user pc
    # Will actually jump with mret
    lw t0, 0(sp)
    csrw mepc, t0
    # Restore other registers from stack
    lw x1, 4(sp)
    # x2/sp handled separately
    lw x3, 12(sp)
    lw x4, 16(sp)
    lw x5, 20(sp)
    lw x6, 24(sp)
    lw x7, 28(sp)
    lw x8, 32(sp)
    lw x9, 36(sp)
    lw x10, 40(sp)
    lw x11, 44(sp)
    lw x12, 48(sp)
    lw x13, 52(sp)
    lw x14, 56(sp)
    lw x15, 60(sp)
    lw x16, 64(sp)
    lw x17, 68(sp)
    lw x18, 72(sp)
    lw x19, 76(sp)
    lw x20, 80(sp)
    lw x21, 84(sp)
    lw x22, 88(sp)
    lw x23, 92(sp)
    lw x24, 96(sp)
    lw x25, 100(sp)
    lw x26, 104(sp)
    lw x27, 108(sp)
    lw x28, 112(sp)
    lw x29, 116(sp)
    lw x30, 120(sp)
    lw x31, 124(sp)
    addi sp, sp, 128

    mret    # Time to go to user mode!



