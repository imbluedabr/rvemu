.section .text
.global _start
.extern user_entry
.org 0x0000
    
_start:
    la sp, __stack_top
    
    #print version
    la a0, msg_version
    call kputs

    # Reserve 256 bytes for OS stack
    # User stack starts 256 bytes lower
    addi t2, sp, -256

    la t0, handler
    csrw mtvec, t0

    # Prepare struct reg
    addi sp, sp, -128

    mv a0, sp # struct regs *

    # Set user pc to user_entry
    la t0, user_entry
    sw t0, 0(a0)

    # Set user sp
    sw t2, 8(a0)

    j enter_user

    # void trap_main(struct regs *regs)
trap_main:
    # Save regs based on calling convention
    addi sp, sp, -16
    sw s0, (sp)
    sw ra, 4(sp)

    mv s0, a0
    csrr a1, mcause
    li t1, 8 # "Environment call from User mode"
    bne a1, t1, do_bad_exception # Not ecall, that's bad

    # Call do_syscall with args from ecall

    lw a0, 40(s0)
    lw a1, 44(s0)
    lw a2, 48(s0)
    lw a3, 52(s0)
    lw a4, 56(s0)
    lw a5, 60(s0)
    lw a6, 64(s0)
    lw a7, 68(s0)
    call do_syscall

    sw a0, 40(s0)   # Set user a0 return value

    # Bump user pc by 4
    # Skip over ecall instruction
    lw t0, 0(s0)
    addi t0, t0, 4
    sw t0, 0(s0)

    # Restore regs based on calling convention
    lw s0, (sp)
    lw ra, 4(sp)
    addi sp, sp, 16
    ret

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

    # [[noreturn]] void do_bad_exception(struct regs *regs, long cause)
    # Print message about bad U-mode exception, then stop
do_bad_exception:
    mv s0, a1

    # Equivalent of printf("Exception 0x%x", cause);
    la a0, msg_exception
    call kputs

    mv a0, s0
    la t0, hex_chars
    add t0, t0, a0
    lbu a0, (t0)
    call kputchar

    li a0, 0xD # '\r'
    call kputchar
    li a0, 0xA # '\n'
    call kputchar

    # Stop the emulator
    ebreak

fatal:
    # Print message about fatal exception, then stop
    la a0, msg_fatal
    call kputs
    ebreak



    # void kputs(const char *);
    # Print string by accessing MMIO directly
kputs:
    lui t1, %hi(0x8000)
1:
    lb t0, 0(a0)
    beq t0, zero, 2f
    sb t0, 0(t1)
    addi a0, a0, 1
    j 1b
2:
    ret

    # void kputchar(char);
    # Print byte by accessing MMIO directly
kputchar:
    lui t1, %hi(0x8000)
    sb a0, (t1)
    ret

    # The big exception handler
handler:
    csrrw sp, mscratch, sp

    # If mscratch was 0, this is exception from M-mode
    # Can't handle that, it's a fatal error
    beq sp, zero, fatal

    # Save all registers
    addi sp, sp, -128
    sw x1, 4(sp)
    # x2/sp handled separately
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

    # Save user sp, also set mscratch to 0 in M-mode
    csrrw t0, mscratch, zero
    sw t0, 8(sp)

    # Save user pc
    csrr t0, mepc
    sw t0, 0(sp)

    mv a0, sp
    call trap_main
    # ... falls through after trap_main ...
enter_user:
    # Set mstatus.MPP = User
    lui t0, %hi(0x1800)
    addi t0, t0, %lo(0x1800)
    csrrc zero, mstatus, t0

    # Set mepc = user pc
    # Will actually jump with mret
    lw t0, 0(sp)
    csrw mepc, t0

    # Set mscratch = user sp temporarily
    # Will swap right before mret
    lw t0, 8(sp)
    csrw mscratch, t0

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

    # Actually restore sp
    csrrw sp, mscratch, sp
    mret    # Time to go to user mode!


.section .data

msg_exception:
    # "Exception 0x"
    .asciz "Exception 0x"

msg_fatal:
    # "Fatal exception\n"
    .asciz "Fatal exception\r\n"

msg_version:
    .asciz "EasyRTOS v0.1.0\r\n"

hex_chars:
    # "0123456789abcdef"
    .asciz "0123456789ABCDEF"

