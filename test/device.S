
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

    # struct device device_table[4]
device_table:
    .space 256
device_table_end:

    # struct driver driver_table[4]
driver_table:
    .space 64

.section .text

    # struct device* device_alloc()
device_alloc:
    la t0, device_table #struct device* p = device_table;
1:
    #if (p->driver == NULL)
    lw t1, 0(t0)
    li t2, 0
    beq t1, t2, 2f
    addi t0, t0, 64
    la t1, device_table_end
    beq t0, t1, 3f
    j 1b
2:
    # return p
    mov a0, t0
    ret
3:
    # return NULL
    li a0, 0
    ret

    # struct device* device_create(int driver, void* base)
device_register:
    addi sp, sp, -16
    sw ra, 0(sp)
    sw s0, 4(sp)
    sw s1, 8(sp)

    # struct driver* drv = &driver_table[driver];
    la s0, driver_table
    slli a0, a0, 4
    add s0, s0, a0
    
    #struct device* dev = device_alloc()
    call device_alloc
    mov s1, a0

    #drv->init(dev, base)
    lw t0, 0(s0)
    jalr ra, t0, 0
    
    mov a0, s0
    lw s1, 8(sp)
    lw s0, 4(sp)
    lw ra, 0(sp)
    addi sp, sp, 16
    ret

    # struct device* device_lookup(int devno)
device_lookup:
    
