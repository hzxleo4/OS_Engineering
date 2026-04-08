#  head.s contains the 32-bit startup code.
#  Two L3 task multitasking. The code of tasks are in kernel area, 
#  just like the Linux. The kernel code is located at 0x10000. 
KRN_BASE 	= 0x10000
SCRN_SEL	= 0x18
LDT0_SEL	= 0x20
//LDT0_SEL    = 0x24  # 原 0x20 → 0x24，TI=1（bit2=1），指向 GDT 中的 LDT 描述符
TSS0_SEL	= 0x28
PAGE_OFFSET = 0xC0000000

#.extern tty_routine
#.extern write_char_routine
#.extern sys_write
#.extern sys_printx
#.extern sys_sendrec
#.extern sys_getticks
#.extern sys_hd_routine
#.extern MemChkBuf, dwMCRNumber

.extern sys_call_table
.extern current

.global startup_32
.global move_to_user_mode,set_cr3_test
.global idt,gdt,tss0,ldt0,pg_dir,pg0,pg1,pg2,pg3,stack0_krn_ptr,stack0_ptr,myjump,finish_paging,set_cr3
.global setup_paging,before_task1_paging,tss1,stack1_ptr,stack1_krn_ptr, new_task_next_ip

.global write_char
#.global systemcall_interrupt_1, systemcall_interrupt_2, systemcall_interrupt_3, systemcall_interrupt_4, systemcall_interrupt_5, systemcall_interrupt_6, systemcall_interrupt_7
.global systemcall
.global timer_interrupt, keyboard_interrupt, harddisk_interrupt, pagefault_interrupt
#.global get_mem_info
.global pg_dir_tasks, pg_tasks


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
	movl $pg1+0x07-PAGE_OFFSET,pg_dir+4*768-PAGE_OFFSET		/*  --------- " " --------- */

	# map (3GB+4MB) - (3GB+8MB) to 0 - 4 MB
	movl $pg1+4092-PAGE_OFFSET,%edi
	movl $0x3ff007,%eax		/*  4Mb - 4096 + 7 (r/w user,p) */
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
	#movl $0x21, %edx
	#inb %dx, %al
	#andb $0xfe, %al
	#outb %al, %dx

	#unmask the keyboard interrupt.
	#movl $0x21, %edx
	#inb %dx, %al
	#andb $0xfd, %al
	#outb %al, %dx

	#unmask the harddisk interrupt.
	#movl $0xA1, %edx
	#inb %dx, %al
	#andb $0xfe, %al
	#outb %al, %dx

	pushfl
	andl $0xffffbfff, (%esp)
	popfl
	movl $TSS0_SEL, %eax
	ltr %ax
	movl $LDT0_SEL, %eax
	lldt %ax 

    # 新增：显式加载用户态 DS/ES/FS/GS 段寄存器（LDT 的 0x17 数据段，RPL=3）
    movw $0x17, %ax  # LDT 数据段选择子（0x17 = 0x10 + 0x7，TI=1表示LDT，RPL=3）
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

	#update cr3 for task0
	movl $pg_dir_tasks-PAGE_OFFSET,%eax	
	movl %eax,%cr3		/* cr3 - page directory start */

	sti
	pushl $0x17
	pushl $0x10000
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


systemcall:

	push %ds
	pushl %edx
	pushl %ecx
	pushl %ebx
	movl $0x10,%edx
	mov %dx,%ds
	mov %dx,%es
	call sys_call_table(, %eax, 4)
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

	popl %eax
	popl %ebx
	popl %ecx
	popl %edx	
	pop %ds
new_task_next_ip:
	iret

keyboard_interrupt:
	push %ds
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	movl $0x10,%edx
	mov %dx,%ds
	movb $0x20, %al
	outb %al, $0x20
	call keyboard_handler
	popl %eax
	popl %ebx
	popl %ecx
	popl %edx
	pop %ds
	
	iret

