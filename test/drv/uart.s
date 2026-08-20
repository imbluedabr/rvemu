
.section .data
.global uart_driver

uart_driver_name:
    .asciz "uart"

# struct driver uart_driver
uart_driver:
    .word uart_create # int (*create)(struct device* dev, void* base) 
    .word 0 # struct device* instance_list
    .word 0 # int instance_count
    .word uart_driver_name

# struct dev_ops uart_ops
uart_ops:
    .word uart_read
    .word uart_write
    .word 0
/*
    struct mmio_uart {
        uint8_t data
        struct {
            uint8_t rx_avail : 1
            uint8_t tx_full : 1
        } csr
    }

    struct uart_device {
        struct device dev
        spinlock_t lock
    }
*/

.section .text
.extern irq_register
.extern irq_device_table
.extern spinlock_lock
.extern spinlock_unlock

    # void uart_create(struct device* uart, void* base)
uart_create:
    #uart->ops = &uart_ops
    la t0, uart_ops
    sw t0, 0(a0)

    #uart->base = base
    sw a1, 8(a0)
    
    # uart->lock = 0
    sb zero, 16(a0)

    ret

    # int uart_write(struct device* uart, void* buff, uint32_t count)
uart_write:
    addi sp, sp, -16
    sw ra, 0(sp)
    sw s0, 4(sp)
    sw s1, 8(sp)
    sw s2, 12(sp)
    mv s0, a0
    mv s1, a1
    mv s2, a2

    # spinlock_lock(&uart->lock)
    addi a0, s0, 16
    call spinlock_lock

    lw t1, 8(s0) #char* data = &uart->dev.data
    li t0, 0 #int i = 0
1:
    beq t0, s2, 2f
    #*data = *(buff + i)
    add t2, s1, t0
    lb t3, 0(t2)
    sb t3, 0(t1)
    #i++
    addi t0, t0, 1
    j 1b
2:
    # spinlock_unlock(&uart->lock)
    addi a0, s0, 16
    call spinlock_unlock

    lw s2, 12(sp)
    lw s1, 8(sp)
    lw s0, 4(sp)
    lw ra, 0(sp)
    addi sp, sp, 16
    ret

uart_read:
    li t0, 0 #int i = 0
    lw t1, 8(a0) #struct mmio_uart* base = &uart->base
1:
    beq t0, a2, 2f
    # if (!base->csr->rx_avail) continue
    lb t3, 1(t1)
    li t4, 0x01
    and t3, t3, t4
    bne t3, t4, 1b
    #*(buff + i) = base->data
    add t2, a1, t0
    lb t3, 0(t1)
    sb t3, 0(t2)
    #i++
    addi t0, t0, 1
    j 1b
2:
    ret


