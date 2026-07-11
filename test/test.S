.section .text
.global _start

.org 0x0000

_start:
    nop
    
    #print(hello_str)
    li a0, %lo(hello_str)
    call print

    ebreak

# void print(char* str)
print:
    # t0 = UART0_DATA
    lui t0, %hi(0x8000)
print_loop:
    lb t1, 0(a0) #t2 = *str
    beq t1, zero, print_exit
    sb t1, 0(t0)  #*t0 = t2
    addi a0, a0, 0x1 #str++
    j print_loop
print_exit:
    ret


.section .data

hello_str:
    .asciz "Hello world!\r\n\0"

