.section .data
.global user_stack

user_stack:
    .space 256

msg_hello:
    .asciz "Hello world!\r\n"

msg_yeet:
    .asciz "Yeet\r\n"

msg_tmr:
    .asciz "tmr_count: %x\r\n"

timer_val:
    .word 0

.section .text
.global user_entry
.extern puts
.extern printf
.extern opendev
.extern write
.extern read
.extern exit

user_entry:
    
    la a0, msg_hello
    call puts
    
    # write(0, "Yeet\r\n", 6)
    li a0, 0
    la a1, msg_yeet
    li a2, 6
    call write

    # int tmr = opendev(MKDEV(1, 0))
    li a0, 16
    call opendev

    # read(tmr, &timer_val, 4)
    la a1, timer_val
    li a2, 4
    call read

    # printf("tmr_count: %x\r\n")
    la a0, msg_tmr
    la a1, timer_val
    lw a1, 0(a1)
    call printf

    call exit


