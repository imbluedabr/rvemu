.section .data
.global task_block
.global ready_tail
/*
	struct task {
		uint8_t* sp
		struct task* ready_next
		struct task* wait_next
		uint32_t save_pc
		uint8_t tid
		uint8_t state 0: zombie, 1: running, 2: waiting
		uint8_t mode 0: user, 3: kernel
	}
*/

# struct task task_table[4]
task_block:
	.space 80

# struct task* ready_head
ready_head:
	.word 0
ready_tail:
	.word 0

# const char* state_strings[3]
state_strings:
	.word str_1
	.word str_2
	.word str_3

str_0:
	.asciz "sp=0x%x, pc=0x%x, tid=0x%x, state=%s\r\n"
str_1:
	.asciz "ZOMBIE"
str_2:
	.asciz "RUNNING"
str_3:
	.asciz "WAITING"
str_4:
	.asciz "task_block:\r\n"
str_5:
	.asciz "halting kernel!\r\n"
.section .text
.global task_view
.global task_create
.global task_cont
.global task_kill
.global schedule_new_task
.extern kprintf
.extern kputs

	# void task_view()
	# show all tasks
task_view:
	csrrci t4, mstatus, 0x8
	addi sp, sp, -16
	sw ra, 0(sp)
	sw s0, 4(sp)
	mv s0, zero
	
	# kputs("task_block:\r\n")
	la a0, str_4
	call kputs
1:
	li t0, 80
	beq s0, t0, 3f
	
	# struct task* t = task_block + i
	la t0, task_block
	add t0, t0, s0
	# if (t->sp)
	lw a1, 0(t0)
	beq a1, zero, 2f
	# kprintf("sp=0x%x, pc=0x%x, tid=0x%x, state=%s\r\n", t->sp, t->tid, state_strings[t->state])
	la a0, str_0
	lw a2, 0(a1)
	lbu a3, 16(t0)
	lbu t1, 17(t0)
	slli t1, t1, 2
	la t0, state_strings
	add t1, t0, t1
	lw a4, 0(t1)
	call kprintf
2:
	# i++
	addi s0, s0, 20
	j 1b
3:
	lw ra, 0(sp)
	addi sp, sp, 16
	csrrw zero, mstatus, t4
	ret

	# struct task* task_create(uint8_t* sp, void* entry)
task_create:
	csrrci t4, mstatus, 0x8 #todo: should really use a mutex here
	la t0, task_block # struct task* t = task_block
	mv t1, zero
	li t2, 4
1:
	beq t1, t2, 3f
	lw t3, 0(t0)
	beq t3, zero, 2f
	addi t0, t0, 20
	addi t1, t1, 1
	j 1b
2:
	sb t1, 16(t0) 	# t->tid = i
	li t1, 2
	sb t1, 17(t0)	# t->state = WAITING
	sb zero, 18(t0)	# t->mode = USER

	# push a context frame to the user stack
	addi a0, a0, -128
	sw a1, 0(a0)

	sw a0, 0(t0)
	sw zero, 4(t0)
	sw zero, 8(t0)
	
	csrrw zero, mstatus, t4
	mv a0, t0
	ret
3:
	csrrw zero, mstatus, t4
	mv a0, zero
	ret



	
	# void task_cont(struct task* t)
task_cont:
	csrrci t4, mstatus, 0x8
	
	# t->state = 1
	li t1, 0x01
	sb t1, 17(a0)
	
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
	csrrw zero, mstatus, t4
	ret

	# void task_kill(struct task* t)
task_kill:
	csrrci t4, mstatus, 0x8
	# if (t->state != ZOMBIE)
	lbu t0, 17(a0)
	beq t0, zero, 1f
	# t->state == ZOMBIE
	sb zero, 17(a0)
1:
	csrrw zero, mstatus, t4
	ret

# void schedule_new_task()
schedule_new_task:
	# struct task* t = ready_tail
1:
	la t0, ready_tail
	lw t1, 0(t0)
	# if (t == NULL)
	beq t1, zero, schedule_stop
	# ready_tail = t->ready_next
	lw t2, 4(t1)
	sw t2, 0(t0)
	# t->ready_next = NULL
	sw zero, 4(t1)

	# if (t->state == ZOMBIE)
	lbu t2, 17(t1)
	beq t2, zero, 1b
	
	# if (ready_tail)
	lw t2, 0(t0)
	bne t2, zero, 2f
	# ready_tail = t
	sw t1, 0(t0)
	# ready_head = t
	la t0, ready_head
	sw t1, 0(t0)
	j 3f
2:
	# ready_head->ready_next = t
	la t0, ready_head
	lw t2, 0(t0)
	sw t1, 4(t2)
	# ready_head = t
	sw t1, 0(t0)
3:
	ret


schedule_stop:
	la a0, str_5
	call kputs
	ebreak

