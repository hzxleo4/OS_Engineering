# ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#                              klibs.s
# ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

P_STACKBASE	=	0
GSREG		=	P_STACKBASE
FSREG		=	GSREG		+ 4
ESREG		=	FSREG		+ 4
DSREG		=	ESREG		+ 4
EDIREG		=	DSREG		+ 4
ESIREG		=	EDIREG		+ 4
EBPREG		=	ESIREG		+ 4
KERNELESPREG	=	EBPREG		+ 4
EBXREG		=	KERNELESPREG	+ 4
EDXREG		=	EBXREG		+ 4
ECXREG		=	EDXREG		+ 4
EAXREG		=	ECXREG		+ 4
RETADR		=	EAXREG		+ 4
EIPREG		=	RETADR		+ 4
CSREG		=	EIPREG		+ 4
EFLAGSREG	=	CSREG		+ 4
ESPREG		=	EFLAGSREG	+ 4
SSREG		=	ESPREG		+ 4
P_STACKTOP	=	SSREG		+ 4
P_LDT_SEL	=	P_STACKTOP
P_LDT		=	P_LDT_SEL	+ 4

INT_M_CTL	=	0x20	# I/O port for interrupt controller         <Master>
INT_M_CTLMASK	=	0x21	# setting bits in this port disables ints   <Master>
INT_S_CTL	=	0xA0	# I/O port for second interrupt controller  <Slave>
INT_S_CTLMASK	=	0xA1	# setting bits in this port disables ints   <Slave>

EOI		=	0x20

.extern disp_pos

.global	disp_str
.global	out_byte
.global	in_byte
.global	enable_int
.global	disable_int

# ========================================================================
#                  void disp_str(char * info);
# ========================================================================
disp_str:
	pushl %ebp
	movl	%esp, %ebp
	movl	8(%ebp), %esi
	movl	disp_pos, %edi
	movb	$0x0F, %ah

	push %gs
	pushl %ebx
	movl $0x18, %ebx 
	movw %bx, %gs 
1:
	lodsb
	test	%al, %al
	jz	2f
	movb	$0x0A, %bl
	cmp	%al, %bl
	jnz	3f
	pushl	%eax
	movl	%edi, %eax
	movb	$160, %bl
	div	%bl
	andl	$0x0FF, %eax
	inc	%eax
	movb	$160, %bl
	mul	%bl
	mov	%eax, %edi
	popl	%eax
	jmp	1b
3:
	movw	%ax, %gs:(%edi)
	addl	$2, %edi
	jmp	1b
2: 
	movl	%edi, disp_pos

	popl 	%ebx
	popw 	%gs
	popl	%ebp
	ret

# ========================================================================
#                  void out_byte(u16 port, u8 value);
# ========================================================================
out_byte:
	movl	4(%esp), %edx	# port
	movb	8(%esp), %al	# value
	outb	%al, %dx
	nop	# 一点延迟
	nop
	ret

# ========================================================================
#                  u8 in_byte(u16 port);
# ========================================================================
in_byte:
	movl	4(%esp), %edx 		# port
	xorl	%eax, %eax
	inb		%dx, %al 
	nop	# 一点延迟
	nop
	ret

# ========================================================================
#		   void disable_int();
# ========================================================================
disable_int:
	cli
	ret

# ========================================================================
#		   void enable_int();
# ========================================================================
enable_int:
	sti
	ret