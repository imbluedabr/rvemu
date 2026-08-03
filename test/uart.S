
.section .data

# struct driver uart_driver
uart_driver:
    .word uart_init
    .word uart_read
    .word uart_write

/*
    struct mmio_uart {
        uint8_t data
        struct {
            uint8_t rx_avail : 1
            uint8_t tx_full : 1
        } csr
    }

    struct uart_device {
        struct device _Device
    }
*/

.section .text
.extern irq_register
.extern irq_device_table

    # void uart_init(struct device* uart, void* base)
uart_init:
    #uart->driver = &uart_driver
    li t0, uart_driver
    sw t0, 0(a0)

    #uart->base = base
    sw a1, 4(a0)
    
    ret

    # int uart_write(struct device* uart, void* buff, uint32_t count)
uart_write:
    li t0, 0 #int i = 0
    lw t1, 4(a0) #char* data = &uart->base->data
1:
    beq t0, a2, 2f
    #*data = *(buff + i)
    add t2, a1, t0
    lb t3, 0(t2)
    sb t3, 0(t1)
    #i++
    addi t0, t0, 1
    j 1b
2:
    ret

uart_read:
    li t0, 0 #int i = 0
    lw t1, 4(a0) #struct mmio_uart* base = &uart->base
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


