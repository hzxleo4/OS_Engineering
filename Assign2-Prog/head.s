#  head.s contains the 32-bit startup code.
#  Two L3 task multitasking. The code of tasks are in kernel area, 
#  just like the Linux. The kernel code is located at 0x10000. 
KRN_BASE 	= 0x10000
SCRN_SEL	= 0x18
LDT0_SEL	= 0x20
TSS0_SEL	= 0x28
#TSS1_SEL	= 0X30
#LDT1_SEL	= 0x38
PAGE_OFFSET = 0x80000000

.global startup_32
.global move_to_user_mode, systemcall_interrupt, timer_interrupt,set_cr3_test
.global idt,gdt,tss0,ldt0,pg_dir,pg0,pg1,pg2,pg3,stack0_krn_ptr,stack0_ptr,myjump,finish_paging,set_cr3
.global pg0_task1,pg1_task1,setup_paging,pg_dir_task1,before_task1_paging,stack1_ptr,stack1_krn_ptr,pg0_task0, pg_dir_task0, new_task_next_ip
.global pg_dir_task2,pg0_task2,stack2_krn_ptr,pg_dir_task3,pg0_task3,stack3_krn_ptr,pg_dir_task4,pg0_task4,stack4_krn_ptr

.text
startup_32:

	movl $0x10,%eax
	mov %ax,%ds
	lss init_stack-PAGE_OFFSET,%esp

	# reset idt and gdt register
	lidt lidt_opcode-PAGE_OFFSET
	lgdt lgdt_opcode-PAGE_OFFSET

	jmp $0x08,$reload-PAGE_OFFSET	# reload cs by jmp instruction
reload:
	movl $0x10,%eax		# reload all the segment registers
	mov %ax,%ds		# after changing gdt. 
	mov %ax,%es
	mov %ax,%fs
	lss init_stack-PAGE_OFFSET,%esp


	call setup_paging

	lea finish_paging, %ecx
    jmp *%ecx

finish_paging:	
	add $PAGE_OFFSET, %esp
	lidt lidt_opcode_paging  # set to kernel space's linear space
	lgdt lgdt_opcode_paging

myjump:	
	jmp kernel_main

setup_paging:
	
	movl $pg0+0x07-PAGE_OFFSET,pg_dir-PAGE_OFFSET		/* set present bit/user r/w */
	movl $pg1+0x03-PAGE_OFFSET,pg_dir+4*512-PAGE_OFFSET		/*  --------- " " --------- */

	# map (2GB+4MB) - (2GB+8MB) to 0 - 4 MB
	movl $pg1+4092-PAGE_OFFSET,%edi
	movl $0x3ff003,%eax		/*  4Mb - 4096 + 3 (r/w supervisor,p) */
	std
1:	stosl			/* fill pages backwards - more efficient :-) */
	subl $0x1000,%eax
	jge 1b

	# map 0 - 4 MB to 0 - 4 MB
	movl $pg0+4092-PAGE_OFFSET,%edi
	movl $0x3ff007,%eax		/*  4Mb - 4096 + 7 (r/w user,p) */
	std
2:	stosl			/* fill pages backwards - more efficient :-) */
	subl $0x1000,%eax
	jge 2b

set_cr3:	movl $pg_dir-PAGE_OFFSET,%eax		/* pg_dir is at 0x1xxxx */
	movl %eax,%cr3		/* cr3 - page directory start */
	movl %cr0,%eax
	orl $0x80000000,%eax
	movl %eax,%cr0		/* set paging (PG) bit */
	ret			/* this also flushes prefetch-queue */	

	# Move to user mode (task 0)
move_to_user_mode: 

	#unmask the timer interrupt.
	movl $0x21, %edx
	inb %dx, %al
	andb $0xfe, %al
	outb %al, %dx

	pushfl
	andl $0xffffbfff, (%esp)
	popfl
	movl $TSS0_SEL, %eax
	ltr %ax
	movl $LDT0_SEL, %eax
	lldt %ax 

	#update cr3 for task0
	movl $pg_dir_task0-PAGE_OFFSET,%eax	
	movl %eax,%cr3		/* cr3 - page directory start */

	sti
	pushl $0x17
	pushl $0x60000
	pushfl
	pushl $0x0f
	pushl $0x0
	iret


write_char:

	push %gs
	pushl %ebx
	pushl %eax
	mov $0x18, %ebx
	mov %bx, %gs
	movw scr_loc, %bx
	shl $1, %ebx 
	movb %al, %gs:(%ebx)
	shr $1, %ebx
	incl %ebx
	cmpl $2000, %ebx
	jb 1f
	movl $0, %ebx
1:	movl %ebx, scr_loc	
	popl %eax
	popl %ebx
	pop %gs
	ret


systemcall_interrupt:

	push %ds
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	movl $0x10,%edx
	mov %dx,%ds
	call write_char
	popl %eax
	popl %ebx
	popl %ecx
	popl %edx
	pop %ds
	
	iret

