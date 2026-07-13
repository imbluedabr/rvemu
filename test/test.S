.section .text
.global _start

.org 0x0000

_start:
    #set the trap vector
    la t0, trap_handler
    csrw mtvec, t0
    
    #print(hello_str)
    la a0, hello_str
    call print

    ecall


trap_handler:
    la a0, trap_str
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

trap_str:
    .asciz "Hello from the ecall handler!\r\n\0"

