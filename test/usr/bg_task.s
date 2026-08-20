/*
    background task/deamon that writes to the console every 2 seconds
*/
.section .data
.global bg_task_stack

bg_task_stack:
    .space 256

msg_init:
    .asciz "bg_task: started\r\n"

msg_update:
    .asciz "Hello!\r\n"

last_update:
    .word 0
curr_update:
    .word 0

.section .text
.global bg_task_entry
.extern puts
.extern opendev
.extern read

bg_task_entry:
    
    # puts("bg_task: started\r\n")
    la a0, msg_init
    call puts

    # int tmr = opendev(MKDEV(1, 0))
    li a0, 16
    call opendev
    mv s0, a0

    # read(tmr, &last_update, 4)
    la a1, last_update
    li a2, 4
    call read

bg_task_loop:
    
    # read(tmr, &curr_update, 4)
    
    mv a0, s0
    la a1, curr_update
    li a2, 4
    call read
    
    # if (curr_update - last_update > 200)
    la t0, curr_update
    lw t1, (t0)
    la t2, last_update
    lw t3, (t2)
    sub t1, t1, t3
    li t3, 200
    ble t1, t3, 1f
    # last_update = curr_update
    lw t1, (t0)
    sw t1, (t2)
    # puts("Hello!\r\n")
    la a0, msg_update
    call puts
1:
    j bg_task_loop
