#  head.s contains the 32-bit startup code.
#  Two L3 task multitasking. The code of tasks are in kernel area, 
#  just like the Linux. The kernel code is located at 0x10000. 
SCRN_SEL	= 0x28

.global startup_32
.text
startup_32:

	movl $0x10,%eax 	# 0x10 is the data segment selector in the GDT
	mov %ax,%ds
	lss init_stack,%esp
	
	# set up gdt and init idt
	lgdt lgdt_opcode
	lidt lidt_opcode

	jmp $0x08,$reload	# reload cs by jmp instruction
reload:
	movl $0x10,%eax		# reload all the segment registers
	mov %ax,%ds			# after changing gdt. 
	mov %ax,%es
	mov %ax,%fs
	lss init_stack,%esp


	mov $SCRN_SEL, %bx
	mov %bx,%ds
	movb $72,2900
	movb $0x1e,2901
	movb $69,2902
	movb $0x1e,2903
	movb $76,2904
	movb $0x1e,2905
	movb $76,2906
	movb $0x1e,2907
	movb $79,2908
	movb $0x1e,2909

	# set idt to handle the exception of protection
	movl $0x10,%eax	
	mov %ax,%ds	

	movl $0x00080000, %eax	
	movw $outofbound_exception, %ax
	movl $0x8E00, %edx
	movl $0x0d, %ecx
	lea idt(,%ecx,8), %esi
	movl %eax,(%esi) 
	movl %edx,4(%esi)

	# set idt to handle division error exception (entry 0)
	movl $0x00080000, %eax
	movw $div_error_exception, %ax
	movl $0x8E00, %edx
	movl $0x00, %ecx
	lea idt(,%ecx,8), %esi
	movl %eax,(%esi)
	movl %edx,4(%esi)

	# Trigger division error exception
	movl $1234,%eax
	xor %ecx, %ecx
	div %ecx

	# jump to an illegal address and cpu will trigger an exception
	jmp 8192


div_error_exception:

	mov $SCRN_SEL, %bx
	mov %bx,%ds
	movb $68,3914
	movb $0x1e,3915
	movb $73,3916
	movb $0x1e,3917
	movb $86,3918
	movb $0x1e,3919
	movb $45,3920
	movb $0x1e,3921
	movb $69,3922
	movb $0x1e,3923
	movb $82,3924
	movb $0x1e,3925
	movb $82,3926
	movb $0x1e,3927
	movb $79,3928
	movb $0x1e,3929
	movb $82,3930
	movb $0x1e,3931
	loop_div: jmp loop_div


outofbound_exception:

	mov $SCRN_SEL, %bx
	mov %bx,%ds
	movb $69,3914
	movb $0x1e,3915
	movb $82,3916
	movb $0x1e,3917
	movb $82,3918
	movb $0x1e,3919
	movb $79,3920
	movb $0x1e,3921
	movb $82,3922
	movb $0x1e,3923
	loop1: jmp loop1


lidt_opcode:
	.word 256*8-1		# idt contains 256 entries
	.long idt+0x10000	# This will be rewrite by code. 


lgdt_opcode:
	.word (end_gdt-gdt)-1	# the length of gdt 
	.long gdt+0x10000		# This will be rewrite by code.




idt:	.fill 256,8,0		# idt is uninitialized

gdt:	.quad 0x0000000000000000	/* NULL descriptor */
		.quad 0x00c09a0100000001	/* 8 KB 0x08, base = 0x10000 */
		.quad 0x00c0920100000001	/* 8 KB 0x10 */
		.quad 0x00c0920b80000002	/* screen 0x18 - for display */
end_gdt:

.fill 128,4,0
init_stack:                          # Will be used as user stack for task0.
	.long init_stack
	.word 0x10

.org 8192
	loop2: jmp loop2
