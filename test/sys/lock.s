/*
    provide synchronization primitives for the drivers and the kernel.


    typedef struct {
        uint8_t lock
    } spinlock_t
*/

.section .text
.global spinlock_lock
.global spinlock_unlock
.extern ready_head


    # void spinlock_lock(spinlock_t* lock)
spinlock_lock:
    
    # while(lock->lock)
1:
    csrrci t0, mstatus, 0x8
    lbu t1, 0(a0)
    bne t1, zero, 2f
    li t2, 1
    sb t2, 0(a0)
2:
    csrrw zero, mstatus, t0
    bne t1, zero, 1b
    ret

    # void spinlock_unlock(spinlock_t* lock)
spinlock_unlock:
    sb zero, 0(a0) # lock->lock = 0
    ret