/* Timer interrupt handler */ 
timer_interrupt:
	push %ds
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	movl $0x10, %eax
	mov %ax, %ds
	movb $0x20, %al
	outb %al, $0x20

	call schedule_new
new_task_next_ip:
	movl $0x17, %eax
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	popl %eax
	popl %ebx
	popl %ecx
	popl %edx	
	pop %ds
	iret



/*******************/
#current:.long 0
scr_loc:.long 0


lidt_opcode:
	.word 256*8-1		# idt contains 256 entries
	.long idt-PAGE_OFFSET	# This will be rewrite by code. 

lgdt_opcode:
	.word 0x7ff	# so does gdt 
	.long gdt-PAGE_OFFSET		# This will be rewrite by code.

lidt_opcode_paging:
	.word 256*8-1		# idt contains 256 entries
	.long idt	# This will be rewrite by code. 

lgdt_opcode_paging:
	.word 0x7ff	# so does gdt 
	.long gdt	# This will be rewrite by code.


idt:	.fill 256,8,0		# idt is uninitialized

gdt:	.quad 0x0000000000000000	/* NULL descriptor */
	.quad 0x00cf9a000000ffff	/* 4GB 0x08, base = 0x0000 */
	.quad 0x00cf92000000ffff	/* 4GB 0x10 */
	.quad 0x00c0920b80000002	/* screen 0x18 - for display */
	.fill 254,4,0
end_gdt:



.fill 64,4,0
init_stack:                         
	.long init_stack-PAGE_OFFSET
	.word 0x10


/*************************************/

ldt0:	.quad 0x0000000000000000
	.quad 0x00cffa000000ffff	# 0x0f, base = 0x0000
	.quad 0x00cff2000000ffff	# 0x17
tss0:
	.long 0 			/* back link */
	.long stack0_krn_ptr, 0x10	/* esp0, ss0 */
	.long 0, 0			/* esp1, ss1 */
	.long 0, 0			/* esp2, ss2 */
	.long pg_dir_task0-PAGE_OFFSET				/* cr3 */
	.long 0x0			/* eip */
	.long 0x200			/* eflags */
	.long 0, 0, 0, 0		/* eax, ecx, edx, ebx */
	.long 0x60000, 0, 0, 0	/* esp, ebp, esi, edi */
	.long 0x17,0x0f,0x17,0x17,0x17,0x17 /* es, cs, ss, ds, fs, gs */
	.long LDT0_SEL			/* ldt */
	.long 0x8000000			/* trace bitmap */

#tss1:
#	.long 0 			/* back link */
#	.long stack1_krn_ptr, 0x10	/* esp0, ss0 */
#	.long 0, 0			/* esp1, ss1 */
#	.long 0, 0			/* esp2, ss2 */
#	.long pg_dir_task1-PAGE_OFFSET				/* cr3 */
#	.long 0			/* eip */
#	.long 0x200			/* eflags */
#	.long 0, 0, 0, 0		/* eax, ecx, edx, ebx */
#	.long 0x10000, 0, 0, 0	/* esp, ebp, esi, edi */
#	.long 0x17,0x0f,0x17,0x17,0x17,0x17 /* es, cs, ss, ds, fs, gs */
#	.long LDT0_SEL			/* ldt */
#	.long 0x8000000			/* trace bitmap */

# kernel stack for task0
	.fill 64,4,0
stack0_krn_ptr:
	.long 0

# kernel stack for task1
	.fill 32,4,0
	.long 0x0
	.long 0x0
	.long 0x0
	.long 0x0
	.long 0x17
	.long 0x0
	.long 0x0f
	.long 0x00000200
	.long 0x70000
	.long 0x17
stack1_krn_ptr:
	.long 0

# kernel stack for task2
	.fill 32,4,0
	.long 0x0
	.long 0x0
	.long 0x0
	.long 0x0
	.long 0x17
	.long 0x0
	.long 0x0f
	.long 0x00000200
	.long 0x80000
	.long 0x17
stack2_krn_ptr:
	.long 0

# kernel stack for task3
	.fill 32,4,0
	.long 0x0
	.long 0x0
	.long 0x0
	.long 0x0
	.long 0x17
	.long 0x0
	.long 0x0f
	.long 0x00000200
	.long 0x90000
	.long 0x17
stack3_krn_ptr:
	.long 0

# kernel stack for task4
	.fill 32,4,0
	.long 0x0
	.long 0x0
	.long 0x0
	.long 0x0
	.long 0x17
	.long 0x0
	.long 0x0f
	.long 0x00000200
	.long 0xA0000
	.long 0x17
stack4_krn_ptr:
	.long 0


.org 0x2000
pg_dir:

.org 0x3000
pg0:

.org 0x4000
pg1:

.org 0x5000

pg_dir_task0:

.org 0x6000

pg0_task0:

.org 0x7000

pg_dir_task1:

.org 0x8000

pg0_task1:

.org 0x9000

pg_dir_task2:

.org 0xA000

pg0_task2:

.org 0xB000

pg_dir_task3:

.org 0xC000

pg0_task3:

.org 0xD000

pg_dir_task4:

.org 0xE000

pg0_task4:


