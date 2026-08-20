/*
    define the file structure and functions for managing file objects.
*/

.section .data
.global file_table
/*
    struct file {
        void* node # can be any device/node that implements the dev_ops vtable 
    }
*/

    # struct file file_table[32]
file_table:
    .space 128

.section .text
.global file_alloc
.global file_free

    # int file_alloc(void* node)
file_alloc:
    la t0, file_table
    li t1, 0
    li t2, 32
1:
    beq t1, t2, 3f
    lw t3, 0(t0)
    beq t3, zero, 2f
    addi t0, t0, 4
    addi t1, t1, 1
    j 1b
2:
    sw a0, 0(t0) # file->node = node
    mv a0, t1 # return i
    ret
3:
    li a0, -1   # return -ENOMEM
    ret

    # void file_free(int file)
file_free:
    sw zero, 0(a0)
    ret