# 缺页异常中断处理入口（page_fault_interrupt）
pagefault_interrupt:
    # 1. 保存通用寄存器和段寄存器（保护现场）
    cli
	push %ds
    push %es
	push %fs
	push %gs
    pushl %ebp
    pushl %edi
    pushl %esi
    pushl %edx
    pushl %ecx
    pushl %ebx
    pushl %eax

    # 2. 切换到内核数据段（0x10，与键盘/定时器中断保持一致）
    movl $0x10, %edx
    mov %dx, %ds
    mov %dx, %es      # es同步切换为内核数据段，避免段访问异常

    # 3. 提取缺页异常核心信息（CPU自动压入错误码 + CR2存储出错地址）
	movl %esp, %ebp          # 用EBP定位栈
    pushl 36(%ebp)     # 错误码：CPU自动压入的，栈偏移=9个push×4=36 + 4=40
    movl %cr2, %eax   # CR2寄存器是x86架构专属，存储缺页的线性地址
    pushl %eax        # 把出错地址压栈，作为C处理函数的参数

    # 4. 调用C层缺页处理逻辑（page_fault_handler需自行实现）
    call pagefault_handler

    # 5. 清理栈帧（弹出出错地址和栈指针，共8字节）
    addl $8, %esp

    # 6. 缺页是CPU内部异常，非8259外设中断，无需发送EOI中断结束信号
    #    （区别于键盘/定时器中断，此处无outb 0x20操作）

    # 7. 恢复寄存器（恢复现场）
    popl %eax
    popl %ebx
    popl %ecx
    popl %edx
    popl %esi
    popl %edi
    popl %ebp
	pop %gs
	pop %fs
    pop %es
    pop %ds
	sti
	
	# 移除栈上的错误码（重要！）
	addl $4, %esp # 跳过错误码

    # 8. 中断返回：iret会自动弹出CPU压入的错误码 + CS/EIP/EFLAGS，完成异常返回
    iret

harddisk_interrupt:
	push %ds
	push %es
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
	movl $0x10,%edx
	mov %dx,%ds
	mov %dx,%es
	movb $0x20, %al
	outb %al, $0x20
	movb $0x20, %al
	outb %al, $0xA0
	call hd_handler
	popl %eax
	popl %ebx
	popl %ecx
	popl %edx
	pop %es
	pop %ds
	
	iret

/*******************/

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
	.quad 0x00cf92000000ffff	/* 4Gb 0x10 */
	.quad 0x00c0920b80000008	/* screen 0x18 - for display */
	.fill 254,4,0
end_gdt:



.fill 128,4,0
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
	.long pg_dir_tasks-PAGE_OFFSET				/* cr3 */
	.long 0x0			/* eip */
	.long 0x200			/* eflags */
	.long 0, 0, 0, 0		/* eax, ecx, edx, ebx */
	.long 0x10000, 0, 0, 0	/* esp, ebp, esi, edi */
	.long 0x17,0x0f,0x17,0x17,0x17,0x17 /* es, cs, ss, ds, fs, gs */
	.long LDT0_SEL			/* ldt */
	.long 0x8000000			/* trace bitmap */



# kernel stack for task0
.align 4096
	.fill 1024,4,0
stack0_krn_ptr:
	.long 0

# kernel stack for task1
.align 4096
	.fill 1019,4,0
	.long 0x0
	.long 0x0f
	.long 0x00000200
	.long 0x10000
	.long 0x17
stack1_krn_ptr:
	.long 0


.align 4096
pg_dir:
	.fill 1024,4,0

#.org 0x3000
.align 4096
pg0:
.fill 1024,4,0

#.org 0x4000
.align 4096
pg1:
.fill 1024,4,0

#.org 0x5000
#.align 4096
#pg_dir_task0:
#.fill 1024,4,0
#.org 0x6000
#.align 4096
#pg0_task0:
#.fill 1024,4,0
#.org 0x7000
#.align 4096
#pg_dir_task1:
#.fill 1024,4,0
#.org 0x8000
#.align 4096
#pg0_task1:
#.fill 1024,4,0
#.org 0x9000

.align 16384
pg_dir_tasks:
.fill 4096,4,0

.align 16384
pg_tasks:
.fill 4096,4,0


