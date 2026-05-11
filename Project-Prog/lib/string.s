.global	memcpy
.global	memset
.global  strcpy
.global  strlen

# ------------------------------------------------------------------------
# void* memcpy(void* es:p_dst, void* ds:p_src, int size);
# ------------------------------------------------------------------------
memcpy:
	pushl 	%ebp
	movl 	%esp, %ebp

	pushl	%esi
	pushl	%edi
	pushl	%ecx

	push %ds
	push %es
	pushl %edx

	movl $0x10,%edx
	mov %dx,%ds
	movl $0x10,%edx
	mov %dx,%es

	movl 	8(%ebp), %edi	# Destination
	movl	12(%ebp), %esi	# Source
	movl	16(%ebp), %ecx	# Counter
1: 
	cmpl	$0, %ecx 	# 判断计数器
	jz		2f			# 计数器为零时跳出

	#movb	%ds:(%esi), %al
	#incl	%esi
	#movb	%al, %es:(%edi)
	#incl	%edi

	movb	(%esi), %al
	incl	%esi
	movb	%al, (%edi)
	incl	%edi

	decl	%ecx
	jmp		1b
2:
	movl 	8(%ebp), %eax

	popl 	%edx
	pop 	%es
	pop 	%ds
	popl	%ecx
	popl	%edi
	popl	%esi
	movl	%ebp, %esp
	popl 	%ebp

	ret

# ------------------------------------------------------------------------
# void memset(void* p_dst, char ch, int size);
# ------------------------------------------------------------------------
memset:
	pushl	%ebp
	movl	%esp, %ebp

	pushl	%esi
	pushl	%edi
	pushl	%ecx

	movl	8(%ebp), %edi
	movl	12(%ebp), %edx
	movl	16(%ebp), %ecx

1:	
	cmpl	$0, %ecx
	jz	2f

	movb 	%dl, (%edi)
	incl	%edi

	decl	%ecx
	jmp		1b
2:
	popl	%ecx
	popl	%edi
	popl	%esi
	movl	%ebp, %esp
	popl	%ebp

	ret

# ------------------------------------------------------------------------
# char* strcpy(char* p_dst, char* p_src);
# ------------------------------------------------------------------------
strcpy:
	pushl	%ebp
	movl	%esp, %ebp

	movl	12(%ebp), %esi	# Source
	movl	8(%ebp), %edi	# Destination

1:
	movb	(%esi), %al
	incl	%esi
	movb	%al, (%edi)
	incl	%edi

	cmpb	$0, %al	# 是否遇到 '\0'
	jnz		1b		# 没遇到就继续循环，遇到就结束

	mov 	8(%ebp), %eax	# 返回值
	popl	%ebp
	ret

# ------------------------------------------------------------------------
# int strlen(char* p_str);
# ------------------------------------------------------------------------
strlen:
	pushl	%ebp
	movl	%esp, %ebp

	movl	$0, %eax		# 字符串长度开始是 0
	movl	8(%ebp), %esi	# esi 指向首地址

1:
	cmpb	$0, (%esi)		# 看 esi 指向的字符是否是 '\0'
	jz 		2f				# 如果是 '\0'，程序结束
	incl	%esi			# 如果不是 '\0'，esi 指向下一个字符
	incl	%eax			# 并且，eax 自加一
	jmp		1b				# 如此循环
2:
	popl	%ebp
	ret
