
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
    




