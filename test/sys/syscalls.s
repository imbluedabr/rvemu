/*
    handles user syscalls and dispatches the right syscall handler
*/
.section .data

# jump table
syscall_table:
    .word sys_read
    .word sys_write
    .word sys_opendev
    .word sys_close
    .word sys_exit
    .word sys_putchar

.section .text
.extern kputs
.extern kputc
.extern ready_tail
.extern task_kill
.extern file_table
.extern file_alloc
.extern file_free
.extern device_lookup

.global syscall_handler

    # entry point of the syscall handler
syscall_handler:
    addi sp, sp, -16
    sw ra, 0(sp)

    # Dispatch based on syscall number
    
    # if (svcno >= 6) return -1
    li t0, 6
    bge a7, t0, 1f
    # syscall_table[svcno]()
    la t0, syscall_table
    slli a7, a7, 2
    add t0, t0, a7
    lw t0, 0(t0)
    jalr ra, t0, 0
    j 2f
1:
    la a0, -1  
2:
    lw ra, 0(sp)
    addi sp, sp, 16
    ecall

    # int sys_putchar(char c)
sys_putchar:
    # Save regs based on calling convention
    addi sp, sp, -16
    sw s0, (sp)
    sw ra, 4(sp)

    call kputc
    li a0, 0

    # Restore regs based on calling convention
    lw s0, (sp)
    lw ra, 4(sp)
    addi sp, sp, 16
    ret

    # void sys_exit()
sys_exit:
    addi sp, sp, -16
    sw ra, 0(sp)

    # kill the current process
    # task_kill(ready_tail)
    la a0, ready_tail
    lw a0, 0(a0)
    call task_kill

    lw ra, 0(sp)
    addi sp, sp, 16
    ret

    # int sys_opendev(int devno)
sys_opendev:
    addi sp, sp, -16
    sw ra, 0(sp)
    # struct device* dev = device_lookup(devno)
    call device_lookup
    bne a0, zero, 1f
    li a0, -1 # return -1
    j 2f
1:
    # return file_alloc(dev)
    call file_alloc
2:
    lw ra, 0(sp)
    addi sp, sp, 16
    ret

    # int sys_write(int file, void* buff, int count)
sys_write:
    addi sp, sp, -16
    sw ra, 0(sp)
    # struct device* dev = file_table[file].node
    la t0, file_table
    slli t1, a0, 2
    add t0, t0, t1
    lw t0, 0(t0)
    bne t0, zero, 1f # if (!dev) return -1
    li a0, -1
    j 2f
1:
    # return dev->ops->write(dev, buff, count)
    mv a0, t0
    lw t1, 0(t0)
    lw t1, 4(t1)
    jalr ra, t1, 0
2:
    lw ra, 0(sp)
    addi sp, sp, 16
    ret

    # int sys_read(int file, void* buff, int count)
sys_read:
    addi sp, sp, -16
    sw ra, 0(sp)
    # struct device* dev = file_table[file].node
    la t0, file_table
    slli t1, a0, 2
    add t0, t0, t1
    lw t0, 0(t0)
    bne t0, zero, 1f # if (!dev) return -1
    li a0, -1
    j 2f
1:
    # return dev->ops->read(dev, buff, count)
    mv a0, t0
    lw t1, 0(t0)
    lw t1, 0(t1)
    jalr ra, t1, 0
2:
    lw ra, 0(sp)
    addi sp, sp, 16
    ret

    # int sys_close(int file)
sys_close:
    addi sp, sp, -16
    sw ra, 0(sp)
    
    call file_free

    lw ra, 0(sp)
    addi sp, sp, 16
    ret


