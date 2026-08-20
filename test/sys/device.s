.section .data

/*
    struct driver {
        int (*create)(struct device* dev, void* base, int irq)
        struct device* instance_list
        int instance_count
        const char* name
    }
    
    struct dev_ops {
        int (*read)(struct device* dev, void* buff, uint32_t count)
        int (*write)(struct device* dev, void* buff, uint32_t count)
        int (*ioctl)(struct device* dev, int cmd, void* arg)
    }

    struct device {
        struct dev_ops* ops
        struct device* next
        void* base
        int minor
        uint8_t padding[48] //each device is allocated 64 bytes
    }
*/

    # struct device device_table[4]
device_table:
    .space 256
device_table_end:

    # struct driver* driver_table[4]
driver_table:
    .space 16

str_0:
    .asciz ":\r\n"
str_1:
    .asciz "0x%x, 0x%x\r\n"
.section .text
.global device_create
.global device_lookup
.global register_driver
.global list_drivers
.extern kputs
.extern kprintf

    # void list_drivers()
list_drivers:
    addi sp, sp, -16
    sw s0, 0(sp)
    sw s1, 4(sp)
    sw ra, 8(sp)
    la s0, driver_table
    mv s1, zero # int i = 0
1:
    li t0, 4
    beq s1, t0, 3f
    # struct driver* drv = driver_table[i]
    slli t0, s1, 2
    add t0, t0, s0
    lw t0, 0(t0)
    beq t0, zero, 2f # if (drv)
    # display_driver_info(drv)
    mv a0, t0
    call display_driver_info
2:
    addi s1, s1, 1
    j 1b
3:
    lw ra, 8(sp)
    lw s1, 4(sp)
    lw s0, 0(sp)
    addi sp, sp, 16
    ret

    # void display_driver_info(struct driver* drv)
display_driver_info:
    addi sp, sp, -16
    sw ra, 0(sp)
    sw s0, 4(sp)
    sw s1, 8(sp)
    mv s0, a0
    
    # kputs(drv->name)
    lw a0, 12(s0)
    call kputs
    la a0, str_0
    call kputs
    # struct device* dev = drv->instance_list
    lw s1, 4(s0)
    # while (dev)
1:
    beq s1, zero, 2f
    # kprintf("0x%x, 0x%x\r\n", dev, dev->base)
    la a0, str_1
    mv a1, s1
    lw a2, 8(s1)
    call kprintf
    # dev = dev->next
    lw s1, 4(s1)
    j 1b
2:
    lw s1, 8(sp)
    lw s0, 4(sp)
    lw ra, 0(sp)
    addi sp, sp, 16
    ret

    # void register_driver(int major, struct driver* drv)
register_driver:
    # driver_table[major] = drv
    la t0, driver_table
    slli a0, a0, 2
    add t0, t0, a0
    sw a1, 0(t0)
    ret

    # struct device* device_alloc()
device_alloc:
    la t0, device_table #struct device* p = device_table;
1:
    # if (p->driver == NULL)
    lw t1, 0(t0)
    beq t1, zero, 2f
    addi t0, t0, 64
    la t1, device_table_end
    beq t0, t1, 3f
    j 1b
2:
    # return p
    mv a0, t0
    ret
3:
    # return NULL
    mv a0, zero
    ret

    # struct device* device_create(int major, void* base, int irq)
device_create:
    addi sp, sp, -16
    sw ra, 0(sp)
    sw s0, 4(sp)
    sw s1, 8(sp)

    # struct driver* drv = driver_table[major]
    la s0, driver_table
    slli a0, a0, 2
    add s0, s0, a0
    lw s0, 0(s0)

    # struct device* dev = device_alloc()
    call device_alloc
    mv s1, a0

    # drv->create(dev, base, irq)
    lw t0, 0(s0)
    jalr ra, t0, 0
    
    # dev->next = drv->instance_list
    lw t0, 4(s0)
    sw t0, 4(s1)
    # drv->instance_list = dev
    sw s1, 4(s0)

    # dev->minor = drv->instance_count++
    lw t0, 8(s0)
    sw t0, 12(s1)
    addi t0, t0, 1
    sw t0, 8(s0)
    
    mv a0, s1 # return dev
    lw s1, 8(sp)
    lw s0, 4(sp)
    lw ra, 0(sp)
    addi sp, sp, 16
    ret

    # struct device* device_lookup(int devno)
device_lookup:
    # struct driver* drv = driver_table[MAJOR(devno)];
    la t0, driver_table
    andi t1, a0, 0xF0
    srli t1, t1, 2
    add t0, t0, t1
    lw t0, 0(t0)
    
    andi a0, a0, 0x0F # devno = MINOR(devno)

    # struct device* dev = drv->instance_list
    lw t1, 4(t0)
    # while(dev)
1:
    beq t1, zero, 2f
    # if (dev->minor == devno)
    lw t2, 12(t1)
    beq t2, a0, 2f
    # dev = dev->next
    lw t1, 4(t1)
    j 1b
2:
    # return dev
    mv a0, t1
    ret

