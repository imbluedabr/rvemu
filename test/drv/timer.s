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
		void (*handler)()
	}

	struct mmio_timer {
		uint32_t count;
		uint32_t tmcr;
		uint32_t ctrl;
	}
*/


.section .text
.global timer_set_handler

	# void timer_set_handler(struct timer_device* tmr, void (*handler)())
timer_set_handler:
	# tmr->handler = handler
	sw a1, 20(a0)
	ret

	# void timer_create(struct device* dev, void* base, int irq)
timer_create:
	addi sp, sp, -16
	sw ra, 0(sp)
	# struct timer_device* timer = dev
	# timer->dev.ops = &timer_ops
	la t0, timer_ops
	sw t0, 0(a0)
	
	# timer->dev.base = base
	sw a1, 8(a0)
	
	# timer->ticks = 0
	li t0, 0
	sw t0, 16(a0)
	
	# struct mmio_timer* mm = base;
	# mm->tmcr = 1000
	li t0, 2000
	sw t0, 4(a1)
	# mm->count = 0
	li t0, 0
	sw t0, 0(a1)
	# mm->ctrl = 0x03
	li t0, 0x03
	sw t0, 8(a1)

	# irq_register(irq, timer_handler, dev)
	mv t0, a0
	mv a0, a2
	mv a2, t0
	la a1, timer_handler
	call irq_register
	
	lw ra, 0(sp)
	addi sp, sp, 16
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

	# int timer_write(struct device* timer, void* buff, int count)
timer_write:
	li t0, 4
	bne t0, a2, 1f
	lw t1, 0(a1)
	sw t1, 16(a0)
	mv a0, t0
	ret
1:
	li a0, 0
	ret


	# void timer_handler(struct timer_device* tmr)
timer_handler:
	addi sp, sp, -16
	sw ra, 0(sp)

	lw t0, 16(a0)
	addi t0, t0, 1
	sw t0, 16(a0)
	
	# if (tmr->handler) tmr->handler()
	lw t0, 20(a0)
	beq t0, zero, 1f
	jalr ra, t0, 0
1:

	lw ra, 0(sp)
	addi sp, sp, 16
	ret
