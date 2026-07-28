.section .text
.global user_entry

user_entry:
    la a0, msg_hello
    call puts
    call exit

    # void puts(const char *);
    # Print string using system call
puts:
    addi sp, sp, -16
    sw s0, (sp)
    sw ra, 4(sp)

    mv s0, a0
1:
    lb a0, 0(s0)
    beq a0, zero, 2f
    call putchar
    addi s0, s0, 1
    j 1b
2:

    lw s0, (sp)
    lw ra, 4(sp)
    addi sp, sp, 16
    ebreak
    ret

    # void putchar(const char *);
    # Print byte using system call
putchar:
    li a7, 1
    ecall
    ret

    # [[noreturn]] void exit();
exit:
    li a7, 2
    ecall
    # Not supposed to return, just to be safe
    unimp

.section .data
msg_hello:
    .byte 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x21, 0x0a, 0x0
