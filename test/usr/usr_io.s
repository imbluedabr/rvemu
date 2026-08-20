/*
    contains functions for io and syscalls
*/
.section .data

hex_chars:
    .asciz "0123456789ABCDEF"

.section .text
.global putc
.global puts
.global puth
.global printf
.global read
.global write
.global opendev
.global close
.global exit

    # int strlen(const char* str)
strlen:
    mv t0, a0
1:
    lbu t1, 0(t0)
    beq t1, zero, 2f
    addi t0, t0, 1
    j 1b
2:
    sub a0, t0, a0
    ret

    # void putc(const char *);
    # write a byte to the boot console
putc:
    li a7, 5
    ecall
    ret

    # void puts(const char * str);
    # Print string using system call
puts:
    addi sp, sp, -16
    sw ra, 0(sp)

    # write(0, str, strlen(str))
    mv a1, a0
    call strlen
    mv a2, a0
    mv a0, zero
    call write

    lw ra, 0(sp)
    addi sp, sp, 16
    ret


    # void puth(int num)
    # print number in hexadecimal
puth:
    addi sp, sp, -16
    sw ra, 0(sp)
    sw s0, 4(sp)
    sw s1, 8(sp)

    li s0, 28
    mv s1, a0
1:
    srl t0, s1, s0
    andi t0, t0, 0xF
    la t1, hex_chars
    add t0, t0, t1
    lbu a0, 0(t0)
    call putc

    beq s0, zero, 2f
    addi s0, s0, -4
    j 1b
2:
    lw s1, 8(sp)
    lw s0, 4(sp)
    lw ra, 0(sp)
    addi sp, sp, 16
    ret


    # void printf(const char* fmt, a1, a2, a3, a4)
printf:
    addi sp, sp, -32
    sw ra, 16(sp)
    sw s0, 20(sp)
    sw s1, 24(sp)
    mv s0, a0
    mv s1, sp

    #push arguments
    sw a1, 0(sp)
    sw a2, 4(sp)
    sw a3, 8(sp)
    sw a4, 12(sp)
    
    # while (*fmt)
1:
    lbu t0, 0(s0)
    beq t0, zero, 7f
    
    li t1, '%'
    bne t0, t1, 5f

    addi s0, s0, 1
    lbu t0, 0(s0)
    li t1, 'c'
    beq t0, t1, 2f
    li t1, 's'
    beq t0, t1, 3f
    li t1, 'x'
    beq t0, t1, 4f
2:  # if (c == 'c')
    lw a0, 0(s1)
    addi s1, s1, 4
    call putc
    j 6f
3:  # if (c == 's')
    lw a0, 0(s1)
    addi s1, s1, 4
    call puts
    j 6f
4:  # if (c == 'x')
    lw a0, 0(s1)
    addi s1, s1, 4
    call puth
    j 6f
5:
    mv a0, t0
    call putc
6:
    addi s0, s0, 1
    j 1b
7:
    lw s1, 24(sp)
    lw s0, 20(sp)
    lw ra, 16(sp)
    addi sp, sp, 32
    ret



    # int read(int file)
read:
    li a7, 0
    ecall
    ret
    
    # int write(int file)
write:
    li a7, 1
    ecall
    ret
    
    # int opendev(int devno)
opendev:
    li a7, 2
    ecall
    ret

    # int close(int file)
close:
    li a7, 3
    ecall
    ret

    # [[noreturn]] void exit();
exit:
    li a7, 4
    ecall
    # Not supposed to return, just to be safe
    unimp




