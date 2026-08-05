/* driver for the timers */

.section .data
.global timer_driver

# struct driver timer_driver
timer_driver:
	.word timer_create
	.word 0
	.word 0
	.word timer_driver_name

timer_driver_name:
    .asciz "timer"

# struct dev_ops timer_ops
timer_ops:
	.word timer_read
	.word timer_write
	.word 0

/*
	struct timer_device {
		struct device dev;
		int ticks;
	}
*/
	

.section .text

# void timer_driver(struct device* dev, void* base)
timer_driver:
	# struct timer_device* timer = dev
	# timer->dev.ops = &timer_ops
	la t0, timer_ops
	sw t0, 0(a0)
	
	# timer->dev.base = base
	sw a1, 8(a0)
	
	# timer->ticks = 0
	li t0, 0
	sw t0, 16(a0)

	ret

# int timer_read(struct device* timer, void* buff, int count)
timer_read:
	li t0, 4
	bne t0, a2, 1f
	lw t1, 16(a0)
	sw t1, 0(a1)
	mv a0, t0
	ret
1:
	li a0, 0
	ret



