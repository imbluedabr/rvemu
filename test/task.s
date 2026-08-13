.section .data
.global task_block
.global ready_tail
/*
	struct task {
		uint8_t* sp
		struct task* ready_next
		struct task* wait_next
		uint8_t tid
		uint8_t state 0: zombie, 1: running, 2: waiting
	}
*/

# struct task task_table[4]
task_block:
	.space 64

# struct task* ready_head
ready_head:
	.word 0
ready_tail:
	.word 0


.section .text
.global task_create
.global task_cont
.global schedule_new_task

# struct task* task_create(uint8_t* sp, void* entry)
task_create:
	csrrci zero, mstatus, 0x8
	la t0, task_block
	mv t1, zero
	li t2, 4
1:
	beq t1, t2, 3f
	addi t0, t0, 16
	lw t3, 0(t0)
	bne t3, zero, 2f
	addi t1, t1, 2
	j 1b
2:
	sbu t1, 12(t0)
	li t1, 2
	sbu t1, 13(t0)

	sw a1, 0(a0)
	sw a0, 0(t0)
	sw zero, 4(t0)
	sw zero, 8(t0)
	
	csrrsi zero, mstatus, 0x8
	mv a0, t0
	ret
3:
	csrrsi zero, mstatus, 0x8
	mv a0, zero
	ret


# void task_cont(struct task* t)
task_cont:
	csrrci zero, mstatus, 0x8
	
	# t->state = 1
	li t1, 0x01
	sbu t1, 12(a0)
	
	# if (ready_head)
	la t1, ready_head
	lw t2, 0(t1)
	beq t2, zero, 1f

	# ready_head->ready_next = t
	sw a0, 4(t2)
	# ready_head = t
	sw a0, 0(t1)
	j 2f
1:
	# ready_head = t
	sw a0, 0(t1)
	# ready_tail = t
	la t1, ready_tail
	sw a0, 0(t1)
2:
	csrrsi zero, mstatus, 0x8
	ret

# void schedule_new_task()
schedule_new_task:
	# struct task* t = read_tail
	la t0, ready_tail
	lw t1, 0(t0)
	# ready_tail = t->ready_next
	lw t2, 4(t1)
	sw t2, 0(t0)
	# ready_head->ready_next = t
	la t0, ready_head
	lw t2, 0(t0)
	sw t1, 4(t2)
	# ready_head = t
	sw t1, 0(t0)
	ret


