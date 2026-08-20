.section .data
msg_version:
    .asciz "EasyRTOS v0.4.1\r\n"

.section .text.start
.global _start
.extern kputs
.extern kprintf
.extern boot_console
.extern irq_init
.extern enter_user
.extern register_driver
.extern create_device
.extern uart_driver
.extern timer_driver
.extern timer_set_handler
.extern task_view
.extern task_create
.extern schedule_new_task
.extern user_stack
.extern user_entry
.extern bg_task_stack
.extern bg_task_entry

# props to easyriscv, modified by me

_start:    
    call irq_init
    
    # register_driver(0, uart_driver)
    li a0, 0
    la a1, uart_driver
    call register_driver

    # register_driver(1, timer_driver)
    li a0, 1
    la a1, timer_driver
    call register_driver
    
    # struct timer_device* timer0 = device_create(1, 0x8010, 1)
    li a0, 1
    li a1, 0x8010
    li a2, 1
    call device_create

    # timer_set_handler(timer0, schedule_new_task)
    la a1, schedule_new_task
    call timer_set_handler   

    # boot_console = device_create(0, 0x8000, 0)
    li a0, 0
    li a1, 0x8000
    li a2, 0
    call device_create
    la t0, boot_console
    sw a0, 0(t0)

    # print version
    la a0, msg_version
    call kputs

    # list_drivers()
    call list_drivers

    # struct task* t0 = task_create(user_stack + 256, user_entry)
    la a0, user_stack
    addi a0, a0, 256
    la a1, user_entry
    call task_create
    
    # task_cont(t0)
    call task_cont

    # struct task* t1 = task_create(bg_task_stack + 256, bg_task_entry)
    la a0, bg_task_stack
    addi a0, a0, 256
    la a1, bg_task_entry
    call task_create

    # task_cont(t1)
    call task_cont

    # task_view()
    call task_view

    li t0, 0x80
    csrrs t0, mstatus, t0
    j enter_user
