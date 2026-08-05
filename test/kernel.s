.section .text.start
.global _start
.extern kputs
.extern user_entry
.extern irq_init
.extern enter_user
.extern uart_driver

# props to easyriscv, modified by me

_start:
    la sp, __stack_top
    
    call irq_init

    #print version
    la a0, msg_version
    call kputs

    # register_driver(0, uart_driver)
    li a0, 0
    la a1, uart_driver
    call register_driver

    # list_drivers()
    call list_drivers

    # Reserve 256 bytes for OS stack
    # User stack starts 256 bytes lower
    addi t2, sp, -256    

    # Prepare struct reg
    addi sp, sp, -128

    mv a0, sp # struct regs *

    # Set user pc to user_entry
    la t0, user_entry
    sw t0, 0(a0)

    # Set user sp
    sw t2, 8(a0)

    j enter_user


.section .data

msg_version:
    .asciz "EasyRTOS v0.2.0\r\n"


