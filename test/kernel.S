.section .text
.global _start
.extern user_entry
.org 0x0000

# props to easyriscv, modified by me

_start:
    la sp, __stack_top
    
    #print version
    la a0, msg_version
    call kputs

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
/*
    struct driver {
        int (*init)(struct device* dev, void* base)
        int (*read)(struct device* dev, void* buff, uint32_t count)
        int (*write)(struct device* dev, void* buff, uint32_t count)
    }
    
    struct device {
        struct driver* driver
        void* base
        uint32_t state[14] //each device is allocated 64 bytes
    }
*/


os_tick_count:
    .word 0

msg_version:
    .asciz "EasyRTOS v0.1.0\r\n"


